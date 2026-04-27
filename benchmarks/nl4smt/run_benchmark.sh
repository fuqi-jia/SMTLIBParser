#!/bin/bash
# NL4SMT Benchmark Runner
#
# Usage:
#   ./run_benchmark.sh          # Run tests + solver validation
#   ./run_benchmark.sh --solve  # Also invoke solver on each .smt2 and show models
#
# This script keeps solver invocation OUTSIDE the parser library.

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SMT2_DIR="$BUILD_DIR/benchmarks/smt2"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== NL4SMT Benchmark Runner ==="
echo "Project root: $PROJECT_ROOT"
echo ""

# Build tests
echo "[1/4] Building tests..."
cd "$PROJECT_ROOT"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DBUILD_TESTS=ON -DBUILD_BOTH_LIBS=ON > /dev/null 2>&1
make -j$(nproc) test_benchmark_nl4smt > /dev/null 2>&1

echo "[2/4] Generating .smt2 files..."
mkdir -p "$SMT2_DIR"
cd "$BUILD_DIR/test"
NL4SMT_SAVE_SMT2="$SMT2_DIR" ./test_benchmark_nl4smt

echo ""

# Optional: invoke solver on each .smt2
if [[ "$1" == "--solve" ]]; then
    echo "[3/4] Invoking solver on each .smt2..."

    if ! command -v z3 &> /dev/null; then
        echo "z3 not found in PATH. Skipping solver invocation."
        exit 0
    fi

    solved=0
    failed=0
    total=0

    for smt2_file in "$SMT2_DIR"/*.smt2; do
        [[ -f "$smt2_file" ]] || continue
        total=$((total + 1))
        name=$(basename "$smt2_file" .smt2)

        # Append (get-model) before (exit) for sat problems
        tmp_file="/tmp/nl4smt_solve_${name}.smt2"
        sed 's/(exit)/(get-model)\n(exit)/' "$smt2_file" > "$tmp_file"

        result=$(z3 -smt2 "$tmp_file" 2>&1 || true)
        rm -f "$tmp_file"

        if echo "$result" | grep -q "^sat$"; then
            echo -e "${GREEN}[sat]${NC}   $name"
            solved=$((solved + 1))
        elif echo "$result" | grep -q "^unsat$"; then
            echo -e "${YELLOW}[unsat]${NC} $name"
            solved=$((solved + 1))
        else
            echo -e "${RED}[err]${NC}   $name"
            failed=$((failed + 1))
        fi
    done

    echo ""
    echo "[4/4] Solver summary: $solved/$total solved, $failed failed"
else
    echo "[3/4] Skipping solver invocation (use --solve to run solver)"
    echo "[4/4] .smt2 files saved to: $SMT2_DIR"
fi

echo ""
echo "=== Done ==="
