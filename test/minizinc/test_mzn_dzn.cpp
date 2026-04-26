/* -*- C++ -*-
 *
 * MiniZinc Frontend — .dzn Parser & Merger Tests
 */

#include "somtparser/frontends/minizinc/mzn_dzn_parser.h"
#include "somtparser/frontends/minizinc/mzn_parser.h"
#include <iostream>
#include <cassert>
#include <sstream>

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

// ── Tests ────────────────────────────────────────────────────────

TEST(int_scalar) {
    MznDznParser parser;
    auto data = parser.parseString("n = 5;");
    ASSERT_TRUE(data.has("n"));
    auto* lit = data.get("n")->as<IntLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, 5);
}

TEST(float_scalar) {
    MznDznParser parser;
    auto data = parser.parseString("pi = 3.14;");
    ASSERT_TRUE(data.has("pi"));
    auto* lit = data.get("pi")->as<FloatLit>();
    ASSERT_TRUE(lit != nullptr);
}

TEST(bool_scalar) {
    MznDznParser parser;
    auto data = parser.parseString("flag = true;");
    ASSERT_TRUE(data.has("flag"));
    auto* lit = data.get("flag")->as<BoolLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_TRUE(lit->value);
}

TEST(string_scalar) {
    MznDznParser parser;
    auto data = parser.parseString("name = \"test\";");
    ASSERT_TRUE(data.has("name"));
    auto* lit = data.get("name")->as<StringLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, "test");
}

TEST(array_1d) {
    MznDznParser parser;
    auto data = parser.parseString("arr = [1, 2, 3];");
    ASSERT_TRUE(data.has("arr"));
    auto* lit = data.get("arr")->as<ArrayLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->elements.size(), 3);
}

TEST(array_2d_literal) {
    MznDznParser parser;
    auto data = parser.parseString("arr2d = [|1, 2|3, 4|];");
    ASSERT_TRUE(data.has("arr2d"));
    auto* lit = data.get("arr2d")->as<ArrayLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->elements.size(), 4);
}

TEST(array_2d_array2d) {
    MznDznParser parser;
    auto data = parser.parseString("arr2d = array2d(1..2, 1..2, [1,2,3,4]);");
    ASSERT_TRUE(data.has("arr2d"));
    auto* lit = data.get("arr2d")->as<ArrayLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->elements.size(), 4);
}

TEST(set_literal) {
    MznDznParser parser;
    auto data = parser.parseString("S = {1, 2, 3};");
    ASSERT_TRUE(data.has("S"));
    auto* lit = data.get("S")->as<SetLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->elements.size(), 3);
}

TEST(range) {
    MznDznParser parser;
    auto data = parser.parseString("R = 1..10;");
    ASSERT_TRUE(data.has("R"));
    auto* lit = data.get("R")->as<SetLit>();
    ASSERT_TRUE(lit != nullptr);
}

TEST(multiple_assignments) {
    MznDznParser parser;
    auto data = parser.parseString("a=1; b=2; c=[1,2];");
    ASSERT_TRUE(data.has("a"));
    ASSERT_TRUE(data.has("b"));
    ASSERT_TRUE(data.has("c"));
}

TEST(merge_with_mzn) {
    MznParser mzn_parser;
    auto model = mzn_parser.parseString("int: n;");
    MznDznParser dzn_parser;
    auto data = dzn_parser.parseString("n = 5;");
    MznDznMerger merger;
    auto merged = merger.merge(model, data);
    auto* vd = dynamic_cast<VarDeclItem*>(merged.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->init != nullptr);
    auto* lit = vd->init->as<SOMTParser::MiniZinc::IntLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, 5);
}

TEST(merge_type_mismatch) {
    MznParser mzn_parser;
    auto model = mzn_parser.parseString("int: n;");
    MznDznParser dzn_parser;
    auto data = dzn_parser.parseString("n = true;");
    MznDznMerger merger;
    try {
        merger.merge(model, data);
        throw std::runtime_error("Expected type mismatch error");
    } catch (const MznTypeError&) {
        // expected
    }
}

TEST(duplicate_key_error) {
    MznDznParser parser;
    try {
        parser.parseString("n=1; n=2;");
        throw std::runtime_error("Expected duplicate key error");
    } catch (const MznParseError&) {
        // expected
    }
}

// ── Main ─────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc DZN Tests =======\n\n";

    RUN_TEST(int_scalar);
    RUN_TEST(float_scalar);
    RUN_TEST(bool_scalar);
    RUN_TEST(string_scalar);
    RUN_TEST(array_1d);
    RUN_TEST(array_2d_literal);
    RUN_TEST(array_2d_array2d);
    RUN_TEST(set_literal);
    RUN_TEST(range);
    RUN_TEST(multiple_assignments);
    RUN_TEST(merge_with_mzn);
    RUN_TEST(merge_type_mismatch);
    RUN_TEST(duplicate_key_error);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
