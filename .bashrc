#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

gcc-seq-compile() {
    gcc main.c -O3 -o build/lab$1-gcc1.out -lm;
}

cc-seq-compile() {
    cc -xO3 main.c -lm -o build/lab$1-cc1.out
}

icc-seq-compile() {
    icc main.c -O3 -lm -fp-model precise -o build/lab$1-icc1.out
}

gcc-par-compile() {
    gcc main.c -O3 -floop-parallelize-all -ftree-parallelize-loops=$2 -lm -o build/lab$1-gcc$2.out;
}

cc-par-compile() {
    cc -xO3 -xautopar -xloopinfo main.c -lm -o build/lab$1-cc$2.out
}

icc-par-compile() {
    icc main.c -O3 -lm -fp-model precise -parallel -qopt-report-phase=par -par-threshold$2  -o build/lab$1-icc$2.out
}

gcc-seq() {
    gcc-seq-compile $1 && lab-exp $2 $3 $1 gcc1
}

cc-seq() {
    cc-seq-compile $1 && lab-exp $2 $3 $1 cc1
}

icc-seq() {
    icc-seq-compile $1 && lab-exp $2 $3 $1 icc1
}

gcc-par() {
    gcc-par-compile $1 $2 && lab-exp $3 $4 $1 gcc$2
}

cc-par() {
    export OMP_NUM_THREADS=$2
    cc-par-compile $1 $2 && lab-exp $3 $4 $1 cc$2
}

icc-par() {
    icc-par-compile $1 $2 && lab-exp $3 $4 $1 icc$2
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
    # lab-clean
    # omp-run $1 $2 2 static 1
    # omp-run $1 $2 2 static 2
    # omp-run $1 $2 2 static 8
    # omp-run $1 $2 2 static 64
    # omp-run $1 $2 2 static 1024
    # omp-run $1 $2 4 static 1
    # omp-run $1 $2 4 static 2
    # omp-run $1 $2 4 static 8
    # omp-run $1 $2 4 static 64
    # omp-run $1 $2 4 static 1024
    # omp-run $1 $2 10 static 1
    # omp-run $1 $2 10 static 2
    # omp-run $1 $2 10 static 8
    # omp-run $1 $2 10 static 64
    # omp-run $1 $2 10 static 1024
    # omp-run $1 $2 100 static 1
    # omp-run $1 $2 100 static 2
    # omp-run $1 $2 100 static 8
    # omp-run $1 $2 100 static 64
    # omp-run $1 $2 100 static 1024

    # omp-run $1 $2 2 dynamic 1
    # omp-run $1 $2 2 dynamic 2
    # omp-run $1 $2 2 dynamic 8
    # omp-run $1 $2 2 dynamic 64
    # omp-run $1 $2 2 dynamic 1024
    # omp-run $1 $2 4 dynamic 1
    # omp-run $1 $2 4 dynamic 2
    # omp-run $1 $2 4 dynamic 8
    # omp-run $1 $2 4 dynamic 64
    # omp-run $1 $2 4 dynamic 1024
    # omp-run $1 $2 10 dynamic 1
    # omp-run $1 $2 10 dynamic 2
    # omp-run $1 $2 10 dynamic 8
    # omp-run $1 $2 10 dynamic 64
    # omp-run $1 $2 10 dynamic 1024
    # omp-run $1 $2 100 dynamic 1
    # omp-run $1 $2 100 dynamic 2
    # omp-run $1 $2 100 dynamic 8
    # omp-run $1 $2 100 dynamic 64
    # omp-run $1 $2 100 dynamic 1024

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
