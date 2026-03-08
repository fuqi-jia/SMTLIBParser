/* -*- C++ -*-
 *
 * Test for GlobalOptions toString() method
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#include "somtparser/frontend/parser.h"
#include <iostream>
#include <cassert>

using namespace SOMTParser;

int main() {
    auto parser = newParser();
    assert(parser);

    std::cout << "=== Test 1: Default Configuration ===" << std::endl;
    std::string defaultOpts = parser->optionToString();
    std::cout << defaultOpts << std::endl;
    assert(!defaultOpts.empty());

    std::cout << "\n\n=== Test 2: Modified Configuration ===" << std::endl;
    parser->getOptions()->setLogic("QF_LIA");
    parser->getOptions()->setEvaluatePrecision(256);
    parser->getOptions()->setKeepLet(false);
    parser->getOptions()->setKeepDivision(false);
    parser->getOptions()->setEvaluateUseFloating(false);
    parser->getOptions()->setExpandRecursiveFunctions(true);
    std::string modOpts = parser->optionToString();
    std::cout << modOpts << std::endl;
    assert(modOpts.find("QF_LIA") != std::string::npos);

    std::cout << "\n\n=== Test 3: Using setOption method ===" << std::endl;
    auto parser2 = newParser();
    parser2->getOptions()->setOption("keep_let", "false");
    parser2->getOptions()->setOption("precision", "512");
    parser2->getOptions()->setOption("float_evaluate", "false");
    parser2->getOptions()->setOption("keep_division", "false");
    parser2->getOptions()->setOption("expand_recursive_functions", "true");
    parser2->getOptions()->setLogic("QF_BV");
    std::string opts2 = parser2->optionToString();
    std::cout << opts2 << std::endl;
    assert(opts2.find("QF_BV") != std::string::npos);

    std::cout << "\n\n=== Test 4: Parsing file with options ===" << std::endl;
    auto parser3 = newParser();
    assert(parser3->parseStr("(set-logic QF_UFLIA)"));
    assert(parser3->parseStr("(set-info :source \"Test source\")"));
    assert(parser3->parseStr("(set-info :smt-lib-version \"2.6\")"));
    assert(parser3->parseStr("(set-option :produce-models true)"));
    assert(parser3->parseStr("(declare-const x Int)"));
    assert(parser3->parseStr("(assert (> x 0))"));
    assert(parser3->parseStr("(check-sat)"));
    std::string opts3 = parser3->optionToString();
    std::cout << opts3 << std::endl;
    assert(!opts3.empty());
    assert(parser3->getAssertions().size() == 1);

    return 0;
}

