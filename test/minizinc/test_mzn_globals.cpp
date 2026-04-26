/* -*- C++ -*-
 *
 * MiniZinc Frontend — Global Constraint Decomposition Tests
 */

#include "somtparser/frontends/minizinc/mzn_parser.h"
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

TEST(all_different_3) {
    auto m = parse("array[1..3] of var int: x; constraint all_different(x);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(all_equal_3) {
    auto m = parse("array[1..3] of var int: x; constraint all_equal(x);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(count_eq) {
    auto m = parse("array[1..5] of var int: a; var int: c; constraint count_eq(a, 5, c);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(global_cardinality) {
    auto m = parse("array[1..5] of var int: a; array[1..2] of int: cover = [1,2]; array[1..2] of var int: counts; constraint global_cardinality(a, cover, counts);");
    ASSERT_EQ(m.items.size(), 4);
}

TEST(increasing) {
    auto m = parse("array[1..4] of var int: a; constraint increasing(a);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(strictly_increasing) {
    auto m = parse("array[1..4] of var int: a; constraint strictly_increasing(a);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(element) {
    auto m = parse("var int: i; array[1..3] of int: a = [1,2,3]; var int: v; constraint element(i, a, v);");
    ASSERT_EQ(m.items.size(), 4);
}

TEST(table) {
    auto m = parse("array[1..2] of var int: x; constraint table(x, [|1,2|3,4|]);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(lex_less) {
    auto m = parse("array[1..2] of var int: a; array[1..2] of var int: b; constraint lex_less(a, b);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(disjunctive_2) {
    auto m = parse("array[1..2] of var int: s; array[1..2] of int: d = [3,4]; constraint disjunctive(s, d);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(inverse_3) {
    auto m = parse("array[1..3] of var 1..3: f; array[1..3] of var 1..3: invf; constraint inverse(f, invf);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(member) {
    auto m = parse("array[1..3] of var int: a; var int: x; constraint member(a, x);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(at_least) {
    auto m = parse("array[1..5] of var int: a; constraint at_least(2, a, 7);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(at_most) {
    auto m = parse("array[1..5] of var int: a; constraint at_most(2, a, 7);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(exactly) {
    auto m = parse("array[1..5] of var int: a; constraint exactly(2, a, 7);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(among) {
    auto m = parse("array[1..5] of var int: a; constraint among(3, a, {1,2,3});");
    ASSERT_EQ(m.items.size(), 2);
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Globals Tests =======\n\n";

    RUN_TEST(all_different_3);
    RUN_TEST(all_equal_3);
    RUN_TEST(count_eq);
    RUN_TEST(global_cardinality);
    RUN_TEST(increasing);
    RUN_TEST(strictly_increasing);
    RUN_TEST(element);
    RUN_TEST(table);
    RUN_TEST(lex_less);
    RUN_TEST(disjunctive_2);
    RUN_TEST(inverse_3);
    RUN_TEST(member);
    RUN_TEST(at_least);
    RUN_TEST(at_most);
    RUN_TEST(exactly);
    RUN_TEST(among);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
