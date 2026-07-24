#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${SOMTPARSER_BUILD_DIR:-${PROJECT_ROOT}/build}"

cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}" \
  -DSOMTPARSER_BUILD_TESTS=ON
cmake --build "${BUILD_DIR}" --parallel
ctest --test-dir "${BUILD_DIR}" --output-on-failure
