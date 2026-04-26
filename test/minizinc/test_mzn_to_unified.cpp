/* -*- C++ -*-
 * Test: MiniZinc AST → Unified IR
 */

#include "somtparser/frontends/minizinc/mzn_parser.h"
#include "somtparser/frontends/minizinc/mzn_to_unified.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/unified/unified_printer.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace SOMTParser;
using namespace SOMTParser::MiniZinc;
using namespace SOMTParser::Unified;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  Running " #name "... "; \
    try { test_##name(); tests_passed++; std::cout << "OK\n"; } \
    catch (const std::exception& e) { tests_failed++; std::cout << "FAILED: " << e.what() << "\n"; } \
} while(0)

#define ASSERT_TRUE(x) do { if (!(x)) { throw std::runtime_error("Assertion failed: " #x); } } while(0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { std::ostringstream oss; oss << "Assertion failed: " #a " == " #b " (got " << (a) << " vs " << (b) << ")"; throw std::runtime_error(oss.str()); } } while(0)

static std::string findConfigPath() {
    std::vector<std::string> candidates = {
        "../config/unified_ops.json",
        "../../config/unified_ops.json",
        "../../../config/unified_ops.json",
        "config/unified_ops.json"
    };
    for (const auto& p : candidates) {
        if (std::ifstream(p).good()) return p;
    }
    return "";
}

static UnifiedOpRegistry& getRegistry() {
    static UnifiedOpRegistry reg;
    static bool loaded = false;
    if (!loaded) {
        std::string path = findConfigPath();
        if (!path.empty()) {
            bool ok = reg.loadFromFile(path);
            (void)ok;
        }
        loaded = true;
    }
    return reg;
}

// ── Tests ──────────────────────────────────────────────────────────

TEST(literal_conversion) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString("constraint true;", "test.mzn");
    ASSERT_EQ(mzn.items.size(), 1u);

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);
    ASSERT_EQ(unified.constraints.size(), 1u);
    ASSERT_TRUE(unified.constraints[0].expr != nullptr);
    ASSERT_TRUE(unified.constraints[0].expr->asLiteral() != nullptr);
    ASSERT_EQ(std::get<bool>(unified.constraints[0].expr->asLiteral()->value), true);
}

TEST(arithmetic_conversion) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString("var int: x; constraint x + 1 = 2;", "test.mzn");

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);
    ASSERT_EQ(unified.vars.size(), 1u);
    ASSERT_EQ(unified.vars[0].name, "x");
    ASSERT_EQ(unified.constraints.size(), 1u);

    auto* op = unified.constraints[0].expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "eq");
}

TEST(bool_ops_conversion) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString("var bool: a; var bool: b; constraint a /\\ b;", "test.mzn");

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);
    ASSERT_EQ(unified.vars.size(), 2u);
    ASSERT_EQ(unified.constraints.size(), 1u);

    auto* op = unified.constraints[0].expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "bool_and");
}

TEST(quantifier_conversion) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString(
        "array[1..3] of var int: xs;\n"
        "constraint forall(i in 1..3)(xs[i] > 0);\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);
    ASSERT_EQ(unified.constraints.size(), 1u);

    auto* quant = unified.constraints[0].expr->asQuant();
    ASSERT_TRUE(quant != nullptr);
    ASSERT_EQ(quant->generators.size(), 1u);
    ASSERT_EQ(quant->generators[0].first, "i");
}

TEST(global_cp_conversion) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString(
        "array[1..4] of var 1..4: xs;\n"
        "constraint all_different(xs);\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);
    ASSERT_EQ(unified.constraints.size(), 1u);

    auto* op = unified.constraints[0].expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "all_different");
    ASSERT_EQ(def->category, "global_cp");
}

TEST(model_roundtrip_minizinc) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString(
        "var int: x;\n"
        "var int: y;\n"
        "constraint x + y <= 10;\n"
        "solve minimize x;\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);

    UnifiedPrinter printer(reg);
    std::string output = printer.toMiniZinc(unified);

    ASSERT_TRUE(output.find("var int: x;") != std::string::npos);
    ASSERT_TRUE(output.find("var int: y;") != std::string::npos);
    ASSERT_TRUE(output.find("constraint") != std::string::npos);
    ASSERT_TRUE(output.find("solve minimize") != std::string::npos);
}

TEST(model_roundtrip_smtlib) {
    auto& reg = getRegistry();
    MznParser parser;
    auto mzn = parser.parseString(
        "var int: x;\n"
        "constraint x > 0;\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);

    UnifiedPrinter printer(reg);
    std::string output = printer.toSmtLib(unified);

    ASSERT_TRUE(output.find("(declare-fun x () Int)") != std::string::npos);
    ASSERT_TRUE(output.find("(assert (> x 0))") != std::string::npos);
    ASSERT_TRUE(output.find("(check-sat)") != std::string::npos);
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc → Unified IR Tests =======\n\n";

    RUN_TEST(literal_conversion);
    RUN_TEST(arithmetic_conversion);
    RUN_TEST(bool_ops_conversion);
    RUN_TEST(quantifier_conversion);
    RUN_TEST(global_cp_conversion);
    RUN_TEST(model_roundtrip_minizinc);
    RUN_TEST(model_roundtrip_smtlib);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
