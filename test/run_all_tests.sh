#!/bin/bash

# Set up colored output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Change to project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
rm -rf build
mkdir -p build
cd build

# Configure with CMake
echo -e "${YELLOW}Configuring project...${NC}"
cmake .. -DBUILD_TESTS=ON
if [ $? -ne 0 ]; then
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

# Compile
echo -e "${YELLOW}Building project...${NC}"
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

# Run all tests
echo -e "${YELLOW}Running tests...${NC}"
cd test

# Find all test executables (built by CMake from test/*.cpp)
TEST_EXES=$(find . -maxdepth 1 -type f -executable -name "test_*" 2>/dev/null | sort)

# If no executables found, try explicit list (e.g. when find fails or cwd is wrong)
if [ -z "$TEST_EXES" ]; then
    echo -e "${YELLOW}No test executables found in current dir, trying explicit list...${NC}"
    POTENTIAL_TESTS=(
        ./test_array_simplify
        ./test_array_theory
        ./test_arithmetic
        ./test_bitvector
        ./test_boolean_logic
        ./test_context_dispatcher
        ./test_error
        ./test_expressions
        ./test_floating_point
        ./test_node_api
        ./test_optimization
        ./test_options_config
        ./test_parse_model
        ./test_parser
        ./test_quantifiers
        ./test_readme
        ./test_rewriter
        ./test_somtparser_exe
        ./test_string_handling
        ./test_string_operations
        ./test_theory_combination
        ./test_umbrella
        ./test_visitor_api
        ./test_define_fun_assert_roundtrip
        ./test_datatype_eval
        ./test_fp_util_wrappers
        ./test_uf_model_api
        ./test_evaluate_fp_dispatch
    )
    for t in "${POTENTIAL_TESTS[@]}"; do
        [ -x "$t" ] && TEST_EXES="$TEST_EXES $t"
    done
    if [ -z "$TEST_EXES" ]; then
        echo -e "${RED}No test executables found! (Run from build/test after make)${NC}"
        exit 1
    fi
fi

# Run each test
FAILED_TESTS=0
for test in $TEST_EXES; do
    echo -e "${YELLOW}Running $test...${NC}"
    # save the output to a temporary file
    OUTPUT=$(mktemp)
    $test > $OUTPUT 2>&1
    EXIT_CODE=$?
    
    # check the exit status and if the output contains "error:"
    if [ $EXIT_CODE -eq 0 ] && ! grep -q "error:" $OUTPUT; then
        echo -e "${GREEN}$test passed!${NC}"
    else
        echo -e "${RED}$test failed!${NC}"
        # if the output contains "error:", output the error information
        if grep -q "error:" $OUTPUT; then
            echo -e "${RED}Found error information:${NC}"
            grep "error:" $OUTPUT
        fi
        FAILED_TESTS=$((FAILED_TESTS+1))
    fi
    
    # clean up the temporary file
    rm $OUTPUT
    echo ""
done

# Report test results
if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}$FAILED_TESTS tests failed!${NC}"
    exit 1
fi 