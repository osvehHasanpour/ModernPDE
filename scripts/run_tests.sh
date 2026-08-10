#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/build"

echo "=============================================="
echo " ModernPDE — full test suite"
echo "=============================================="
echo "Project : ${ROOT}"
echo "Build   : ${BUILD}"
echo ""

mkdir -p "${BUILD}"
cd "${BUILD}"

echo "[1/3] Configuring..."
cmake .. -DCMAKE_CXX_FLAGS="-Wall -Wextra"

echo ""
echo "[2/3] Building ModernPDE + tests..."
cmake --build . --parallel "$(nproc 2>/dev/null || echo 2)"

echo ""
echo "[3/3] Running ctest..."
ctest --output-on-failure

echo ""
echo "=============================================="
echo " All ctest targets finished"
echo "=============================================="
