/* 
  matrix summation using pthreads

  features: 
    uses a barrier; the Worker[0] computes
    the total sum from partial sums computed by Workers
    and prints the total sum to the standard output

  usage under Linux:
    gcc -o sum matrix_sum_a.c -lpthread
    ./sum <size> <num_workers>
*/

#ifndef _REENTRANT 
#define _REENTRANT 
#endif 

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define MAX_SIZE 10000      // maximum matrix size
#define MAX_WORKERS 10      // maximum number of workers
#define MAX_VALUE 1000      // maximum value of an element in matrix

pthread_mutex_t mutex;      // mutex lock for the barrier
pthread_cond_t go;          // condition variable for leaving barrier

int num_workers;       
int num_arrived = 0;

// barrier is a reusable counter barrier.
void barrier() {
  pthread_mutex_lock(&mutex);           
  num_arrived++;
  if (num_arrived == num_workers) {    
    num_arrived = 0;
    pthread_cond_broadcast(&go);
  } else {
    pthread_cond_wait(&go, &mutex);    
  }
  pthread_mutex_unlock(&mutex);     
}

// read_timer returns the current time. 
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized ) {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

typedef struct {
    int row;
    int col;
    int value;
} Elem;

double start_time;
double end_time;
int size;   // assume size is multiple of numWorkers
int strip_size;  
long part_sums[MAX_WORKERS];     // partial sums 
Elem part_max[MAX_WORKERS];     // partial max
Elem part_min[MAX_WORKERS];     // partial min
int matrix[MAX_SIZE][MAX_SIZE]; 

// Each worker sums the values in one strip of the matrix.
// After barrier, worker(0) computes and prints the total.
void *worker(void *arg) {
  long id = (long) arg;
  
  // determine first and last rows of my strip
  int first = id * strip_size;
  int last = (id == num_workers - 1) ? (size - 1) : (first + strip_size - 1);

  // sum values in my strip and find max and min
  long total = 0;
  Elem min = {-1, -1, INT_MAX};
  Elem max = {-1, -1, INT_MIN};
  for (int row = first; row <= last; row++) {
    for (int col = 0; col < size; col++) {
      total += matrix[row][col];
      if (matrix[row][col] < min.value) {
        min.row = row;
        min.col = col;
        min.value = matrix[row][col];
      }
      if (matrix[row][col] > max.value) {
        max.row = row;
        max.col = col;
        max.value = matrix[row][col];
      }
    }
  }
  part_sums[id] = total;
  part_min[id] = min;
  part_max[id] = max;

  // wait for all threads to finish their calculations
  barrier();

  // check final result
  if (id == 0) {
    long final_total = 0;
    Elem final_min = {-1, -1, INT_MAX};
    Elem final_max = {-1, -1, INT_MIN};

    for (int i = 0; i < num_workers; i++) {
      final_total += part_sums[i];
      if (part_min[i].value < final_min.value) {
        final_min = part_min[i];
      }
      if (part_max[i].value > final_max.value) {
        final_max = part_max[i];
      }
    }
      
    // print result
    end_time = read_timer();
    printf("The total sum is: %ld\n", final_total);
    printf("The min value is: %d and its position is (%d, %d) \n", final_min.value, final_min.row, final_min.col);
    printf("The max value is: %d and its position is (%d, %d) \n", final_max.value, final_max.row, final_max.col);
    printf("The execution time is %g sec\n", end_time - start_time);
  }
}

// read command line, initialize, and create threads
int main(int argc, char *argv[]) {
  pthread_attr_t attr;
  pthread_t threads[MAX_WORKERS];

  // set global thread attributes
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  // initialize mutex and condition variable
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&go, NULL);

  // command line args
  size = (argc > 1) ? atoi(argv[1]) : MAX_SIZE;
  num_workers = (argc > 2) ? atoi(argv[2]) : MAX_WORKERS;
  if (size > MAX_SIZE) {
    size = MAX_SIZE;
  }
  if (num_workers > MAX_WORKERS) {
    num_workers = MAX_WORKERS;
  }
  strip_size = size/num_workers;

  // initialize matrix
  srand(time(NULL));
  for (int row = 0; row < size; row++) {
	  for (int col = 0; col < size; col++) {
          matrix[row][col] = rand() % MAX_VALUE;
	  }
  }

  // create workers
  start_time = read_timer();
  for (long id = 0; id < num_workers; id++) {
    if(pthread_create(&threads[id], &attr, worker, (void *) id) != 0) {
      fprintf(stderr, "Creating thread failed");
      return 1;
    }
  }
  pthread_exit(NULL);
}