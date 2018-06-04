#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

omp-compile() {
    gcc main.c -O3 -fopenmp -lm -o build/lab$1-gcc-omp.out
}

omp-run() {
    export OMP_NUM_THREADS=$3
    export OMP_SCHEDULE="$4, $5"
    echo $OMP_NUM_THREADS
    echo $OMP_SCHEDULE
    omp-compile 3 && lab-exp $1 $2 3 gcc-omp-$3-$4-$5
}


lab-exp() {
    # set -e
    diff=$(( $2 - $1 ))
    step=$(( $diff / 10 ))
    counter=$1
    while [ $counter -le $2 ]; do
        echo $4 $counter
        ./build/lab$3-gcc-omp.out $counter 47 > build/lab$3-$4-$counter.txt
        counter=$(( $counter + $step ))
    done
}

do-lab() {
    lab=3
    lab-clean
    omp-run $1 $2 2 static 1
    omp-run $1 $2 2 static 2
    omp-run $1 $2 2 static 8
    omp-run $1 $2 2 static 64
    omp-run $1 $2 2 static 1024
    omp-run $1 $2 4 static 1
    omp-run $1 $2 4 static 2
    omp-run $1 $2 4 static 8
    omp-run $1 $2 4 static 64
    omp-run $1 $2 4 static 1024
    omp-run $1 $2 10 static 1
    omp-run $1 $2 10 static 2
    omp-run $1 $2 10 static 8
    omp-run $1 $2 10 static 64
    omp-run $1 $2 10 static 1024
    omp-run $1 $2 100 static 1
    omp-run $1 $2 100 static 2
    omp-run $1 $2 100 static 8
    omp-run $1 $2 100 static 64
    omp-run $1 $2 100 static 1024

    omp-run $1 $2 2 dynamic 1
    omp-run $1 $2 2 dynamic 2
    omp-run $1 $2 2 dynamic 8
    omp-run $1 $2 2 dynamic 64
    omp-run $1 $2 2 dynamic 1024
    omp-run $1 $2 4 dynamic 1
    omp-run $1 $2 4 dynamic 2
    omp-run $1 $2 4 dynamic 8
    omp-run $1 $2 4 dynamic 64
    omp-run $1 $2 4 dynamic 1024
    omp-run $1 $2 10 dynamic 1
    omp-run $1 $2 10 dynamic 2
    omp-run $1 $2 10 dynamic 8
    omp-run $1 $2 10 dynamic 64
    omp-run $1 $2 10 dynamic 1024
    omp-run $1 $2 100 dynamic 1
    omp-run $1 $2 100 dynamic 2
    omp-run $1 $2 100 dynamic 8
    omp-run $1 $2 100 dynamic 64
    omp-run $1 $2 100 dynamic 1024

    omp-run $1 $2 2 guided 1
    omp-run $1 $2 2 guided 2
    omp-run $1 $2 2 guided 8
    omp-run $1 $2 2 guided 64
    omp-run $1 $2 2 guided 1024
    omp-run $1 $2 4 guided 1
    omp-run $1 $2 4 guided 2
    omp-run $1 $2 4 guided 8
    omp-run $1 $2 4 guided 64
    omp-run $1 $2 4 guided 1024
    omp-run $1 $2 10 guided 1
    omp-run $1 $2 10 guided 2
    omp-run $1 $2 10 guided 8
    omp-run $1 $2 10 guided 64
    omp-run $1 $2 10 guided 1024
    omp-run $1 $2 100 guided 1
    omp-run $1 $2 100 guided 2
    omp-run $1 $2 100 guided 8
    omp-run $1 $2 100 guided 64
    omp-run $1 $2 100 guided 1024
}
