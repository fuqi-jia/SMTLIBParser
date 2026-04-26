/* -*- C++ -*-
 * Test: Unified IR Core
 */

#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/unified/unified_visitor.h"
#include "somtparser/unified/unified_rewriter.h"
#include "somtparser/unified/unified_printer.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>

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

TEST(expr_literal) {
    auto lit = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    lit->data = UnifiedExpr::Literal::mkInt(42);
    ASSERT_TRUE(lit->asLiteral() != nullptr);
    ASSERT_EQ(std::get<int64_t>(lit->asLiteral()->value), 42);

    auto blit = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    blit->data = UnifiedExpr::Literal::mkBool(true);
    ASSERT_EQ(std::get<bool>(blit->asLiteral()->value), true);
}

TEST(expr_ident) {
    auto id = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    id->data = UnifiedExpr::Ident{"x"};
    ASSERT_EQ(id->asIdent()->name, "x");
}

TEST(expr_op) {
    auto& reg = getRegistry();
    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());

    auto x = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    x->data = UnifiedExpr::Ident{"x"};
    auto y = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    y->data = UnifiedExpr::Ident{"y"};

    auto add = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    add->data = UnifiedExpr::OpNode{ref, {x, y}, {}};

    ASSERT_TRUE(add->asOp() != nullptr);
    ASSERT_EQ(add->asOp()->args.size(), 2u);
}

TEST(expr_array_lit) {
    auto a = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    a->data = UnifiedExpr::Literal::mkInt(1);
    auto b = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    b->data = UnifiedExpr::Literal::mkInt(2);

    auto arr = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ARRAY_LIT, SourceLoc{});
    arr->data = UnifiedExpr::ArrayLit{{a, b}};

    ASSERT_EQ(arr->asArray()->elems.size(), 2u);
}

TEST(expr_ite) {
    auto c = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    c->data = UnifiedExpr::Literal::mkBool(true);
    auto t = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    t->data = UnifiedExpr::Literal::mkInt(1);
    auto e = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    e->data = UnifiedExpr::Literal::mkInt(0);

    auto ite = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ITE, SourceLoc{});
    ite->data = UnifiedExpr::IteExpr{c, t, e};

    ASSERT_TRUE(ite->asIte() != nullptr);
}

TEST(expr_quant) {
    auto body = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    body->data = UnifiedExpr::Literal::mkBool(true);

    auto forall = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::FORALL, SourceLoc{});
    forall->data = UnifiedExpr::QuantExpr{{{"i", nullptr}}, body};

    ASSERT_TRUE(forall->asQuant() != nullptr);
    ASSERT_EQ(forall->asQuant()->generators.size(), 1u);
}

TEST(model_build) {
    UnifiedModel model;

    UnifiedVarDecl x("x", UnifiedType(UnifiedSort::mkInt()));
    model.addVar(std::move(x));

    UnifiedVarDecl y("y", UnifiedType(UnifiedSort::mkBool()));
    model.addVar(std::move(y));

    ASSERT_EQ(model.vars.size(), 2u);
}

TEST(visitor_walk) {
    auto a = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    a->data = UnifiedExpr::Literal::mkInt(1);
    auto b = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    b->data = UnifiedExpr::Literal::mkInt(2);
    auto arr = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ARRAY_LIT, SourceLoc{});
    arr->data = UnifiedExpr::ArrayLit{{a, b}};

    ASSERT_TRUE(arr->asArray() != nullptr);
    ASSERT_EQ(arr->asArray()->elems.size(), 2u);

    int count = 0;
    struct CountVisitor : public UnifiedVisitor {
        int* count;
        CountVisitor(int* c) : count(c) {}
        void visit(UnifiedExpr& e) override { (void)e; (*count)++; }
    };

    CountVisitor cv(&count);
    cv.walk(arr);
    ASSERT_EQ(count, 3); // array + 2 literals
}

TEST(rewriter_identity) {
    auto a = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    a->data = UnifiedExpr::Literal::mkInt(1);
    auto b = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    b->data = UnifiedExpr::Literal::mkInt(2);

    auto arr = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ARRAY_LIT, SourceLoc{});
    arr->data = UnifiedExpr::ArrayLit{{a, b}};

    UnifiedRewriter rw;
    installDefaultRewriteRules(rw);
    auto result = rw.rewrite(arr);

    ASSERT_TRUE(result.get() == arr.get()); // identity: no change
}

