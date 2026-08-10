#!/bin/bash

# Run all ModernPDE tests including Phase 8

set -e

echo ""
echo "==============================================================================="
echo "  ModernPDE - Full Test Suite (Phases 1-8)"
echo "==============================================================================="
echo ""

cd build

echo "[1/3] Running Phase 8 Unit Tests..."
echo ""
./tests/phase8_test

echo ""
echo "[2/3] Running Phase 8 Full Pipeline..."
echo ""
./ModernPDE_Phase8

echo ""
echo "[3/3] Running CTest (All Phases)..."
echo ""
ctest --output-on-failure

echo ""
echo "==============================================================================="
echo "  ✓ All tests passed (Phases 1-8)"
echo "==============================================================================="
echo ""
