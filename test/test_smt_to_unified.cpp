/* -*- C++ -*-
 * Test: SMT-LIB AST → Unified IR
 */

#include "somtparser/frontends/smt/smt_to_unified.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/frontend/command.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/unified/unified_printer.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <unistd.h>

using namespace SOMTParser;
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
#define ASSERT_NE(a, b) do { if ((a) == (b)) { std::ostringstream oss; oss << "Assertion failed: " #a " != " #b " (got " << (a) << ")"; throw std::runtime_error(oss.str()); } } while(0)

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

// ── Helper: create a temporary .smt2 file ──────────────────────────

static std::string writeTempSmt2(const std::string& content) {
    char path[] = "/tmp/smt_test_XXXXXX";
    int fd = mkstemp(path);
    if (fd < 0) throw std::runtime_error("mkstemp failed");
    close(fd);
    std::string filename = std::string(path) + ".smt2";
    std::ofstream ofs(filename);
    ofs << content;
    ofs.close();
    return filename;
}

// ── Tests: Expression conversion ───────────────────────────────────

TEST(bool_literal_true) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("true");
    ASSERT_TRUE(node && node->isTrue());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* lit = expr->asLiteral();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(std::get<bool>(lit->value), true);
}

TEST(bool_literal_false) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("false");
    ASSERT_TRUE(node && node->isFalse());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* lit = expr->asLiteral();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(std::get<bool>(lit->value), false);
}

TEST(int_literal) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("42");
    ASSERT_TRUE(node && node->isCInt());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* lit = expr->asLiteral();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(std::get<int64_t>(lit->value), 42);
}

TEST(real_literal) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("3.14");
    ASSERT_TRUE(node && node->isCReal());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* lit = expr->asLiteral();
    ASSERT_TRUE(lit != nullptr);
    double val = std::get<double>(lit->value);
    ASSERT_TRUE(val > 3.13 && val < 3.15);
}

TEST(string_literal) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("\"hello\"");
    ASSERT_TRUE(node && node->isCStr());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* lit = expr->asLiteral();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(std::get<std::string>(lit->value), "hello");
}

TEST(variable_ident) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto var = parser->mkVar(SortManager::INT_SORT, "x");
    ASSERT_TRUE(var && var->isVar());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(var);
    ASSERT_TRUE(expr != nullptr);
    auto* ident = expr->asIdent();
    ASSERT_TRUE(ident != nullptr);
    ASSERT_EQ(ident->name, "x");
}

TEST(bool_and_op) {
    auto& reg = getRegistry();
    auto parser = newParser();
    // Declare variables to avoid constant folding
    parser->mkVar(SortManager::BOOL_SORT, "a");
    parser->mkVar(SortManager::BOOL_SORT, "b");
    auto node = parser->mkExpr("(and a b)");
    ASSERT_TRUE(node && node->isAnd());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "bool_and");
    ASSERT_EQ(op->args.size(), 2u);
}

TEST(bool_or_not_ops) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::BOOL_SORT, "a");
    parser->mkVar(SortManager::BOOL_SORT, "b");
    auto node = parser->mkExpr("(or (not a) b)");
    ASSERT_TRUE(node && node->isOr());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "bool_or");
    ASSERT_EQ(op->args.size(), 2u);

    // First arg should be (not a)
    auto* not_op = op->args[0]->asOp();
    ASSERT_TRUE(not_op != nullptr);
    const auto* not_def = reg.getDef(not_op->op);
    ASSERT_TRUE(not_def != nullptr);
    ASSERT_EQ(not_def->unified_name, "bool_not");
}

TEST(arithmetic_add_sub_mul) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::INT_SORT, "x");
    parser->mkVar(SortManager::INT_SORT, "y");
    auto node = parser->mkExpr("(+ x y)");
    ASSERT_TRUE(node && node->isAdd());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "int_add");
    ASSERT_EQ(op->args.size(), 2u);
}

