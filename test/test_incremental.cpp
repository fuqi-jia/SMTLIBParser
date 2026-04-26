/* -*- C++ -*-
 * Test: Incremental push/pop + Command API
 */

#include "somtparser/frontend/parser.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace SOMTParser;
using namespace std::chrono;

static void test_push_pop_assertions() {
    std::cout << "--- test_push_pop_assertions ---" << std::endl;
    Parser parser;
    parser.parseStr("(push 1) (assert true) (pop 1)");
    assert(parser.getAssertions().size() == 0);
    std::cout << "PASS" << std::endl;
}

static void test_push_pop_symbols() {
    std::cout << "--- test_push_pop_symbols ---" << std::endl;
    Parser parser;
    parser.parseStr("(push 1) (declare-const x Int) (pop 1)");
    assert(!parser.isDeclaredVariable("x"));
    std::cout << "PASS" << std::endl;
}

static void test_push_pop_objectives() {
    std::cout << "--- test_push_pop_objectives ---" << std::endl;
    Parser parser;
    parser.parseStr("(declare-const x Int) (push 1) (minimize x) (pop 1)");
    assert(parser.getObjectives().size() == 0);
    std::cout << "PASS" << std::endl;
}

static void test_nested_push_pop() {
    std::cout << "--- test_nested_push_pop ---" << std::endl;
    Parser parser;
    parser.parseStr(
        "(push 1)"
        "  (declare-const x Int)"
        "  (push 1)"
        "    (declare-const y Int)"
        "    (assert (> x 0))"
        "  (pop 1)"
        "  (assert (> x 1))"
        "(pop 1)"
    );
    assert(!parser.isDeclaredVariable("x"));
    assert(!parser.isDeclaredVariable("y"));
    assert(parser.getAssertions().size() == 0);
    std::cout << "PASS" << std::endl;
}

static void test_reset_assertions() {
    std::cout << "--- test_reset_assertions ---" << std::endl;
    Parser parser;
    parser.parseStr("(declare-const x Int) (assert (> x 0)) (reset-assertions)");
    assert(parser.isDeclaredVariable("x"));
    assert(parser.getAssertions().size() == 0);
    std::cout << "PASS" << std::endl;
}

static void test_reset() {
    std::cout << "--- test_reset ---" << std::endl;
    Parser parser;
    parser.parseStr("(declare-const x Int) (assert (> x 0)) (reset)");
    assert(!parser.isDeclaredVariable("x"));
    assert(parser.getAssertions().size() == 0);
    std::cout << "PASS" << std::endl;
}

static void test_nextCommand() {
    std::cout << "--- test_nextCommand ---" << std::endl;
    Parser parser;
    parser.parseStr("(set-logic QF_LIA) (declare-const x Int) (assert (> x 0))");

    Command cmd1 = parser.nextCommand();
    assert(cmd1.type == CMD_TYPE::CT_SET_LOGIC);

    Command cmd2 = parser.nextCommand();
    assert(cmd2.type == CMD_TYPE::CT_DECLARE_CONST);

    Command cmd3 = parser.nextCommand();
    assert(cmd3.type == CMD_TYPE::CT_ASSERT);

    Command cmd4 = parser.nextCommand();
    assert(cmd4.type == CMD_TYPE::CT_EOF);
    std::cout << "PASS" << std::endl;
}

static void test_command_script() {
    std::cout << "--- test_command_script ---" << std::endl;
    Parser parser;
    parser.setCommandLogging(true);
    parser.parseStr("(set-logic QF_LIA) (declare-const x Int) (assert (> x 0))");

    const Script& script = parser.getScript();
    assert(script.size() == 3);
    assert(script[0].type == CMD_TYPE::CT_SET_LOGIC);
    assert(script[1].type == CMD_TYPE::CT_DECLARE_CONST);
    assert(script[2].type == CMD_TYPE::CT_ASSERT);
    std::cout << "PASS" << std::endl;
}

static void test_push_pop_levels() {
    std::cout << "--- test_push_pop_levels ---" << std::endl;
    Parser parser;
    parser.parseStr("(push 2) (declare-const x Int) (declare-const y Int) (pop 2)");
    assert(!parser.isDeclaredVariable("x"));
    assert(!parser.isDeclaredVariable("y"));
    std::cout << "PASS" << std::endl;
}

static void test_push_pop_assertion_groups() {
    std::cout << "--- test_push_pop_assertion_groups ---" << std::endl;
    Parser parser;
    parser.parseStr(
        "(declare-const x Int)"
        "(push 1)"
        "  (assert (> x 0) :named a1)"
        "  (assert (> x 1) :id g1)"
        "(pop 1)"
    );
    assert(parser.getNamedAssertions().find("a1") == parser.getNamedAssertions().end());
    assert(parser.getGroupedAssertions().find("g1") == parser.getGroupedAssertions().end());
    std::cout << "PASS" << std::endl;
}

// --- Stress tests ---

