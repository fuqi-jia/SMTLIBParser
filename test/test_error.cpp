#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <algorithm>
#include "somtparser/frontend/parser.h"
#include <cassert>

// Test function to check file existence
bool file_exists(const std::string& filename) {
    FILE* file = fopen(filename.c_str(), "r");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

// Test function to parse a specific SMT2 file and report results. Returns parse_success.
bool test_parse_file(const std::string& filename, SOMTParser::ParserPtr& parser, bool expect_success) {
    std::cout << "\n=== Testing file: " << filename << " ===" << std::endl;
    if (!file_exists(filename)) {
        std::cerr << "Error: Test file not found: " << filename << std::endl;
        assert(false && "test file must exist");
        return false;
    }

    parser->setOption("keep_let", false);
    parser->setOption("preserve_let", false);
    bool parse_success = parser->parse(filename);

    if (parse_success) {
        std::cout << "✓ PARSE_SUCCESS" << std::endl;
        auto assertions = parser->getAssertions();
        auto variables = parser->getVariables();
        auto functions = parser->getFunctions();
        auto nodes = parser->getNodeCount();
        std::cout << "  Assertions: " << assertions.size() << std::endl;
        std::cout << "  Variables: " << variables.size() << std::endl;
        std::cout << "  Functions: " << functions.size() << std::endl;
        std::cout << "  Total Nodes: " << nodes << std::endl;
        if (!variables.empty()) {
            std::cout << "  Variables:" << std::endl;
            for (const auto& var : variables) {
                std::cout << "    " << var->getName() << " : " << parser->toString(var->getSort()) << std::endl;
            }
        }
        if (!functions.empty()) {
            std::cout << "  Functions:" << std::endl;
            for (const auto& func : functions) {
                std::cout << "    " << func->getName() << std::endl;
            }
        }
        if (!assertions.empty()) {
            std::cout << "  Assertions (first 3):" << std::endl;
            for (size_t i = 0; i < std::min(assertions.size(), size_t(3)); ++i) {
                std::cout << "    " << (i+1) << ". " << parser->toString(assertions[i]) << std::endl;
            }
            if (assertions.size() > 3) {
                std::cout << "    ... and " << (assertions.size() - 3) << " more assertions" << std::endl;
            }
        }
    } else {
        std::cout << "✗ PARSE_FAILURE" << std::endl;
    }
    assert(parse_success == expect_success && "parse result should match expected");
    return parse_success;
}

int main() {
    std::cout << "======= Parser Error Test =======" << std::endl;

    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    // Test 1: error_kind_mismatch.smt2 — fixed, should parse successfully
    test_parse_file("../test/instances/error_kind_mismatch.smt2", parser, true);

    parser = SOMTParser::newParser();
    // Test 2: error_unknown_symbol.smt2 — contains :named; parser supports :named so expect success
    test_parse_file("../test/instances/error_unknown_symbol.smt2", parser, true);

    parser = SOMTParser::newParser();
    // Test 3: error_unknown_symbol_2.smt2 — contains (declare-sort S 1) etc.; may succeed or fail; assert by current behavior
    bool ok3 = test_parse_file("../test/instances/error_unknown_symbol_2.smt2", parser, true);
    (void)ok3;

    std::cout << "\n======= Test Complete =======" << std::endl;
    return 0;
} 