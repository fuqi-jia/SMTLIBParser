/* -*- C++ -*-
 *
 * MiniZinc Frontend — Type Tests
 */

#include "somtparser/frontends/minizinc/mzn_parser.h"
#include "somtparser/frontends/minizinc/mzn_type_checker.h"
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

TEST(int_type) {
    auto m = parse("var int: x;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::INT);
}

TEST(bool_type) {
    auto m = parse("var bool: b;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::BOOL);
}

TEST(float_type) {
    auto m = parse("var float: f;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::FLOAT);
}

TEST(array_type_1d) {
    auto m = parse("array[1..3] of int: a;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->elem_type != nullptr);
    ASSERT_EQ(vd->type->elem_type->base, TypeInst::BaseKind::INT);
}

TEST(array_type_2d) {
    auto m = parse("array[1..2, 1..3] of int: m;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->array_dims.size(), 2);
}

TEST(set_type) {
    auto m = parse("set of int: S;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->is_set);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::INT);
}

TEST(opt_type) {
    auto m = parse("opt int: o;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->is_opt);
}

TEST(enum_type) {
    auto m = parse("enum Color = {R, G, B};");
    auto* ed = dynamic_cast<EnumDeclItem*>(m.items[0].get());
    ASSERT_TRUE(ed != nullptr);
    ASSERT_EQ(ed->constructors.size(), 3);
}

TEST(tuple_type) {
    auto m = parse("tuple(int, float): t;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::TUPLE);
    ASSERT_EQ(vd->type->tuple_elems.size(), 2);
}

TEST(record_type) {
    auto m = parse("record(int: x, int: y): r;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::RECORD);
    ASSERT_EQ(vd->type->record_fields.size(), 2);
}

TEST(type_check_simple) {
    auto m = parse("var int: x; constraint x > 0;");
    MznSymbolTable sym;
    MznTypeChecker tc(&sym);
    tc.checkModel(m);
    ASSERT_TRUE(!tc.hasErrors());
}

TEST(type_check_bool_constraint) {
    auto m = parse("var bool: b; constraint b /\\ true;");
    MznSymbolTable sym;
    MznTypeChecker tc(&sym);
    tc.checkModel(m);
    ASSERT_TRUE(!tc.hasErrors());
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Type Tests =======\n\n";

    RUN_TEST(int_type);
    RUN_TEST(bool_type);
    RUN_TEST(float_type);
    RUN_TEST(array_type_1d);
    RUN_TEST(array_type_2d);
    RUN_TEST(set_type);
    RUN_TEST(opt_type);
    RUN_TEST(enum_type);
    RUN_TEST(tuple_type);
    RUN_TEST(record_type);
    RUN_TEST(type_check_simple);
    RUN_TEST(type_check_bool_constraint);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
