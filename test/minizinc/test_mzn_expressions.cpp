/* -*- C++ -*-
 *
 * MiniZinc Frontend — Expression Lowering Tests
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

TEST(bool_not) {
    auto m = parse("var bool: a; constraint not a;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(bool_and) {
    auto m = parse("var bool: a; var bool: b; constraint a /\\ b;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(bool_or) {
    auto m = parse("var bool: a; var bool: b; constraint a \\/ b;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(bool_implies) {
    auto m = parse("var bool: a; var bool: b; constraint a -> b;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(bool_iff) {
    auto m = parse("var bool: a; var bool: b; constraint a <-> b;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(bool_xor) {
    auto m = parse("var bool: a; var bool: b; constraint a xor b;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_add) {
    auto m = parse("var int: x; var int: y; constraint x + y = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_sub) {
    auto m = parse("var int: x; var int: y; constraint x - y = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_mul) {
    auto m = parse("var int: x; var int: y; constraint x * y = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_div) {
    auto m = parse("var int: x; var int: y; constraint x div y = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_mod) {
    auto m = parse("var int: x; var int: y; constraint x mod y = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(arith_int_pow) {
    auto m = parse("var int: x; constraint x ^ 2 = 4;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(arith_int_neg) {
    auto m = parse("var int: x; constraint -x = 0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(arith_float_add) {
    auto m = parse("var float: x; constraint x + 2.5 = 0.0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(arith_float_div) {
    auto m = parse("var float: x; var float: y; constraint x / y = 0.0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(comparison_lt) {
    auto m = parse("var int: x; var int: y; constraint x < y;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(comparison_le) {
    auto m = parse("var int: x; var int: y; constraint x <= y;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(comparison_eq) {
    auto m = parse("var int: x; var int: y; constraint x = y;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(comparison_neq) {
    auto m = parse("var int: x; var int: y; constraint x != y;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(ite_expr) {
    auto m = parse("var int: x = if true then 1 else 2 endif;");
    ASSERT_EQ(m.items.size(), 1);
}

TEST(call_abs) {
    auto m = parse("var int: x; constraint abs(x) > 0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(array_literal_const) {
    auto m = parse("array[1..3] of int: a = [1, 2, 3];");
    ASSERT_EQ(m.items.size(), 1);
}

TEST(array_access_var) {
    auto m = parse("array[1..3] of int: a; var int: i; constraint a[i] > 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(coercion_bool2int) {
    auto m = parse("var bool: b; var int: x = bool2int(b);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(coercion_int2float) {
    auto m = parse("var int: x; var float: f = int2float(x);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(transcendental_sin) {
    auto m = parse("var float: x; constraint sin(x) > 0.0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(transcendental_exp) {
    auto m = parse("var float: x; constraint exp(x) > 0.0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(let_binding) {
    auto m = parse("var int: x = let { int: t = 1 } in t + 2;");
    ASSERT_EQ(m.items.size(), 1);
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Expression Tests =======\n\n";

    RUN_TEST(bool_not);
    RUN_TEST(bool_and);
    RUN_TEST(bool_or);
    RUN_TEST(bool_implies);
    RUN_TEST(bool_iff);
    RUN_TEST(bool_xor);
    RUN_TEST(arith_int_add);
    RUN_TEST(arith_int_sub);
    RUN_TEST(arith_int_mul);
    RUN_TEST(arith_int_div);
    RUN_TEST(arith_int_mod);
    RUN_TEST(arith_int_pow);
    RUN_TEST(arith_int_neg);
    RUN_TEST(arith_float_add);
    RUN_TEST(arith_float_div);
    RUN_TEST(comparison_lt);
    RUN_TEST(comparison_le);
    RUN_TEST(comparison_eq);
    RUN_TEST(comparison_neq);
    RUN_TEST(ite_expr);
    RUN_TEST(call_abs);
    RUN_TEST(array_literal_const);
    RUN_TEST(array_access_var);
    RUN_TEST(coercion_bool2int);
    RUN_TEST(coercion_int2float);
    RUN_TEST(transcendental_sin);
    RUN_TEST(transcendental_exp);
    RUN_TEST(let_binding);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
