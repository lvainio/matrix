/* 
  matrix summation using pthreads

  usage under Linux:
    gcc -o sum matrix_sum_c.c -lpthread
    ./sum <size> <num_workers>
*/

#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#define MAX_SIZE 10000      // maximum matrix size
#define MAX_WORKERS 10      // maximum number of workers
#define MAX_VALUE 1000      // maximum value of an element in matrix

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
    int min_value;
    int max_value;
    int min_row;
    int min_col;
    int max_row;
    int max_col;
    long sum;
} Result;

int num_workers;
int matrix[MAX_SIZE][MAX_SIZE]; 
int size;

pthread_mutex_t mutex;
int curr_row = 0;
  
// Each worker returns partial result to main thread.
void *worker(void *arg) {
    // sum values from my tasks and find max and min
    Result res = {INT_MAX, INT_MIN, -1, -1, -1, -1, 0};
    while (1) { 
        pthread_mutex_lock(&mutex);
        if (curr_row >= size) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        int row = curr_row;
        curr_row++;
        pthread_mutex_unlock(&mutex);

        for (int col = 0; col < size; col++) {
            res.sum += matrix[row][col];
            if (matrix[row][col] < res.min_value) {
                res.min_value = matrix[row][col];
                res.min_row = row;
                res.min_col = col;
            }
            if (matrix[row][col] > res.max_value) {
                res.max_value = matrix[row][col];
                res.max_row = row; 
                res.max_col = col;
            }
        }
    }

    Result* ret = malloc(sizeof(Result));
    *ret = res;
    return (void*) ret;
}

// read command line, initialize, and create threads
int main(int argc, char *argv[]) {
    // command line args
    size = (argc > 1) ? atoi(argv[1]) : MAX_SIZE;
    if (size > MAX_SIZE) {
        size = MAX_SIZE;
    }
    num_workers = (argc > 2) ? atoi(argv[2]) : MAX_WORKERS;
    if (num_workers > MAX_WORKERS) {
        num_workers = MAX_WORKERS;
    }
    if (size == 0) {
        printf("The total sum is: 0\n");
        return 0;
    }

    pthread_mutex_init(&mutex, NULL);

    // initialize matrix
    srand(time(NULL));
    for (int row = 0; row < size; row++) {
        for (int col = 0; col < size; col++) {
            matrix[row][col] = rand() % MAX_VALUE;
        }
    }

    // create workers
    pthread_t threads[MAX_WORKERS];
    double start_time = read_timer();
    for (long id = 0; id < num_workers; id++) {
        if(pthread_create(&threads[id], NULL, worker, NULL) != 0) {
            fprintf(stderr, "Creating thread failed");
            return 1;
        } 
    }

    // get result from workers
    Result* res;
    Result final_res = {INT_MAX, INT_MIN, -1, -1, -1, -1, 0};
    for(int i = 0; i < num_workers; i++) {
        if(pthread_join(threads[i], (void**) &res) != 0) {
            fprintf(stderr, "Joining thread failed");
            return 1;
        }
        final_res.sum += res->sum;
        if (res->min_value < final_res.min_value) {
            final_res.min_value = res->min_value;
            final_res.min_row = res->min_row;
            final_res.min_col = res->min_col;
        }
        if (res->max_value > final_res.max_value) {
            final_res.max_value = res->max_value;
            final_res.max_row = res->max_row;
            final_res.max_col = res->max_col;
        } 
        free(res);
    }    

    double end_time = read_timer();
    printf("The total sum is: %ld\n", final_res.sum);
    printf("The min value is: %d and its position is (%d, %d) \n", final_res.min_value, final_res.min_row, final_res.min_col);
    printf("The max value is: %d and its position is (%d, %d) \n", final_res.max_value, final_res.max_row, final_res.max_col);
    printf("The execution time is: %g sec\n", end_time - start_time);

    return 0;
}