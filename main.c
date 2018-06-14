#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>
#include "omp.h"

#define EPS 1e-6
#define NUMBER_OF_TESTS 10
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0
#define CHUNK_SIZE 64

#ifndef _OPENMP
double omp_get_wtime(void) {
    struct timeval wtime;
    gettimeofday(&wtime, NULL);
    return wtime.tv_sec + ((double) wtime.tv_usec) / USEC_IN_SECOND;
}

int omp_get_num_procs(void) {
    return 1;
}
#endif

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (1; A).
 *
 * @param n size of generated array
 * @return random array
 */
double *generate_m1(int n);

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
 * @return random array
 */
double *generate_m2(int n);

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
 * Sorts given array using parallelization.
 * Splits array into 2 equal parts, sorts them, then merges into one.
 *
 * @param n size of given array
 * @param array an array
 */
void rs_sort2(int n, double *array);

/**
 * Sorts given array using parallelization.
 * Splits array into K equal parts, sorts them, then merges into one.
 * K is number of OMP threads.
 *
 * @param n size of given array
 * @param array an array
 */
void rs_sortK(int n, double *array);

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
 * @return reduced result of work
 */
double do_work(int n);

int main(int argc, char* argv[]) {
    int n;
    int i;
    long int best_time;
    double expected_result;

    n = atoi(argv[1]);

    best_time = MAX_EXECUTION_TIME;
    for (i = 0; i < NUMBER_OF_TESTS; ++i) {
        double work_result;

        work_result = do_work(n);

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
    }
    return 0;
}

double do_work(int n) {
    double x;
    int progress;

    progress = 0;

    #pragma omp parallel sections default(none) shared(n,x,progress)
    {
        #pragma omp section
        {
#ifdef _OPENMP
            while (progress < 100) {
                sleep(1);
#ifdef DEBUG                
                printf("Progress: %d\n", progress);
#endif                
            }
#endif            
        }
        #pragma omp section
        {
            double *m1, *m2;
            int m1_size, m2_size;
            double min_value;
            int sort_type;
            double start, end;

            start = omp_get_wtime();

            m1_size = n;
            m2_size = n / 2;

            m1 = generate_m1(m1_size);
            progress += 5;
            m2 = generate_m2(m2_size);
            progress += 5;

            map_m1(m1_size, m1);
            progress += 10;
            map_m2(m2_size, m2);
            progress += 10;

            merge(m2_size, m1, m2);
            progress += 10;

            sort_type = atoi(getenv("LAB_SORT_TYPE"));
            switch (sort_type) {
                case 0:
                    rs_sort(m2_size, m2);
                    break;
                case 1:
                    rs_sort2(m2_size, m2);
                    break;
                case 2:
                    rs_sortK(m2_size, m2);        
                    break;
                default:
                    rs_sort(m2_size, m2);
                    break;
            }
            progress += 50;

            min_value = min_positive(m2_size, m2);
            if (min_value == 0.0) {
                printf("Failed to find minimal value :(\n");
                exit(100);
            }

            x = reduce(m2_size, m2, min_value);
            progress += 10;
            end = omp_get_wtime();
            printf("%ld;%f\n", (long) ((end - start) * USEC_IN_SECOND), x);
            free(m1);
            free(m2);
        }
    }
    return x;
}

double *generate_m1(int n) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    #pragma omp parallel for default(none) private(i) shared(array,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double val;
        unsigned int seed;
        seed = i * i;
        val = (double) rand_r(&seed);
        val /= (double) RAND_MAX;
        val *= (A - 1.0);
        val += 1.0;
        array[i] = val;
    }
    return array;
}

void map_m1(int n, double *m1) {
    int i;
    #pragma omp parallel for default(none) private(i) shared(m1,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m1[i] = sqrt(m1[i] / M_E);
    }
}

double *generate_m2(int n) {
    int i;
    double *array;

    array = malloc(sizeof(double) * n);
    #pragma omp parallel for default(none) private(i) shared(array,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double val;
        unsigned int seed;
        seed = i * i * i;
        val = (double) rand_r(&seed);
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
    #pragma omp parallel for default(none) private(i) shared(temp,m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        temp[i] = m2[i];
    }

    zip_sum(m2 + 1, temp, n - 1);

    #pragma omp parallel for default(none) private(i) shared(m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m2[i] = fabs(tan(m2[i]));
    }
    free(temp);
}

