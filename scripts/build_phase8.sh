#!/bin/bash

# Phase 8 Optimization Build & Test Script

set -e  # Exit on error

echo ""
echo "==============================================================================="
echo "  ModernPDE Phase 8 - Build & Test Automation"
echo "==============================================================================="
echo ""

# Create build directory
echo "[1/4] Creating build directory..."
if [ ! -d "build" ]; then
    mkdir -p build
fi

cd build

# Configure
echo "[2/4] Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -march=native -Wall -Wextra"

# Build
echo "[3/4] Building Phase 8..."
make -j$(nproc)

# Test
echo "[4/4] Running Phase 8 Tests..."
echo ""

# Run unit tests
echo "Running Phase 8 Unit Tests..."
echo "------"
./tests/phase8_test

echo ""
echo "Running Phase 8 Full Pipeline..."
echo "------"
mkdir -p ../output
./ModernPDE_Phase8 ../output/phase8_benchmark.txt

echo ""
echo "==============================================================================="
echo "  ✓ All Phase 8 tests passed successfully!"
echo "==============================================================================="
echo ""
echo "Generated artifacts:"
echo "  - Binary: ./ModernPDE_Phase8"
echo "  - Tests:  ./tests/phase8_test"
echo "  - Report: ../output/phase8_benchmark.txt"
echo ""
