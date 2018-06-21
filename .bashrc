#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

omp-compile() {
    gcc main.c -O3 -fopenmp -lm -o build/lab$1-omp.out
}

pthreads-compile() {
    gcc threads.c -O3 -D$2 -lm -lpthread -o build/lab$1-pthread-$3.out 
}

omp-run() {
    export OMP_NUM_THREADS=$3
    export OMP_SCHEDULE="$4, $5"
    export OMP_NESTED=TRUE
    omp-compile 5 && lab-exp $1 $2 5 omp omp-$3-$4-$5
}

pthreads-run() {
    pthreads-compile 5 $6 $4 && lab-exp $1 $2 5 pthread-$4 pthread-$3-$4-$5 $3 $5
}

lab-exp() {
    # set -e
    diff=$(( $2 - $1 ))
    step=$(( $diff / 10 ))
    counter=$1
    while [ $counter -le $2 ]; do
        echo $4 $counter
        ./build/lab$3-$4.out $counter $6 $7 > build/lab$3-$5-$counter.txt
        counter=$(( $counter + $step ))
    done
}

do-lab() {
    lab=5
    # lab-clean

    # dynamic 1
    # omp-run $1 $2 2 dynamic 1
    # omp-run $1 $2 4 dynamic 1
    # omp-run $1 $2 10 dynamic 1
    # omp-run $1 $2 100 dynamic 1

    # pthreads-run $1 $2 2 dynamic 1 SCHED_DYNAMIC
    # pthreads-run $1 $2 4 dynamic 1 SCHED_DYNAMIC
    # pthreads-run $1 $2 10 dynamic 1 SCHED_DYNAMIC
    # pthreads-run $1 $2 100 dynamic 1 SCHED_DYNAMIC

    # dynamic 64
    omp-run $1 $2 2 dynamic 64
    omp-run $1 $2 4 dynamic 64
    omp-run $1 $2 10 dynamic 64
    omp-run $1 $2 100 dynamic 64

    # pthreads-run $1 $2 2 dynamic 64 SCHED_DYNAMIC
    # pthreads-run $1 $2 4 dynamic 64 SCHED_DYNAMIC
    # pthreads-run $1 $2 10 dynamic 64 SCHED_DYNAMIC
    # pthreads-run $1 $2 100 dynamic 64 SCHED_DYNAMIC

    # static 1
    # omp-run $1 $2 2 static 1
    # omp-run $1 $2 4 static 1
    # omp-run $1 $2 10 static 1
    # omp-run $1 $2 100 static 1

    # pthreads-run $1 $2 2 static 1 SCHED_STATIC
    # pthreads-run $1 $2 4 static 1 SCHED_STATIC
    # pthreads-run $1 $2 10 static 1 SCHED_STATIC
    # pthreads-run $1 $2 100 static 1 SCHED_STATIC

    # static 8
    omp-run $1 $2 2 static 8
    omp-run $1 $2 4 static 8
    omp-run $1 $2 10 static 8
    omp-run $1 $2 100 static 8

    # pthreads-run $1 $2 2 static 8 SCHED_STATIC
    # pthreads-run $1 $2 4 static 8 SCHED_STATIC
    # pthreads-run $1 $2 10 static 8 SCHED_STATIC
    # pthreads-run $1 $2 100 static 8 SCHED_STATIC

    # guided 1
    omp-run $1 $2 2 guided 1
    omp-run $1 $2 4 guided 1
    omp-run $1 $2 10 guided 1
    omp-run $1 $2 100 guided 1

    # pthreads-run $1 $2 2 guided 1 SCHED_GUIDED
    # pthreads-run $1 $2 4 guided 1 SCHED_GUIDED
    # pthreads-run $1 $2 10 guided 1 SCHED_GUIDED
    # pthreads-run $1 $2 100 guided 1 SCHED_GUIDED

    # guided 1024
    # omp-run $1 $2 2 guided 1024
    # omp-run $1 $2 4 guided 1024
    # omp-run $1 $2 10 guided 1024
    # omp-run $1 $2 100 guided 1024

    # pthreads-run $1 $2 2 guided 1024 SCHED_GUIDED
    # pthreads-run $1 $2 4 guided 1024 SCHED_GUIDED
    # pthreads-run $1 $2 10 guided 1024 SCHED_GUIDED
    # pthreads-run $1 $2 100 guided 1024 SCHED_GUIDED
}
