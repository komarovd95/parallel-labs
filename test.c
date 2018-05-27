#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include "fwBase.h"
#include "fwSignal.h"

#define EPS 1e-6
#define NUMBER_OF_TESTS 1
#define USEC_IN_SECOND 1000000L
#define MAX_EXECUTION_TIME 10000000000L // 10s
#define A 637.0
#define N 250000

int main(int argc, char* argv[]) {
    int i, m;
    Fw64f *array, *result;
    struct timeval start, end;
    long int elapsed_time;

    m = atoi(argv[1]);

    array = fwsMalloc_64f(N);
    result = fwsMalloc_64f(N);
    fwSetNumThreads(m);
    srand(47);

    
    gettimeofday(&start, NULL);
    for (i = 0; i < N; ++i) {
        array[i] = rand();
    }
    // fwsTan_64f_A53(array, result, N);
    // fwsTan_64f_A53(array, result, N);
    // fwsTan_64f_A53(array, result, N);
    fwsAddC_64f_I(9.0, array, N);
    fwsAddC_64f_I(8.0, array, N);
    fwsAddC_64f_I(7.0, array, N);
    gettimeofday(&end, NULL);

    elapsed_time = (end.tv_sec - start.tv_sec) * USEC_IN_SECOND;
    elapsed_time += (end.tv_usec - start.tv_usec);
    printf("%ld\n", elapsed_time);
    return 0;
}