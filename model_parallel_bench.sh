#!/usr/bin/env bash

# Set script to stop if any command fails
set -e

# Display each command line
set -x

# Compile
make clean
make

echo "Running Fenwick Tree benchmark with pipeline-fixed-size..."
./fenwick -t pipeline-fixed-size -p 2 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-fixed-size -p 3 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-fixed-size -p 5 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-fixed-size -p 8 -s 4095 -b 262144 -n 1000

./fenwick -t pipeline-fixed-size -p 2 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-fixed-size -p 3 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-fixed-size -p 5 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-fixed-size -p 8 -s 2097151 -b 262144 -n 400

./fenwick -t pipeline-fixed-size -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-fixed-size -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-fixed-size -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-fixed-size -p 8 -s 16777215 -b 262144 -n 100

echo "Running Fenwick Tree benchmark with pipeline-access-aware..."
./fenwick -t pipeline-access-aware -p 2 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-access-aware -p 3 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-access-aware -p 5 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-access-aware -p 8 -s 4095 -b 262144 -n 1000

./fenwick -t pipeline-access-aware -p 2 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-access-aware -p 3 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-access-aware -p 5 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-access-aware -p 8 -s 2097151 -b 262144 -n 400

./fenwick -t pipeline-access-aware -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-access-aware -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-access-aware -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-access-aware -p 8 -s 16777215 -b 262144 -n 100

echo "Running Fenwick Tree benchmark with pipeline-semi-static..."
./fenwick -t pipeline-semi-static -p 2 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-semi-static -p 3 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-semi-static -p 5 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-semi-static -p 8 -s 4095 -b 262144 -n 1000

./fenwick -t pipeline-semi-static -p 2 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-semi-static -p 3 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-semi-static -p 5 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-semi-static -p 8 -s 2097151 -b 262144 -n 400

./fenwick -t pipeline-semi-static -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-semi-static -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-semi-static -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-semi-static -p 8 -s 16777215 -b 262144 -n 100

echo "Running Fenwick Tree benchmark with pipeline-aggregate..."
./fenwick -t pipeline-aggregate -p 2 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-aggregate -p 3 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-aggregate -p 5 -s 4095 -b 262144 -n 1000
./fenwick -t pipeline-aggregate -p 8 -s 4095 -b 262144 -n 1000

./fenwick -t pipeline-aggregate -p 2 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-aggregate -p 3 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-aggregate -p 5 -s 2097151 -b 262144 -n 400
./fenwick -t pipeline-aggregate -p 8 -s 2097151 -b 262144 -n 400

./fenwick -t pipeline-aggregate -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-aggregate -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-aggregate -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t pipeline-aggregate -p 8 -s 16777215 -b 262144 -n 100

echo "Run complete."