/* -*- C++ -*-
 * Test: Unified IR → SMT Lowering
 */

#include "somtparser/frontends/minizinc/mzn_parser.h"
#include "somtparser/frontends/minizinc/mzn_to_unified.h"
#include "somtparser/lowering/lower_to_smt.h"
#include "somtparser/unified/unified_op_registry.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace SOMTParser;
using namespace SOMTParser::MiniZinc;
using namespace SOMTParser::Unified;
using namespace SOMTParser::Lowering;

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

TEST(literal_lower) {
    auto& reg = getRegistry();
    Parser parser;
    LowerToSmt lowerer(parser, reg);

    auto lit = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, Unified::SourceLoc{});
    lit->data = UnifiedExpr::Literal::mkInt(42);

    auto dag = lowerer.lowerExpr(lit);
    ASSERT_TRUE(dag != nullptr);
}

TEST(arithmetic_lower) {
    auto& reg = getRegistry();
    Parser parser;
    LowerToSmt lowerer(parser, reg);

    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());

    auto a = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, Unified::SourceLoc{});
    a->data = UnifiedExpr::Literal::mkInt(1);
    auto b = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, Unified::SourceLoc{});
    b->data = UnifiedExpr::Literal::mkInt(2);

    auto add = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, Unified::SourceLoc{});
    add->data = UnifiedExpr::OpNode{ref, {a, b}, {}};

    auto dag = lowerer.lowerExpr(add);
    ASSERT_TRUE(dag != nullptr);
}

TEST(bool_lower) {
    auto& reg = getRegistry();
    Parser parser;
    LowerToSmt lowerer(parser, reg);

    auto ref = reg.lookupByUnifiedName("bool_and");
    ASSERT_TRUE(ref.valid());

    auto t = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, Unified::SourceLoc{});
    t->data = UnifiedExpr::Literal::mkBool(true);
    auto f = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, Unified::SourceLoc{});
    f->data = UnifiedExpr::Literal::mkBool(false);

    auto and_expr = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, Unified::SourceLoc{});
    and_expr->data = UnifiedExpr::OpNode{ref, std::vector<Unified::ExprPtr>{t, f}, {}};

    auto dag = lowerer.lowerExpr(and_expr);
    ASSERT_TRUE(dag != nullptr);
}

TEST(model_lower) {
    auto& reg = getRegistry();
    MznParser mzn_parser;
    auto mzn = mzn_parser.parseString(
        "var int: x;\n"
        "var int: y;\n"
        "constraint x + y <= 10;\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);

    Parser parser;
    LowerToSmt lowerer(parser, reg);
    auto assertion = lowerer.lowerModel(unified);

    ASSERT_TRUE(assertion != nullptr);
    ASSERT_TRUE(!lowerer.hasErrors());
}

TEST(model_lower_with_objective) {
    auto& reg = getRegistry();
    MznParser mzn_parser;
    auto mzn = mzn_parser.parseString(
        "var int: x;\n"
        "constraint x > 0;\n"
        "solve minimize x;\n",
        "test.mzn"
    );

    MznAstToUnifiedIR converter(reg);
    auto unified = converter.convert(mzn);

    Parser parser;
    LowerToSmt lowerer(parser, reg);
    auto assertion = lowerer.lowerModel(unified);

    ASSERT_TRUE(assertion != nullptr);
    ASSERT_TRUE(!lowerer.hasErrors());
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= Lower To SMT Tests =======\n\n";

    RUN_TEST(literal_lower);
    RUN_TEST(arithmetic_lower);
    RUN_TEST(bool_lower);
    RUN_TEST(model_lower);
    RUN_TEST(model_lower_with_objective);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
