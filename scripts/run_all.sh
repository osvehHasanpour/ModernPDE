#!/bin/bash

mkdir -p ../results

echo "Running ModernPDE..."

./ModernPDE | tee ../results/full_output.txt

grep "Variables:" ../results/full_output.txt > ../results/benchmark.txt

grep -E "Accuracy|Precision|Recall|F1" \
../results/full_output.txt > ../results/metrics.txt

grep "Classification" \
../results/full_output.txt > ../results/pde_result.txt

grep "Block" \
../results/full_output.txt > ../results/cfg_result.txt

grep "Def(" \
../results/full_output.txt > ../results/duchain_result.txt

grep "Path " \
../results/full_output.txt > ../results/path_result.txt

echo ""
echo "==============================="
echo "Results saved in results/"
echo "==============================="