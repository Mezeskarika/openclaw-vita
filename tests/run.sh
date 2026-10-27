#!/bin/sh
# Builds and runs the host unit tests in build-tests/.
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-tests"

cmake -S "$ROOT/tests" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"
"$BUILD_DIR/openclaw_tests"