TEST(rewriter_custom_rule) {
    auto lit = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    lit->data = UnifiedExpr::Literal::mkInt(0);

    UnifiedRewriter rw;
    rw.onLiteral([](const UnifiedExpr::Literal& l) -> ExprPtr {
        if (l.lit_kind == UnifiedExpr::Literal::LitKind::INT &&
            std::get<int64_t>(l.value) == 0) {
            auto zero = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
            zero->data = UnifiedExpr::Literal::mkInt(999);
            return zero;
        }
        return nullptr;
    });

    auto result = rw.rewrite(lit);
    ASSERT_TRUE(result->asLiteral() != nullptr);
    ASSERT_EQ(std::get<int64_t>(result->asLiteral()->value), 999);
}

TEST(printer_smtlib_expr) {
    auto& reg = getRegistry();
    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());

    auto x = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    x->data = UnifiedExpr::Ident{"x"};
    auto y = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    y->data = UnifiedExpr::Ident{"y"};

    auto add = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    add->data = UnifiedExpr::OpNode{ref, {x, y}, {}};

    UnifiedPrinter printer(reg);
    std::string s = printer.exprToSmtLib(add);
    ASSERT_TRUE(s.find("(+ x y)") != std::string::npos);
}

TEST(printer_minizinc_expr) {
    auto& reg = getRegistry();
    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());

    auto x = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    x->data = UnifiedExpr::Ident{"x"};
    auto y = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    y->data = UnifiedExpr::Ident{"y"};

    auto add = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    add->data = UnifiedExpr::OpNode{ref, {x, y}, {}};

    UnifiedPrinter printer(reg);
    std::string s = printer.exprToMiniZinc(add);
    ASSERT_TRUE(s.find("(x + y)") != std::string::npos);
}

TEST(printer_debug) {
    auto& reg = getRegistry();
    auto ref = reg.lookupByUnifiedName("bool_and");
    ASSERT_TRUE(ref.valid());

    auto t = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    t->data = UnifiedExpr::Literal::mkBool(true);
    auto f = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    f->data = UnifiedExpr::Literal::mkBool(false);

    auto and_expr = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    and_expr->data = UnifiedExpr::OpNode{ref, {t, f}, {}};

    UnifiedPrinter printer(reg);
    std::string s = printer.toDebugString(and_expr);
    ASSERT_TRUE(s.find("OP(bool_and)") != std::string::npos);
    ASSERT_TRUE(s.find("LITERAL(true)") != std::string::npos);
    ASSERT_TRUE(s.find("LITERAL(false)") != std::string::npos);
}

TEST(printer_model_roundtrip) {
    auto& reg = getRegistry();
    auto ref_lt = reg.lookupByUnifiedName("lt");
    ASSERT_TRUE(ref_lt.valid());

    UnifiedModel model;
    UnifiedVarDecl x("x", UnifiedType(UnifiedSort::mkInt()));
    model.addVar(std::move(x));

    auto xid = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::IDENT, SourceLoc{});
    xid->data = UnifiedExpr::Ident{"x"};
    auto ten = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LITERAL, SourceLoc{});
    ten->data = UnifiedExpr::Literal::mkInt(10);

    auto cmp = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    cmp->data = UnifiedExpr::OpNode{ref_lt, {xid, ten}, {}};
    model.addConstraint(UnifiedConstraint(cmp));

    UnifiedPrinter printer(reg);
    std::string smt = printer.toSmtLib(model);
    ASSERT_TRUE(smt.find("(declare-fun x () Int)") != std::string::npos);
    ASSERT_TRUE(smt.find("(assert (< x 10))") != std::string::npos);
    ASSERT_TRUE(smt.find("(check-sat)") != std::string::npos);

    std::string mzn = printer.toMiniZinc(model);
    ASSERT_TRUE(mzn.find("var int: x;") != std::string::npos);
    ASSERT_TRUE(mzn.find("constraint (x < 10);") != std::string::npos);
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= Unified IR Core Tests =======\n\n";

    RUN_TEST(expr_literal);
    RUN_TEST(expr_ident);
    RUN_TEST(expr_op);
    RUN_TEST(expr_array_lit);
    RUN_TEST(expr_ite);
    RUN_TEST(expr_quant);
    RUN_TEST(model_build);
    RUN_TEST(visitor_walk);
    RUN_TEST(rewriter_identity);
    RUN_TEST(rewriter_custom_rule);
    RUN_TEST(printer_smtlib_expr);
    RUN_TEST(printer_minizinc_expr);
    RUN_TEST(printer_debug);
    RUN_TEST(printer_model_roundtrip);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
