#!/usr/bin/env bash
set -euo pipefail

# Run from the repository root, or invoke this script from any directory.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-build}"

cd "$ROOT_DIR"

echo "[1/3] Configure"
cmake -S . -B "$BUILD_DIR"

echo "[2/3] Build"
cmake --build "$BUILD_DIR" --parallel

echo "[3/3] Run registered tests"
ctest --test-dir "$BUILD_DIR" --output-on-failure

echo "All configure, build, and registered test steps completed successfully."
