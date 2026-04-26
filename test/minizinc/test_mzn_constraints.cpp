/* -*- C++ -*-
 *
 * MiniZinc Frontend — Constraint Lowering Tests
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

TEST(single_constraint) {
    auto m = parse("var int: x; constraint x > 0;");
    ASSERT_EQ(m.items.size(), 2);
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[1].get());
    ASSERT_TRUE(ci != nullptr);
}

TEST(multiple_constraints) {
    auto m = parse("var int: x; constraint x > 0; constraint x < 10; constraint x != 5;");
    ASSERT_EQ(m.items.size(), 4);
}

TEST(bool_constraint) {
    auto m = parse("var bool: b; var int: x; constraint b -> (x > 0);");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(domain_range) {
    auto m = parse("var 1..10: x;");
    ASSERT_EQ(m.items.size(), 1);
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->domain_expr != nullptr);
}

TEST(domain_set) {
    auto m = parse("var {1,3,5}: y;");
    ASSERT_EQ(m.items.size(), 1);
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->domain_expr != nullptr);
}

TEST(array_element_constraint) {
    auto m = parse("array[1..3] of int: a; var int: i; constraint a[i] = 0;");
    ASSERT_EQ(m.items.size(), 3);
}

TEST(opt_constraint) {
    auto m = parse("opt int: o; constraint occurs(o) -> deopt(o) > 0;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(annotated_constraint) {
    auto m = parse("var int: x; constraint x > 0 :: domain;");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(implied_constraint) {
    auto m = parse("var int: x; constraint implied_constraint(x > 0);");
    ASSERT_EQ(m.items.size(), 2);
}

TEST(redundant_constraint) {
    auto m = parse("var int: x; constraint redundant_constraint(x > 0);");
    ASSERT_EQ(m.items.size(), 2);
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Constraint Tests =======\n\n";

    RUN_TEST(single_constraint);
    RUN_TEST(multiple_constraints);
    RUN_TEST(bool_constraint);
    RUN_TEST(domain_range);
    RUN_TEST(domain_set);
    RUN_TEST(array_element_constraint);
    RUN_TEST(opt_constraint);
    RUN_TEST(annotated_constraint);
    RUN_TEST(implied_constraint);
    RUN_TEST(redundant_constraint);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
