#!/usr/bin/env bash
# build.sh — convenience script to configure, build, and optionally test FSSS

set -e

BUILD_DIR="build"
BUILD_TYPE="${1:-Release}"
RUN_TESTS="${2:-false}"

echo "═══════════════════════════════════════════════"
echo "  FSSS — Build Script"
echo "  Build type : $BUILD_TYPE"
echo "═══════════════════════════════════════════════"

mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DFSSS_BUILD_TESTS="$([ "$RUN_TESTS" = "true" ] && echo ON || echo OFF)"

cmake --build "$BUILD_DIR" --parallel "$(nproc 2>/dev/null || echo 4)"

echo ""
echo "Build complete. Binary: $BUILD_DIR/fsss"

if [ "$RUN_TESTS" = "true" ]; then
    echo ""
    echo "Running tests..."
    cd "$BUILD_DIR" && ctest --output-on-failure
fi
