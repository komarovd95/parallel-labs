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
    cc -xO3 -xautopar -xloopinfo main.c -lm -o build/lab$1-cc4.out
}

icc-par-compile() {
    icc main.c -O3 -lm -fp-model precise -parallel -qopt-report-phase=par -par-threshold$2 -o build/lab$1-icc$2.out
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
    cc-par-compile $1 && lab-exp $2 $3 $1 cc4
}

icc-par() {
    icc-par-compile $1 $2 && lab-exp $3 $4 $1 icc$2
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
    lab=1
    lab-clean
    gcc-seq $lab $1 $2
    gcc-par $lab 2 $1 $2
    gcc-par $lab 4 $1 $2
    gcc-par $lab 10 $1 $2
    gcc-par $lab 100 $1 $2
    cc-seq $lab $1 $2
    cc-par $lab $1 $2
    icc-seq $lab $1 $2
    icc-par $lab 2 $1 $2
    icc-par $lab 4 $1 $2
    icc-par $lab 10 $1 $2
    icc-par $lab 100 $1 $2
}
