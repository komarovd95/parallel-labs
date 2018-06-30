#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

omp-compile() {
    gcc main.c -O3 -fopenmp -lm -o build/lab$1-omp.out
}

ocl-compile() {
    gcc main.c -O3 -DUSE_OPENCL -fopenmp -lOpenCL -lm -o build/lab$1-ocl.out
}

omp-run() {
    export OMP_NUM_THREADS=$4
    export OMP_SCHEDULE="$5, $6"
    export OMP_NESTED=TRUE
    omp-compile $1 && lab-exp $2 $3 $1 omp
}

ocl-run() {
    ocl-compile $1 && lab-exp $2 $3 $1 ocl
}

lab-exp() {
    # set -e
    diff=$(( $2 - $1 ))
    step=$(( $diff / 10 ))
    counter=$1
    while [ $counter -le $2 ]; do
        echo $4 $counter
        ./build/lab$3-$4.out $counter 47 > build/lab$3-$4-$counter.txt
        counter=$(( $counter + $step ))
    done
}

do-lab() {
    lab=6
    lab-clean

    omp-run $lab $1 $2 4 guided 1
    ocl-run $lab $1 $2
}