void zip_sum(double *left, double *right, int len) {
    int i;
    #pragma omp parallel for default(none) private(i) shared(left,right,len) schedule(runtime)
    for (i = 0; i < len; ++i) {
        left[i] += right[i];
    }
}

void merge(int n, double *m1, double *m2) {
    int i;
    #pragma omp parallel for default(none) private(i) shared(m1,m2,n) schedule(runtime)
    for (i = 0; i < n; ++i) {
        m2[i] *= m1[i];
    }
}

void rs_sort2(int n, double *array) {
    int chunk_size;
    int a, b;
    int temp_size;
    double *temp;
    int i;

    chunk_size = (n + 1) / 2;
    #pragma omp parallel sections default(none) shared(array, chunk_size)
    {
        #pragma omp section
        {
            sort(chunk_size, array);
        }
        #pragma omp section
        {
            sort(chunk_size, array + chunk_size);
        }
    }

    temp = malloc(sizeof(double) * n);
    for (temp_size = 0, a = 0, b = chunk_size; a < n && a < chunk_size && b < n;) {
        if (array[a] < array[b]) {
            temp[temp_size++] = array[a++];
        } else {
            temp[temp_size++] = array[b++];
        }
    }
    while (a < n && a < chunk_size) {
        temp[temp_size++] = array[a++];
    }
    while (b < n) {
        temp[temp_size++] = array[b++];
    }
    for (i = 0; i < n; ++i) {
        array[i] = temp[i];
    }
    free(temp);
}

void rs_sortK(int n, double *array) {
    int k;
    int chunk_size;
    int c;
    int a, b;
    int temp_size;
    double *temp;
    int i;

    k = omp_get_num_procs();
    chunk_size = (n + k - 1) / k;

    #pragma omp parallel for default(none) private(c) shared(array,chunk_size,n) schedule(static,1) num_threads(k)
    for (c = 0; c < n; c += chunk_size) {
        int subsize;
        subsize = n - c;
        subsize = subsize < chunk_size ? subsize : chunk_size;
        sort(subsize, array + c);
    }


    temp = malloc(sizeof(double) * n);
    for (; chunk_size < n; chunk_size *= 2) {
        int offset;

        #pragma omp parallel for default(none) private(offset) shared(array,chunk_size,n,temp) schedule(runtime)
        for (offset = 0; offset < n; offset += 2 * chunk_size) {
            int temp_size;
            int i, j, k;
            int chunk_1, chunk_2;

            chunk_1 = offset + chunk_size;
            chunk_2 = chunk_1 + chunk_size;

            temp_size = offset;
            for (j = offset, k = chunk_1; j < n && k < n && j < chunk_1 && k < chunk_2;) {
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
            for (i = offset; i < temp_size; ++i) {
                array[i] = temp[i];
            }
        }
    }
    free(temp);
}

void rs_sort(int n, double *array) {
    int chunks;
    int chunk_size;
    int chunk_n;
    double *temp;

    chunk_size = CHUNK_SIZE;
    chunks = (n + chunk_size - 1) / chunk_size;

    #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n) schedule(runtime)
    for (chunk_n = 0; chunk_n < chunks; ++chunk_n) {
        int subsize;
        subsize = n - chunk_size * chunk_n;
        subsize = subsize < chunk_size ? subsize : chunk_size;
        sort(subsize, array + chunk_n * chunk_size);
    }

    temp = malloc(sizeof(double) * n);
    for (; chunk_size < n; chunk_size *= 2) {
        chunks = (n + chunk_size - 1) / chunk_size;
        #pragma omp parallel for default(none) private(chunk_n) shared(array,chunk_size,chunks,n,temp) schedule(runtime)
        for (chunk_n = 0; chunk_n < chunks; chunk_n += 2) {
            int temp_size;
            int i, j, k;
            int chunk_0, chunk_1, chunk_2;

            chunk_0 = chunk_n * chunk_size;
            chunk_1 = chunk_0 + chunk_size;
            chunk_2 = chunk_1 + chunk_size;

            temp_size = chunk_0;
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
            for (i = chunk_0; i < temp_size; ++i) {
                array[i] = temp[i];
            }
        }
    }
    free(temp);
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
    #pragma omp parallel for default(none) private(i) shared(array,min,n) reduction(+:sum) schedule(runtime)
    for (i = 0; i < n; ++i) {
        double value;

        value = array[i] / min;
        if (!(((int) value) % 2)) {
            sum += sin(value);
        }
    }
    return sum;
}
