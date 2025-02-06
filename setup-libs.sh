#!/bin/bash
set -euxo pipefail

WORKDIR=$(pwd)
cd "$(dirname "$0")"

if [ ! -d "libs" ]; then
    mkdir libs
fi

if [ ! -d "include" ]; then
    mkdir include
    cd include
    if [ ! -d "libiscsi" ]; then
        mkdir libiscsi
    fi
    cd ../
fi

if [ ! -d "libiscsi" ]; then
    git clone https://github.com/sahlberg/libiscsi.git
fi

cd libiscsi

./autogen.sh
./configure --prefix=$(pwd)/build
make

cp include/* ../include/libiscsi
cp lib/.libs/libiscsi.so* ../libs/

cd ../

rm -rf libiscsi

echo "complete setup library"