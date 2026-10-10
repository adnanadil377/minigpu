#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"

echo "=========================================="
echo "    MiniGPU Test Suite Runner"
echo "=========================================="

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[1/3] Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Debug

echo "[2/3] Building compiler and test binaries..."
cmake --build . --parallel

echo "[3/3] Running automated CTest suite..."
ctest --output-on-failure

echo "=========================================="
echo "All MiniGPU unit and integration tests passed!"
echo "=========================================="
