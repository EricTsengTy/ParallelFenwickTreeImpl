#!/bin/bash

# Set script to stop if any command fails
set -e

echo "Running Fenwick Tree benchmark with centralized scheduler..."

# Add 1 since central scheduler only distributes task
./fenwick -t central_scheduler -p 2 -s 4095 -b 262144 -n 100
./fenwick -t central_scheduler -p 3 -s 4095 -b 262144 -n 100
./fenwick -t central_scheduler -p 5 -s 4095 -b 262144 -n 100
./fenwick -t central_scheduler -p 8 -s 4095 -b 262144 -n 100

./fenwick -t central_scheduler -p 2 -s 2097151 -b 262144 -n 100
./fenwick -t central_scheduler -p 3 -s 2097151 -b 262144 -n 100
./fenwick -t central_scheduler -p 5 -s 2097151 -b 262144 -n 100
./fenwick -t central_scheduler -p 8 -s 2097151 -b 262144 -n 100

./fenwick -t central_scheduler -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t central_scheduler -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t central_scheduler -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t central_scheduler -p 8 -s 16777215 -b 262144 -n 100

echo "Running Fenwick Tree benchmark with lockfree_scheduler scheduler..."

# Add 1 since lockfree scheduler only distributes task
./fenwick -t lockfree_scheduler -p 2 -s 4095 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 3 -s 4095 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 5 -s 4095 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 8 -s 4095 -b 262144 -n 100

./fenwick -t lockfree_scheduler -p 2 -s 2097151 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 3 -s 2097151 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 5 -s 2097151 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 8 -s 2097151 -b 262144 -n 100

./fenwick -t lockfree_scheduler -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t lockfree_scheduler -p 8 -s 16777215 -b 262144 -n 100

echo "Running Fenwick Tree benchmark with lockfree_scheduler scheduler..."

# Add 1 since lockfree scheduler only distributes task
./fenwick -t pure_parallel -p 2 -s 4095 -b 262144 -n 100
./fenwick -t pure_parallel -p 3 -s 4095 -b 262144 -n 100
./fenwick -t pure_parallel -p 5 -s 4095 -b 262144 -n 100
./fenwick -t pure_parallel -p 8 -s 4095 -b 262144 -n 100

./fenwick -t pure_parallel -p 2 -s 2097151 -b 262144 -n 100
./fenwick -t pure_parallel -p 3 -s 2097151 -b 262144 -n 100
./fenwick -t pure_parallel -p 5 -s 2097151 -b 262144 -n 100
./fenwick -t pure_parallel -p 8 -s 2097151 -b 262144 -n 100

./fenwick -t pure_parallel -p 2 -s 16777215 -b 262144 -n 100
./fenwick -t pure_parallel -p 3 -s 16777215 -b 262144 -n 100
./fenwick -t pure_parallel -p 5 -s 16777215 -b 262144 -n 100
./fenwick -t pure_parallel -p 8 -s 16777215 -b 262144 -n 100

## Test Query Percentage
echo "Testing query percentage"

./fenwick -t query_percentage_lazy -p 8 -s 16777215 -b 262144 -n 100
./fenwick -t query_percentage_pure -p 8 -s 16777215 -b 262144 -n 100
echo "Run complete."