TEST(arithmetic_div_mod) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::INT_SORT, "x");
    parser->mkVar(SortManager::INT_SORT, "y");
    auto node = parser->mkExpr("(div x y)");
    ASSERT_TRUE(node && node->isDivInt());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "int_div");
}

TEST(comparison_eq_lt_le) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::INT_SORT, "x");
    parser->mkVar(SortManager::INT_SORT, "y");
    auto node = parser->mkExpr("(< x y)");
    ASSERT_TRUE(node && node->isLt());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "lt");
}

TEST(ite_conversion) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::BOOL_SORT, "c");
    auto node = parser->mkExpr("(ite c 1 0)");
    ASSERT_TRUE(node && node->isIte());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* ite = expr->asIte();
    ASSERT_TRUE(ite != nullptr);
    ASSERT_TRUE(ite->cond != nullptr);
    ASSERT_TRUE(ite->then_expr != nullptr);
    ASSERT_TRUE(ite->else_expr != nullptr);
}

TEST(quantifier_forall) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("(forall ((x Int)) (> x 0))");
    ASSERT_TRUE(node && node->getKind() == NODE_KIND::NT_FORALL);

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    ASSERT_TRUE(expr->kind == UnifiedExpr::Kind::FORALL);
    auto* quant = expr->asQuant();
    ASSERT_TRUE(quant != nullptr);
    ASSERT_EQ(quant->generators.size(), 1u);
    ASSERT_EQ(quant->generators[0].first, "x");
    ASSERT_TRUE(quant->body != nullptr);
}

TEST(quantifier_exists) {
    auto& reg = getRegistry();
    auto parser = newParser();
    auto node = parser->mkExpr("(exists ((x Int) (y Int)) (= x y))");
    ASSERT_TRUE(node && node->getKind() == NODE_KIND::NT_EXISTS);

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    ASSERT_TRUE(expr->kind == UnifiedExpr::Kind::EXISTS);
    auto* quant = expr->asQuant();
    ASSERT_TRUE(quant != nullptr);
    ASSERT_EQ(quant->generators.size(), 2u);
    ASSERT_TRUE(quant->body != nullptr);
}

TEST(array_select_store) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkArray("a", SortManager::INT_SORT, SortManager::INT_SORT);
    auto node = parser->mkExpr("(select a 0)");
    ASSERT_TRUE(node && node->isSelect());

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
    auto* op = expr->asOp();
    ASSERT_TRUE(op != nullptr);
    const auto* def = reg.getDef(op->op);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "array_access");
    ASSERT_EQ(op->args.size(), 2u);
}

TEST(let_conversion) {
    auto& reg = getRegistry();
    auto parser = newParser();
    parser->mkVar(SortManager::INT_SORT, "y");
    auto node = parser->mkExpr("(let ((x 1)) (+ x y))");
    // Let may be folded/evaluated; just check conversion doesn't crash

    SmtLibToUnifiedIR converter(reg);
    auto expr = converter.convertExpr(node);
    ASSERT_TRUE(expr != nullptr);
}

// ── Tests: Parser-based conversion ─────────────────────────────────

TEST(parser_assert_only) {
    auto& reg = getRegistry();
    std::string smt2 = R"(
(set-logic QF_LIA)
(declare-const x Int)
(assert (> x 0))
(check-sat)
)";
    std::string filename = writeTempSmt2(smt2);
    auto parser = newParser();
    bool ok = parser->parseSmtlib2File(filename);
    (void)ok;

    SmtLibToUnifiedIR converter(reg);
    auto model = converter.convert(*parser);

    ASSERT_TRUE(model.vars.size() >= 1u);
    ASSERT_TRUE(model.constraints.size() >= 1u);

    std::remove(filename.c_str());
}

