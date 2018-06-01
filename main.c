#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "omp.h"

#define EPS 1e-6
#define NUMBER_OF_TESTS 20
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0
#define CHUNK_SIZE 64

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (1; A).
 *
 * @param n size of generated array
 * @param seed random generator´s seed
 * @return random array
 */
double *generate_m1(int n, unsigned int *seed);

/**
 * Maps values of given array: m1[i] = sqrt(m1[i] / E)
 *
 * @param n size of given array
 * @param m1 an array
 */
void map_m1(int n, double *m1);

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (A; 10A).
 *
 * @param n size of generated array
 * @param seed random generator's seed
 * @return random array
 */
double *generate_m2(int n, unsigned int *seed);

/**
 * Maps values of given array: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param n size of given array
 * @param m2 an array
 */
void map_m2(int n, double *m2);

/**
 * Merges values of given arrays into M2 array.
 * Applies function to values while merging: f(a, b) = a * b
 *
 * @param n size of M2 array
 * @param m1 a M1 array
 * @param m2 a M2 array
 */
void merge(int n, double *m1, double *m2);

/**
 * Sorts given array using Insertion Sort algorithm.
 *
 * @see https://en.wikipedia.org/wiki/Insertion_sort
 * @param n size of given array
 * @param array an array
 */
void sort(int n, double *array);

/**
 * Sorts given array using parallelization.
 * Inspired by Regular Sampling Parallel Sort Algorithm.
 *
 * @param n size of given array
 * @param array an array
 */
void rs_sort(int n, double *array);

/**
 * Finds minimal positive element of given array.
 *
 * @param n size of given array
 * @param array sorted array
 * @return minimal positive element or 0.0 if no positive elements found
 */
double min_positive(int n, double *array);

/**
 * Reduces given array into single double value by function:
 * f(a[i]) = f(a[i-1]) + ([a / min] % 2 == 0 ? sin(a / min) : 0.0)
 *
 * @param n size of given array
 * @param array an array
 * @param min minimal value for dividing, must be positive
 * @return reduced sum
 */
double reduce(int n, double *array, double min);

/**
 * Apply sum function to pairs of zipped arrays.
 * 
 * @param left result array
 * @param right an array
 * @param len size of zipped arrays
 */
void zip_sum(double *left, double *right, int len);

/**
 *  Does work for given steps:
 *  - generating arrays
 *  - mapping them
 *  - sorting and merging
 *  - reducing into result
 *
 * @param n size of work (N)
 * @param seed a random seed
 * @return reduced result of work
 */
double do_work(int n, unsigned int seed);

int main(int argc, char* argv[]) {
    int n;
    unsigned int seed;
    int i;
    long int best_time;
    double expected_result;

    n = atoi(argv[1]);
    seed = (unsigned int) atoi(argv[2]);

    best_time = MAX_EXECUTION_TIME;
    for (i = 0; i < NUMBER_OF_TESTS; ++i) {
        struct timeval start, end;
        long int elapsed_time;
        double work_result;

        srand(seed);
        gettimeofday(&start, NULL);
        work_result = do_work(n, seed);
        gettimeofday(&end, NULL);

        if (i == 0) {
            expected_result = work_result;
        } else if (fabs(expected_result - work_result) > EPS) {
            printf("\nFailed to verify result of work: expected=%.5f actual=%.5f", 
                expected_result, work_result);
            return 1; 
        }
#ifdef DEBUG
        printf("Work result = %lf\n", work_result);
#endif
        elapsed_time = (end.tv_sec - start.tv_sec) * USEC_IN_SECOND;
        elapsed_time += (end.tv_usec - start.tv_usec);
        if (elapsed_time < best_time) {
#ifdef DEBUG
            printf("Best time will be updated: step=%d old=%ldus new=", i, best_time);
#endif
            best_time = elapsed_time;
        }
        printf("%ld;%f\n", elapsed_time, work_result);
    }
    return 0;
}

double do_work(int n, unsigned int seed) {
    double *m1, *m2;
    int m1_size, m2_size;
    double min_value;
    double x;

    m1_size = n;
    m2_size = n / 2;

    m1 = generate_m1(m1_size, &seed);
    m2 = generate_m2(m2_size, &seed);

    map_m1(m1_size, m1);
    map_m2(m2_size, m2);

    merge(m2_size, m1, m2);

    rs_sort(m2_size, m2);

    min_value = min_positive(m2_size, m2);
    if (min_value == 0.0) {
        printf("Failed to find minimal value :(\n");
        exit(100);
    }

    x = reduce(m2_size, m2, min_value);
    free(m1);
    free(m2);
    return x;
}