static void test_stress_loop_push_pop() {
    std::cout << "--- test_stress_loop_push_pop ---" << std::endl;
    const int ITERATIONS = 5000;
    auto start = high_resolution_clock::now();

    Parser parser;
    for (int i = 0; i < ITERATIONS; ++i) {
        parser.push(1);
        parser.pop(1);
    }

    auto end = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "  " << ITERATIONS << " push/pop cycles in " << ms << " ms" << std::endl;
    assert(parser.getAssertions().empty());
    std::cout << "PASS" << std::endl;
}

static void test_stress_deep_nesting() {
    std::cout << "--- test_stress_deep_nesting ---" << std::endl;
    const int DEPTH = 1000;
    auto start = high_resolution_clock::now();

    Parser parser;
    // Build a string: (push 1) x DEPTH ... (pop 1) x DEPTH
    std::ostringstream oss;
    for (int i = 0; i < DEPTH; ++i) {
        oss << "(push 1)";
    }
    for (int i = 0; i < DEPTH; ++i) {
        oss << "(pop 1)";
    }
    bool ok = parser.parseStr(oss.str());
    assert(ok);

    auto end = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "  " << DEPTH << " nested scopes parsed in " << ms << " ms" << std::endl;
    assert(parser.getAssertions().empty());
    assert(parser.context().scope_stack_.empty());
    std::cout << "PASS" << std::endl;
}

static void test_stress_many_symbols_in_scope() {
    std::cout << "--- test_stress_many_symbols_in_scope ---" << std::endl;
    const int VARS = 500;
    auto start = high_resolution_clock::now();

    Parser parser;
    parser.push(1);
    for (int i = 0; i < VARS; ++i) {
        std::ostringstream oss;
        oss << "(declare-const v" << i << " Int)";
        parser.parseStr(oss.str());
    }
    // All variables should be declared
    for (int i = 0; i < VARS; ++i) {
        std::ostringstream oss;
        oss << "v" << i;
        assert(parser.isDeclaredVariable(oss.str()));
    }
    parser.pop(1);
    // After pop, none should exist
    for (int i = 0; i < VARS; ++i) {
        std::ostringstream oss;
        oss << "v" << i;
        assert(!parser.isDeclaredVariable(oss.str()));
    }

    auto end = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "  " << VARS << " vars declared + rolled back in " << ms << " ms" << std::endl;
    std::cout << "PASS" << std::endl;
}

static void test_stress_mixed_operations() {
    std::cout << "--- test_stress_mixed_operations ---" << std::endl;
    const int ROUNDS = 200;
    auto start = high_resolution_clock::now();

    Parser parser;
    int expected_assertions = 0;
    for (int r = 0; r < ROUNDS; ++r) {
        parser.push(1);
        // declare a variable
        {
            std::ostringstream oss;
            oss << "(declare-const x" << r << " Int)";
            parser.parseStr(oss.str());
        }
        // add an assertion
        {
            std::ostringstream oss;
            oss << "(assert (> x" << r << " 0))";
            parser.parseStr(oss.str());
            ++expected_assertions;
        }
        // add a soft assertion
        {
            std::ostringstream oss;
            oss << "(assert-soft (> x" << r << " 1) :id g" << r << ")";
            parser.parseStr(oss.str());
        }
        parser.pop(1);
    }
    // After all pops, everything should be gone
    assert(parser.getAssertions().size() == 0);
    assert(parser.getSoftAssertions().size() == 0);
    assert(parser.getObjectives().size() == 0);

    auto end = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "  " << ROUNDS << " mixed push/declare/assert/pop rounds in " << ms << " ms" << std::endl;
    std::cout << "PASS" << std::endl;
}

static void test_stress_nextCommand_stream() {
    std::cout << "--- test_stress_nextCommand_stream ---" << std::endl;
    const int CMDS = 5000;
    auto start = high_resolution_clock::now();

    std::ostringstream oss;
    for (int i = 0; i < CMDS; ++i) {
        oss << "(push 1)(pop 1)";
    }
    Parser parser;
    parser.parseStr(oss.str());
    parser.setCommandLogging(true);

    // Now walk with nextCommand until EOF
    size_t count = 0;
    Command cmd;
    do {
        cmd = parser.nextCommand();
        ++count;
    } while (cmd.type != CMD_TYPE::CT_EOF);
    // -1 because the last is EOF
    assert(count - 1 == static_cast<size_t>(CMDS * 2));

    auto end = high_resolution_clock::now();
    auto ms = duration_cast<milliseconds>(end - start).count();
    std::cout << "  " << CMDS * 2 << " commands via nextCommand in " << ms << " ms" << std::endl;
    std::cout << "PASS" << std::endl;
}

int main() {
    std::cout << "======= Incremental / Command API Tests =======" << std::endl;
    test_push_pop_assertions();
    test_push_pop_symbols();
    test_push_pop_objectives();
    test_nested_push_pop();
    test_reset_assertions();
    test_reset();
    test_nextCommand();
    test_command_script();
    test_push_pop_levels();
    test_push_pop_assertion_groups();

    std::cout << std::endl << "======= Stress Tests =======" << std::endl;
    test_stress_loop_push_pop();
    test_stress_deep_nesting();
    test_stress_many_symbols_in_scope();
    test_stress_mixed_operations();
    test_stress_nextCommand_stream();

    std::cout << std::endl << "======= All Incremental tests passed =======" << std::endl;
    return 0;
}
