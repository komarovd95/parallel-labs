#!/bin/bash

lab-clean() {
    rm -rf build && mkdir build;
}

create-links() {
    CUR_PWD=${PWD}
    cd ./fw/$1/lib
    ln -sf ./libfwBase.so.1.3.1 libfwBase.so
    ln -sf ./libfwBase.so.1.3.1 libfwBase.so.1
    ln -sf ./libfwImage.so.1.3.1 libfwImage.so
    ln -sf ./libfwImage.so.1.3.1 libfwImage.so.1
    ln -sf ./libfwSignal.so.1.3.1 libfwSignal.so
    ln -sf ./libfwSignal.so.1.3.1 libfwSignal.so.1
    cd $CUR_PWD
}

build-framewave() {
    version=FW_1.3.1_Lin64
    if [ ! -d "$PWD/fw" ]; then
        mkdir fw
        wget -O ./fw/framewave.tar.gz https://sourceforge.net/projects/framewave/files/framewave-releases/Framewave%201.3.1/$version.tar.gz/download
        tar -xvzf ./fw/framewave.tar.gz -C ./fw
        create-links $version
        export LD_LIBRARY_PATH=$PWD/fw/$version/lib:$LD_LIBRARY_PATH
    fi
    fw-compile 2 $version && lab-exp $1 $2 2 gcc-fw $3
}

fw-compile() {
    gcc -m64 main.c -O3 -Ifw/$2 -Lfw/$2/lib -lm -lfwBase -lfwImage -lfwSignal -o build/lab$1-gcc-fw.out
}

do-lab() {
    lab-clean
    build-framewave $1 $2 1
    build-framewave $1 $2 2
    build-framewave $1 $2 3
    build-framewave $1 $2 4
    build-framewave $1 $2 10
    build-framewave $1 $2 100
}

lab-exp() {
    # set -e
    diff=$(( $2 - $1 ))
    step=$(( $diff / 10 ))
    counter=$1
    while [ $counter -le $2 ]; do
        echo $4 $counter
        ./build/lab$3-$4.out $counter 47 $5 > build/lab$3-$4-$5-$counter.txt
        counter=$(( $counter + $step ))
    done
}
