/* 
  matrix summation using OpenMP

  usage with gcc:
    gcc -O -fopenmp -o out matrix_sum_openmp.c 
    ./out <size> <num_workers>
*/

#include <limits.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SIZE 25000  // maximum matrix size 
#define MAX_WORKERS 16  // maximum number of workers
#define MAX_VALUE 1000  // maximum value of element

int matrix[MAX_SIZE][MAX_SIZE];
int size;

// init_matrix fills the matrix with random values.
void init_matrix() {
  srand(time(NULL));
  for (int row = 0; row < size; row++)
	  for (int col = 0; col < size; col++)
      matrix[row][col] = rand() % MAX_VALUE;
}

// print_matrix print the contents of matrix to stdout.
void print_matrix() {
  printf("matrix:\n");
  for (int row = 0; row < size; row++) {
	  for (int col = 0; col < size; col++) {
      printf("%4d ", matrix[row][col]);
	  }
    printf("\n");
  }
  printf("\n");
}

// test calculates the sum sequentially (just for testing purpose).
void test() {
  long total = 0;
  for (int row = 0; row < size; row++)
	  for (int col = 0; col < size; col++)
      total += matrix[row][col];
  printf("sum (test): %ld\n", total);
}

typedef struct {
  int value;
  int row;
  int col;  
} Elem;

// main. read command line, initialize, and create threads.
int main(int argc, char *argv[]) {
  // command line args
  size = (argc > 1) ? atoi(argv[1]) : MAX_SIZE;
  int num_workers = (argc > 2) ? atoi(argv[2]) : MAX_WORKERS;
  if (size > MAX_SIZE) {
    size = MAX_SIZE;
  }
  if (num_workers > MAX_WORKERS) {
    num_workers = MAX_WORKERS;
  }

  // init
  init_matrix();

  omp_set_num_threads(num_workers);
  double start_time = omp_get_wtime();

  // sum, min, max
  long total = 0;
  int row, col;

  // omp_in is the partial result local to each thread, omp_out references final value after reduction. omp_priv is the initial value of private.
#pragma omp declare reduction(mini : Elem : omp_out = omp_in.value < omp_out.value ? omp_in : omp_out) initializer(omp_priv = {INT_MAX, -1, -1})
  Elem min = {INT_MAX, -1, -1};
#pragma omp declare reduction(maxi : Elem : omp_out = omp_in.value > omp_out.value ? omp_in : omp_out) initializer(omp_priv = {INT_MIN, -1, -1})
  Elem max = {INT_MIN, -1, -1};

#pragma omp parallel for reduction(+:total) reduction(mini:min) reduction(maxi:max) private(col)
  for (row = 0; row < size; row++) {
    for (col = 0; col < size; col++){
      total += matrix[row][col];
      if (matrix[row][col] < min.value) {
        min.value = matrix[row][col];
        min.row = row;
        min.col = col;
      }
      if (matrix[row][col] > max.value) {
        max.value = matrix[row][col];
        max.row = row;
        max.col = col;
      }
    }
  }

  // result
  double end_time = omp_get_wtime();
  printf("total sum: %ld\n", total);
  printf("min value: %d, row: %d, col: %d\n", min.value, min.row, min.col);
  printf("max value: %d, row: %d, col: %d\n", max.value, max.row, max.col);
  printf("it took: %g seconds\n", end_time - start_time);

  return 0;
}

// ----- BENCHMARKS ----- //

// OBS: time given in seconds!

// 1 worker:
//
// n = 1 000:  0.000617971, 0.000550561, 0.000518891, 0.000531661, 0.00091914
// n = 5 000:  0.0144823, 0.0210428, 0.0146157, 0.0152325, 0.0151701
// n = 10 000: 0.0590837, 0.0587444, 0.0591411, 0.0590605, 0.0678576
// n = 25 000: 0.364172, 0.371191, 0.359799, 0.348585, 0.338681

// 2 workers:
//
// n = 1 000:  0.000587661, 0.0004723, 0.000465471, 0.00051165, 0.00053767
// n = 5 000:  0.00780203, 0.00760044, 0.00755674, 0.00773756, 0.00827722
// n = 10 000: 0.0334583, 0.030183, 0.0311585, 0.0295619, 0.0343968
// n = 25 000: 0.186708, 0.186202, 0.221548, 0.1833, 0.175461

// 4 workers:
//
// n = 1 000:  0.00060746, 0.0003633, 0.0004598, 0.00032312, 0.000379961
// n = 5 000:  0.00420517, 0.00449698, 0.00534637, 0.00434458, 0.00411235
// n = 10 000: 0.0214218, 0.0153514, 0.0162699, 0.017506, 0.0234972
// n = 25 000: 0.110763, 0.105742, 0.14561, 0.138983, 0.120365

// 8 workers:
//
// n = 1 000:  0.00045639, 0.000326871, 0.000355851, 0.00028826, 0.00037066
// n = 5 000:  0.00340789, 0.00411231, 0.00367292, 0.00441999, 0.0059132
// n = 10 000: 0.0167978, 0.0183835, 0.0133747, 0.0202045, 0.0155894
// n = 25 000: 0.0767498, 0.0766681, 0.0784072, 0.0794835, 0.0828636

// 16 workers:
//
// n = 1 000:  0.000563074, 0.000605013, 0.000561343, 0.000625223, 0.000659104
// n = 5 000:  0.00414018, 0.0044013, 0.00470704, 0.00417224, 0.00480796
// n = 10 000: 0.0156434, 0.0145758, 0.0142295, 0.0140835, 0.0135682 
// n = 25 000: 0.0852079, 0.0780734, 0.079582, 0.0825748, 0.0768576