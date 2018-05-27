#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "fwBase.h"
// #include "fwImage.h"
#include "fwSignal.h"

#define EPS 1e-6
#define NUMBER_OF_TESTS 20
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (1; A).
 *
 * @param n size of generated array
 * @param seed random generator´s seed
 * @return random array
 */
Fw64f *generate_m1(int n, unsigned int *seed);

/**
 * Maps values of given array: m1[i] = sqrt(m1[i] / E)
 *
 * @param n size of given array
 * @param m1 an array
 */
void map_m1(int n, Fw64f *m1);

/**
 * Generates a random array of given size and seed.
 * An array has even distribution between (A; 10A).
 *
 * @param n size of generated array
 * @param seed random generator's seed
 * @return random array
 */
Fw64f *generate_m2(int n, unsigned int *seed);

/**
 * Maps values of given array: m2[i] = abs(tan(m2[i] + m2[i-1]))
 *
 * @param n size of given array
 * @param m2 an array
 */
void map_m2(int n, Fw64f *m2);

/**
 * Merges values of given arrays into M2 array.
 * Applies function to values while merging: f(a, b) = a * b
 *
 * @param n size of M2 array
 * @param m1 a M1 array
 * @param m2 a M2 array
 */
void merge(int n, Fw64f *m1, Fw64f *m2);

/**
 * Sorts given array using Insertion Sort algorithm.
 *
 * @see https://en.wikipedia.org/wiki/Insertion_sort
 * @param n size of given array
 * @param array an array
 */
void sort(int n, Fw64f *array);

/**
 * Finds minimal positive element of given array.
 *
 * @param n size of given array
 * @param array sorted array
 * @return minimal positive element or 0.0 if no positive elements found
 */
Fw64f min_positive(int n, Fw64f *array);

/**
 * Reduces given array into single double value by function:
 * f(a[i]) = f(a[i-1]) + ([a / min] % 2 == 0 ? sin(a / min) : 0.0)
 *
 * @param n size of given array
 * @param array an array
 * @param min minimal value for dividing, must be positive
 * @return reduced sum
 */
Fw64f reduce(int n, Fw64f *array, Fw64f min);

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
Fw64f do_work(int n, unsigned int seed);

int main(int argc, char* argv[]) {
    int n;
    unsigned int seed;
    unsigned int m;
    int i;
    long int best_time;
    Fw64f expected_result;

    n = atoi(argv[1]);
    seed = (unsigned int) atoi(argv[2]);
    m = (unsigned int) atoi(argv[3]);

    fwSetNumThreads(m);
    best_time = MAX_EXECUTION_TIME;
    for (i = 0; i < NUMBER_OF_TESTS; ++i) {
        struct timeval start, end;
        long int elapsed_time;
        Fw64f work_result;

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

Fw64f do_work(int n, unsigned int seed) {
    Fw64f *m1, *m2;
    int m1_size, m2_size;
    Fw64f min_value;

    m1_size = n;
    m2_size = n / 2;

    m1 = generate_m1(m1_size, &seed);
    m2 = generate_m2(m2_size, &seed);

    map_m1(m1_size, m1);
    map_m2(m2_size, m2);

    merge(m2_size, m1, m2);

    sort(m2_size, m2);

    min_value = min_positive(m2_size, m2);
    if (min_value == 0.0) {
        printf("Failed to find minimal value :(\n");
        exit(100);
    }

    return reduce(m2_size, m2, min_value);
}

Fw64f *generate_m1(int n, unsigned int *seed) {
    int i;
    Fw64f *array;

    array = fwsMalloc_64f(n);
    for (i = 0; i < n; ++i) {
        array[i] = (Fw64f) rand_r(seed);
    }
    fwsDivC_64f_I((Fw64f) RAND_MAX, array, n);
    fwsMulC_64f_I((Fw64f) (A - 1.0), array, n);
    fwsAddC_64f_I((Fw64f) 1.0, array, n);
    return array;
}

void map_m1(int n, Fw64f *m1) {
    fwsDivC_64f_I((Fw64f) M_E, m1, n);
    fwsSqrt_64f_I(m1, n);
}

Fw64f *generate_m2(int n, unsigned int *seed) {
    int i;
    Fw64f *array;

    array = fwsMalloc_64f(n);
    for (i = 0; i < n; ++i) {
        array[i] = (Fw64f) rand_r(seed);
    }
    fwsDivC_64f_I((Fw64f) RAND_MAX, array, n);
    fwsMulC_64f_I((Fw64f) (9.0 * A), array, n);
    fwsAddC_64f_I((Fw64f) A, array, n);
    return array;
}

void map_m2(int n, Fw64f *m2) {
    Fw64f *temp;

    temp = fwsMalloc_64f(n);
    fwsMove_64f(m2, temp, n);
    fwsAdd_64f_I(m2, temp + 1, n - 1);
    fwsTan_64f_A53(temp, m2, n);
    fwsAbs_64f_I(m2, n);
    fwsFree(temp);
}

void merge(int n, Fw64f *m1, Fw64f *m2) {
    fwsMul_64f_I(m1, m2, n);
}

void sort(int n, Fw64f *array) {
    int i, j;
    Fw64f key;

    for (i = 1; i < n; ++i) {
        key = array[i];
        for (j = i - 1; j >= 0 && array[j] > key; --j) {
            array[j + 1] = array[j];
        }
        array[j + 1] = key;
    }
}

Fw64f min_positive(int n, Fw64f *array) {
    int i;
    for (i = 0; i < n; ++i) {
        if (array[i] != 0.0) {
            return array[i];
        }
    }
    return (Fw64f) 0.0;
}

Fw64f reduce(int n, Fw64f *array, Fw64f min) {
    int i;
    int m;
    Fw64f *temp;
    Fw64f sum;

    temp = fwsMalloc_64f(n);
    fwsDivC_64f_I(min, array, n);
    m = 0;
    for (i = 0; i < n; ++i) {
        Fw64f value;

        value = array[i];
        if (!(((int) value) % 2)) {
            temp[m++] = value;
        }
    }

    fwsSin_64f_A53(temp, temp, m);

    sum = 0.0;
    for (i = 0; i < m; ++i) {
        sum += temp[i];
    }
    fwsFree(temp);
    return sum;
}

void print_array(Fw64f *arr, int n) {
    int i;
    for (i = 0; i < n; i += 100) {
        printf("%f\n", arr[i]);
    }
}
