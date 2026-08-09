/* -*- C++ -*-
 *
 * Test for GlobalOptions toString() method
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#include "somtparser/frontend/parser.h"
#include "test_helpers.h"
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
    parser2->getOptions()->setOption("expand_recursive_functions", "true");
    parser2->getOptions()->setLogic("QF_BV");
    std::string opts2 = parser2->optionToString();
    std::cout << opts2 << std::endl;
    assert(opts2.find("QF_BV") != std::string::npos);

    std::cout << "\n\n=== Test 3b: setOption with a string literal ===" << std::endl;
    // Regression: a string literal used to bind to the bool overload
    // (pointer-to-bool is a standard conversion, which outranks the
    // user-defined conversion to std::string), so passing "false" turned the
    // option ON. Every spelling below must mean the same thing.
    // VERIFY, not assert: this file is built with -DNDEBUG in Release, where
    // assert() compiles away entirely.
    {
        auto p = newParser();
        p->setOption("keep_let", "false");
        VERIFY(p->getOptions()->getKeepLet() == false);
        p->setOption("keep_let", "true");
        VERIFY(p->getOptions()->getKeepLet() == true);
        p->setOption("keep_let", std::string("false"));
        VERIFY(p->getOptions()->getKeepLet() == false);
        p->setOption("keep_let", true);   // genuine bool still works
        VERIFY(p->getOptions()->getKeepLet() == true);
        p->setOption("keep_let", false);
        VERIFY(p->getOptions()->getKeepLet() == false);
        // Non-boolean options must still see the literal as its text.
        p->setOption("precision", "512");
        VERIFY(p->getOptions()->getEvaluatePrecision() == 512);
        std::cout << "setOption string-literal overload OK" << std::endl;
    }

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

    // strict SMT-LIB FP surface vs lenient dialect (no error: lines on stdout)
    std::cout << "\n\n=== Test 5: strict_smtlib / lenient FP surface ===" << std::endl;
    {
        auto p = newParser();
        p->setStrictSmtlib(false);
        auto e = p->mkExpr("(fp.sqrt ((_ to_fp 8 24) RNE 25.0))");
        assert(e && !e->isErr() && e->isCFP());
    }
    {
        auto p = newParser();
        p->setStrictSmtlib(true);
        auto e = p->mkExpr("(fp.sqrt RNE ((_ to_fp 8 24) RNE 25.0))");
        assert(e && !e->isErr() && e->isCFP());
    }
    {
        auto p = newParser();
        p->setStrictSmtlib(true);
        auto e = p->mkExpr("(fp.eq ((_ to_fp 8 24) RNE 1.0) ((_ to_fp 8 24) RNE 1.0))");
        assert(e && !e->isErr() && e->isTrue());
    }

    return 0;
}

