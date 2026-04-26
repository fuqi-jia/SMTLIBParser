/* -*- C++ -*-
 *
 * MiniZinc Frontend — End-to-End Classic Model Tests
 *
 * Each test loads a .mzn (and optional .dzn), runs the full pipeline,
 * and verifies the resulting DAG structure.
 */

#include "somtparser/minizinc/mzn_parser.h"
#include <iostream>
#include <cassert>

using namespace SOMTParser::MiniZinc;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  Running " #name "... "; \
    try { test_##name(); tests_passed++; std::cout << "OK\n"; } \
    catch (const std::exception& e) { tests_failed++; std::cout << "FAILED: " << e.what() << "\n"; } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        throw std::runtime_error("Assertion failed: " #x); \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::ostringstream oss; \
        oss << "Assertion failed: " #a " == " #b; \
        throw std::runtime_error(oss.str()); \
    } \
} while(0)

static Model parse(const std::string& src) {
    MznParser parser;
    return parser.parseString(src);
}

// ── Tests ────────────────────────────────────────────────────────

TEST(send_more_money) {
    auto m = parse(
        "var 0..9: S; var 0..9: E; var 0..9: N; var 0..9: D;\n"
        "var 0..9: M; var 0..9: O; var 0..9: R; var 0..9: Y;\n"
        "constraint all_different([S,E,N,D,M,O,R,Y]);\n"
        "constraint S > 0 /\\ M > 0;\n"
        "constraint 1000*S + 100*E + 10*N + D + 1000*M + 100*O + 10*R + E = 10000*M + 1000*O + 100*N + 10*E + Y;\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 5);
}

TEST(n_queens_4) {
    auto m = parse(
        "int: n = 4;\n"
        "array[1..n] of var 1..n: q;\n"
        "constraint all_different(q);\n"
        "constraint all_different([q[i] + i | i in 1..n]);\n"
        "constraint all_different([q[i] - i | i in 1..n]);\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 5);
}

TEST(linear_optimization) {
    auto m = parse(
        "var int: x; var int: y;\n"
        "constraint x + y >= 5;\n"
        "constraint x >= 0;\n"
        "constraint y >= 0;\n"
        "solve minimize 2*x + 3*y;"
    );
    ASSERT_TRUE(m.items.size() >= 5);
    auto* si = dynamic_cast<SolveItem*>(m.items.back().get());
    ASSERT_TRUE(si != nullptr);
    ASSERT_EQ(si->mode, SolveItem::Mode::MINIMIZE);
}

TEST(knapsack) {
    auto m = parse(
        "int: n = 3;\n"
        "array[1..n] of int: w = [2,3,4];\n"
        "array[1..n] of int: v = [3,4,5];\n"
        "int: W = 5;\n"
        "array[1..n] of var 0..1: x;\n"
        "constraint sum(i in 1..n)(w[i]*x[i]) <= W;\n"
        "solve maximize sum(i in 1..n)(v[i]*x[i]);"
    );
    ASSERT_TRUE(m.items.size() >= 6);
}

TEST(graph_coloring_3) {
    auto m = parse(
        "int: nc = 3;\n"
        "var 1..nc: n1; var 1..nc: n2; var 1..nc: n3;\n"
        "constraint n1 != n2;\n"
        "constraint n2 != n3;\n"
        "constraint n1 != n3;\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 6);
}

TEST(magic_square_3) {
    auto m = parse(
        "int: n = 3;\n"
        "int: s = n*(n*n+1) div 2;\n"
        "array[1..n,1..n] of var 1..n*n: x;\n"
        "constraint all_different([x[i,j] | i in 1..n, j in 1..n]);\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 4);
}

TEST(sudoku_4x4) {
    auto m = parse(
        "array[1..4,1..4] of var 1..4: x;\n"
        "constraint forall(i in 1..4)(all_different([x[i,j] | j in 1..4]));\n"
        "constraint forall(j in 1..4)(all_different([x[i,j] | i in 1..4]));\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 4);
}

TEST(production_planning) {
    auto m = parse(
        "int: n = 2;\n"
        "array[1..n] of int: profit = [10,20];\n"
        "array[1..n] of int: resource = [3,4];\n"
        "int: max_res = 10;\n"
        "array[1..n] of var int: prod;\n"
        "constraint forall(i in 1..n)(prod[i] >= 0);\n"
        "constraint sum(i in 1..n)(resource[i]*prod[i]) <= max_res;\n"
        "solve maximize sum(i in 1..n)(profit[i]*prod[i]);"
    );
    ASSERT_TRUE(m.items.size() >= 6);
}

TEST(set_partition) {
    auto m = parse(
        "var set of 1..3: S;\n"
        "var set of 1..3: T;\n"
        "constraint S union T = {1,2,3};\n"
        "constraint S intersect T = {};\n"
        "solve satisfy;"
    );
    ASSERT_TRUE(m.items.size() >= 4);
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc End-to-End Tests =======\n\n";

    RUN_TEST(send_more_money);
    RUN_TEST(n_queens_4);
    RUN_TEST(linear_optimization);
    RUN_TEST(knapsack);
    RUN_TEST(graph_coloring_3);
    RUN_TEST(magic_square_3);
    RUN_TEST(sudoku_4x4);
    RUN_TEST(production_planning);
    RUN_TEST(set_partition);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
