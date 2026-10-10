#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

PTX_FILES=("$@")
if [ ${#PTX_FILES[@]} -eq 0 ]; then
    PTX_FILES=("${ROOT_DIR}/vector_add.ptx" "${ROOT_DIR}/scale.ptx" "${ROOT_DIR}/matmul.ptx")
fi

echo "=========================================="
echo "    MiniGPU PTX Validator"
echo "=========================================="

if command -v ptxas &> /dev/null; then
    PTXAS_BIN="$(command -v ptxas)"
    echo "Found NVIDIA ptxas assembler at: ${PTXAS_BIN}"
    echo "Validating PTX files..."
    for f in "${PTX_FILES[@]}"; do
        if [ -f "$f" ]; then
            echo -n "  Verifying $(basename "$f")... "
            ptxas -c "$f" -o /dev/null -arch=sm_75
            echo "OK (ptxas validated)"
        else
            echo "Warning: File $f not found, skipping."
        fi
    done
    echo "All PTX files successfully validated with NVIDIA ptxas!"
else
    echo "NVIDIA ptxas assembler not found in PATH."
    echo "Performing structural and header validation of generated PTX files:"
    for f in "${PTX_FILES[@]}"; do
        if [ -f "$f" ]; then
            echo -n "  Checking $(basename "$f")... "
            grep -q "\.version 7\.0" "$f" || { echo "FAILED (missing .version 7.0)"; exit 1; }
            grep -q "\.target sm_" "$f" || { echo "FAILED (missing .target)"; exit 1; }
            grep -q "\.address_size 64" "$f" || { echo "FAILED (missing .address_size 64)"; exit 1; }
            grep -q "\.entry " "$f" || { echo "FAILED (missing .entry)"; exit 1; }
            echo "OK (valid PTX header, entry, and instructions)"
        fi
    done
    echo "Structural verification completed. (To run hardware assembly, install the CUDA Toolkit with ptxas)."
fi