double *generate_m1(int n, unsigned int *seed) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    /**
     * Loop has dependency. Inefficient parallelization/
     */
    for (i = 0; i < n; ++i) {
        array[i] = (double) rand_r(seed);
    }

    // #pragma omp parallel for default(none) private(i) shared(array,n)
    for (i = 0; i < n; ++i) {
        double val;
        val = array[i];
        val /= (double) RAND_MAX;
        val *= (A - 1.0);
        val += 1.0;
        array[i] = val;
    }
    return array;
}

void map_m1(int n, double *m1) {
    int i;
    // #pragma omp parallel for default(none) private(i) shared(m1,n)
    for (i = 0; i < n; ++i) {
        m1[i] = sqrt(m1[i] / M_E);
    }
}

double *generate_m2(int n, unsigned int *seed) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    /**
     * Loop has dependency. Inefficient parallelization/
     */
    for (i = 0; i < n; ++i) {
        array[i] = (double) rand_r(seed);
    }

    // #pragma omp parallel for default(none) private(i) shared(array,n)
    for (i = 0; i < n; ++i) {
        double val;
        val = array[i];
        val /= (double) RAND_MAX;
        val *= (A * 9.0);
        val += A;
        array[i] = val;
    }
    return array;
}

void map_m2(int n, double *m2) {
    int i;
    double *temp;

    temp = malloc(sizeof(double) * n);
    // #pragma omp parallel for default(none) private(i) shared(temp,m2,n)
    for (i = 0; i < n; ++i) {
        temp[i] = m2[i];
    }

    zip_sum(m2 + 1, temp, n - 1);

    // #pragma omp parallel for default(none) private(i) shared(m2,n)
    for (i = 0; i < n; ++i) {
        m2[i] = fabs(tan(m2[i]));
    }
    free(temp);
}

void zip_sum(double *left, double *right, int len) {
    int i;
    // #pragma omp parallel for default(none) private(i) shared(left,right,len)
    for (i = 0; i < len; ++i) {
        left[i] += right[i];
    }
}

void merge(int n, double *m1, double *m2) {
    int i;
    // #pragma omp parallel for default(none) private(i) shared(m1,m2,n)
    for (i = 0; i < n; ++i) {
        m2[i] *= m1[i];
    }
}

void rs_sort(int n, double *array) {
    int chunks;
    int chunk_size;
    int chunk_n;

    chunk_size = CHUNK_SIZE;
    chunks = (n + chunk_size - 1) / chunk_size;

    // #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n)
    for (chunk_n = 0; chunk_n < chunks; ++chunk_n) {
        int subsize;
        subsize = n - chunk_size * chunk_n;
        subsize = subsize < chunk_size ? subsize : chunk_size;
        sort(subsize, array + chunk_n * chunk_size);
    }

    for (; chunk_size < n; chunk_size *= 2) {
        chunks = (n + chunk_size - 1) / chunk_size;
        // #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n)
        for (chunk_n = 0; chunk_n < chunks; chunk_n += 2) {
            double *temp;
            int temp_size;
            int i, j, k;
            int chunk_0, chunk_1, chunk_2;

            temp = malloc(sizeof(double) * chunk_size * 2);

            chunk_0 = chunk_n * chunk_size;
            chunk_1 = chunk_0 + chunk_size;
            chunk_2 = chunk_1 + chunk_size;

            temp_size = 0;
            for (j = chunk_0, k = chunk_1; j < n && k < n && j < chunk_1 && k < chunk_2;) {
                if (array[j] < array[k]) {
                    temp[temp_size++] = array[j++];
                } else {
                    temp[temp_size++] = array[k++];
                }
            }
            while (j < n && j < chunk_1) {
                temp[temp_size++] = array[j++];
            }
            while (k < n && k < chunk_2) {
                temp[temp_size++] = array[k++];
            }
            for (i = 0; i < temp_size; ++i) {
                array[chunk_0 + i] = temp[i];
            }
            free(temp);
        }
    }
}

/**
 * Sorting algorithm invariants will be broken while parallelizing
 */
void sort(int n, double *array) {
    int i, j;
    double key;
    int lo, hi, m;

    for (i = 1; i < n; ++i) {
        key = array[i];
        for (j = i - 1; j >= 0 && array[j] > key; --j) {
            array[j + 1] = array[j];
        }
        array[j + 1] = key;
    }
}

/**
 * This method has O(1) complexity in common case
 */
double min_positive(int n, double *array) {
    int i;
    for (i = 0; i < n; ++i) {
        if (array[i] != 0.0) {
            return array[i];
        }
    }
    return 0.0;
}

double reduce(int n, double *array, double min) {
    int i;
    double sum;

    sum = 0.0;
    // #pragma omp parallel for default(none) private(i) shared(array,min,n) reduction(+:sum)
    for (i = 0; i < n; ++i) {
        double value;

        value = array[i] / min;
        if (!(((int) value) % 2)) {
            sum += sin(value);
        }
    }
    return sum;
}
