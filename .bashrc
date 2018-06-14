#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

omp-compile() {
    gcc main.c -O3 -fopenmp -lm -o build/lab$1-gcc-omp.out
}

gcc-compile() {
    gcc main.c -O3 -lm -o build/lab$1-gcc.out
}

omp-run() {
    export OMP_NUM_THREADS=$3
    export OMP_SCHEDULE="$5, $6"
    export LAB_SORT_TYPE=$4
    omp-compile 4 && lab-exp $1 $2 4 gcc-omp gcc-omp-$3-$5-$6-$4
}

gcc-run() {
    export LAB_SORT_TYPE=$3
    gcc-compile 4 && lab-exp $1 $2 4 gcc gcc-$3
}


lab-exp() {
    # set -e
    diff=$(( $2 - $1 ))
    step=$(( $diff / 10 ))
    counter=$1
    while [ $counter -le $2 ]; do
        echo $4 $counter
        ./build/lab$3-$4.out $counter 47 > build/lab$3-$5-$counter.txt
        counter=$(( $counter + $step ))
    done
}

do-lab() {
    lab=4
    lab-clean

    # single threaded
    gcc-run $1 $2 0
    gcc-run $1 $2 1
    gcc-run $1 $2 2

    # dynamic 1
    omp-run $1 $2 2 0 dynamic 1
    omp-run $1 $2 2 1 dynamic 1
    omp-run $1 $2 2 2 dynamic 1

    omp-run $1 $2 4 0 dynamic 1
    omp-run $1 $2 4 1 dynamic 1
    omp-run $1 $2 4 2 dynamic 1

    omp-run $1 $2 10 0 dynamic 1
    omp-run $1 $2 10 1 dynamic 1
    omp-run $1 $2 10 2 dynamic 1

    omp-run $1 $2 100 0 dynamic 1
    omp-run $1 $2 100 1 dynamic 1
    omp-run $1 $2 100 2 dynamic 1
}
