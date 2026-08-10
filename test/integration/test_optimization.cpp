/*
 * Test optimization / OMT: minimize, maximize, getObjectives, define-objective, lex-optimize.
 */

#include "somtparser/frontend/parser.h"
#include <iostream>
#include "test_helpers.h"

static void test_minimize() {
    std::cout << "=== Testing (minimize x) ===" << std::endl;
    SOMTParser::Parser parser;
    VERIFY(parser.parseStr("(set-logic QF_LIA)"));
    VERIFY(parser.parseStr("(declare-const x Int)"));
    VERIFY(parser.parseStr("(minimize x)"));
    auto objs = parser.getObjectives();
    VERIFY(objs.size() == 1 && "expect one objective");
    VERIFY(objs[0]->isMinimize() && "objective should be minimize");
    VERIFY(objs[0]->getObjectiveTerm() && "minimize term should be non-null");
    std::cout << "  Objectives: " << objs.size() << ", first is minimize, term present." << std::endl;
    std::cout << "  ✓ minimize test passed" << std::endl;
}

static void test_maximize() {
    std::cout << "=== Testing (maximize y) ===" << std::endl;
    SOMTParser::Parser parser;
    VERIFY(parser.parseStr("(set-logic QF_LIA)"));
    VERIFY(parser.parseStr("(declare-const y Int)"));
    VERIFY(parser.parseStr("(maximize y)"));
    auto objs = parser.getObjectives();
    VERIFY(objs.size() == 1 && "expect one objective");
    VERIFY(objs[0]->isMaximize() && "objective should be maximize");
    std::cout << "  ✓ maximize test passed" << std::endl;
}

static void test_multiple_objectives() {
    std::cout << "=== Testing multiple objectives ===" << std::endl;
    SOMTParser::Parser parser;
    VERIFY(parser.parseStr("(set-logic QF_LIA)"));
    VERIFY(parser.parseStr("(declare-const x Int)"));
    VERIFY(parser.parseStr("(declare-const y Int)"));
    VERIFY(parser.parseStr("(minimize x)"));
    VERIFY(parser.parseStr("(maximize y)"));
    auto objs = parser.getObjectives();
    VERIFY(objs.size() == 2 && "expect two objectives");
    VERIFY(objs[0]->isMinimize() && objs[1]->isMaximize());
    std::cout << "  ✓ multiple objectives test passed" << std::endl;
}

static void test_define_objective_and_lex_optimize() {
    std::cout << "=== Testing define-objective and lex-optimize ===" << std::endl;
    SOMTParser::Parser parser;
    VERIFY(parser.parseStr("(set-logic QF_LIA)"));
    VERIFY(parser.parseStr("(declare-const x Int)"));
    VERIFY(parser.parseStr("(declare-const y Int)"));
    VERIFY(parser.parseStr("(define-objective obj1 (minimize x))"));
    VERIFY(parser.parseStr("(define-objective obj2 (maximize y))"));
    VERIFY(parser.parseStr("(lex-optimize (obj1 obj2))"));
    auto objs = parser.getObjectives();
    VERIFY(objs.size() == 3 && "expect three: obj1, obj2, and lex-optimize composite");
    VERIFY(objs[2]->isLexOptimize() && "third should be lex-optimize");
    VERIFY(objs[2]->getObjectiveSize() == 2 && "lex-optimize should have two sub-objectives");
    std::cout << "  ✓ define-objective and lex-optimize test passed" << std::endl;
}

static void test_empty_objectives_before_any_optimization() {
    std::cout << "=== Testing getObjectives() before any optimization ===" << std::endl;
    SOMTParser::Parser parser;
    VERIFY(parser.parseStr("(set-logic QF_LIA)"));
    VERIFY(parser.parseStr("(declare-const x Int)"));
    auto objs = parser.getObjectives();
    VERIFY(objs.empty() && "expect no objectives before minimize/maximize");
    std::cout << "  ✓ empty objectives test passed" << std::endl;
}

int main() {
    std::cout << "======= Optimization / OMT Tests =======\n" << std::endl;
    test_empty_objectives_before_any_optimization();
    test_minimize();
    test_maximize();
    test_multiple_objectives();
    test_define_objective_and_lex_optimize();
    std::cout << "\n======= All optimization tests passed =======" << std::endl;
    return 0;
}