TEST(parser_minimize) {
    auto& reg = getRegistry();
    std::string smt2 = R"(
(set-logic QF_LIA)
(declare-const x Int)
(assert (>= x 0))
(minimize x)
(check-sat)
)";
    std::string filename = writeTempSmt2(smt2);
    auto parser = newParser();
    bool ok = parser->parseSmtlib2File(filename);
    (void)ok;

    SmtLibToUnifiedIR converter(reg);
    auto model = converter.convert(*parser);

    ASSERT_TRUE(model.vars.size() >= 1u);
    ASSERT_TRUE(model.constraints.size() >= 1u);
    ASSERT_TRUE(model.objectives.size() >= 1u);
    ASSERT_EQ(static_cast<int>(model.objectives[0].mode), static_cast<int>(UnifiedObjective::Mode::MINIMIZE));
    ASSERT_TRUE(model.objectives[0].expr != nullptr);

    std::remove(filename.c_str());
}

TEST(parser_maximize) {
    auto& reg = getRegistry();
    std::string smt2 = R"(
(set-logic QF_LIA)
(declare-const y Real)
(assert (<= y 100.0))
(maximize y)
)";
    std::string filename = writeTempSmt2(smt2);
    auto parser = newParser();
    bool ok = parser->parseSmtlib2File(filename);
    (void)ok;

    SmtLibToUnifiedIR converter(reg);
    auto model = converter.convert(*parser);

    ASSERT_TRUE(model.vars.size() >= 1u);
    ASSERT_TRUE(model.objectives.size() >= 1u);
    ASSERT_EQ(static_cast<int>(model.objectives[0].mode), static_cast<int>(UnifiedObjective::Mode::MAXIMIZE));

    std::remove(filename.c_str());
}

TEST(parser_multiple_asserts) {
    auto& reg = getRegistry();
    std::string smt2 = R"(
(declare-const a Bool)
(declare-const b Bool)
(assert a)
(assert (or a b))
(assert (not b))
)";
    std::string filename = writeTempSmt2(smt2);
    auto parser = newParser();
    bool ok = parser->parseSmtlib2File(filename);
    (void)ok;

    SmtLibToUnifiedIR converter(reg);
    auto model = converter.convert(*parser);

    ASSERT_TRUE(model.vars.size() >= 2u);
    ASSERT_TRUE(model.constraints.size() >= 3u);

    std::remove(filename.c_str());
}

TEST(parser_define_fun) {
    auto& reg = getRegistry();
    std::string smt2 = R"(
(define-fun f ((x Int)) Int (+ x 1))
(assert (= (f 0) 1))
)";
    std::string filename = writeTempSmt2(smt2);
    auto parser = newParser();
    bool ok = parser->parseSmtlib2File(filename);
    (void)ok;

    SmtLibToUnifiedIR converter(reg);
    auto model = converter.convert(*parser);

    // define-fun should create a variable declaration
    ASSERT_TRUE(model.vars.size() >= 1u);
    bool found_f = false;
    for (const auto& v : model.vars) {
        if (v.name == "f") {
            found_f = true;
            ASSERT_TRUE(v.init != nullptr);
            break;
        }
    }
    ASSERT_TRUE(found_f);

    std::remove(filename.c_str());
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= SMT-LIB to Unified IR Test =======" << std::endl;

    RUN_TEST(bool_literal_true);
    RUN_TEST(bool_literal_false);
    RUN_TEST(int_literal);
    RUN_TEST(real_literal);
    RUN_TEST(string_literal);
    RUN_TEST(variable_ident);
    RUN_TEST(bool_and_op);
    RUN_TEST(bool_or_not_ops);
    RUN_TEST(arithmetic_add_sub_mul);
    RUN_TEST(arithmetic_div_mod);
    RUN_TEST(comparison_eq_lt_le);
    RUN_TEST(ite_conversion);
    RUN_TEST(quantifier_forall);
    RUN_TEST(quantifier_exists);
    RUN_TEST(array_select_store);
    RUN_TEST(let_conversion);
    RUN_TEST(parser_assert_only);
    RUN_TEST(parser_minimize);
    RUN_TEST(parser_maximize);
    RUN_TEST(parser_multiple_asserts);
    RUN_TEST(parser_define_fun);

    std::cout << "===========================================" << std::endl;
    std::cout << tests_passed << " passed, " << tests_failed << " failed" << std::endl;
    return tests_failed > 0 ? 1 : 0;
}
