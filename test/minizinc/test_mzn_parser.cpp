/* -*- C++ -*-
 *
 * MiniZinc Frontend — Parser Tests
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

// ── Helpers ────────────────────────────────────────────────────────
static Model parse(const std::string& src) {
    MznParser parser;
    return parser.parseString(src);
}

// ── Tests ──────────────────────────────────────────────────────────

TEST(empty_model) {
    auto m = parse("");
    ASSERT_EQ(m.items.size(), 0);
}

TEST(var_decl_simple) {
    auto m = parse("var int: x;");
    ASSERT_EQ(m.items.size(), 1);
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->name, "x");
    ASSERT_TRUE(vd->type->isVar());
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::INT);
}

TEST(var_decl_bool) {
    auto m = parse("var bool: b;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::BOOL);
}

TEST(var_decl_float) {
    auto m = parse("var float: f;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::FLOAT);
}

TEST(var_decl_with_domain) {
    auto m = parse("var 1..10: x;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->type->base, TypeInst::BaseKind::INT);
    ASSERT_TRUE(vd->type->domain_expr != nullptr);
}

TEST(par_decl_with_init) {
    auto m = parse("int: n = 5;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->isPar());
    ASSERT_TRUE(vd->init != nullptr);
    auto* lit = vd->init->as<IntLit>();
    ASSERT_TRUE(lit != nullptr);
    ASSERT_EQ(lit->value, 5);
}

TEST(array_decl_1d) {
    auto m = parse("array[1..3] of int: a = [1,2,3];");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_TRUE(vd->type->elem_type != nullptr);
    ASSERT_EQ(vd->type->elem_type->base, TypeInst::BaseKind::INT);
    ASSERT_TRUE(vd->init != nullptr);
    auto* arr = vd->init->as<ArrayLit>();
    ASSERT_TRUE(arr != nullptr);
    ASSERT_EQ(arr->elements.size(), 3);
}

TEST(constraint_simple) {
    auto m = parse("constraint x > 0;");
    ASSERT_EQ(m.items.size(), 1);
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[0].get());
    ASSERT_TRUE(ci != nullptr);
    ASSERT_EQ(ci->expr->kind, Expr::Kind::BINARY_OP);
}

TEST(solve_satisfy) {
    auto m = parse("solve satisfy;");
    ASSERT_EQ(m.items.size(), 1);
    auto* si = dynamic_cast<SolveItem*>(m.items[0].get());
    ASSERT_TRUE(si != nullptr);
    ASSERT_EQ(si->mode, SolveItem::Mode::SATISFY);
}

TEST(solve_minimize) {
    auto m = parse("solve minimize x + y;");
    auto* si = dynamic_cast<SolveItem*>(m.items[0].get());
    ASSERT_TRUE(si != nullptr);
    ASSERT_EQ(si->mode, SolveItem::Mode::MINIMIZE);
    ASSERT_TRUE(si->objective != nullptr);
}

TEST(solve_maximize) {
    auto m = parse("solve maximize x * y;");
    auto* si = dynamic_cast<SolveItem*>(m.items[0].get());
    ASSERT_TRUE(si != nullptr);
    ASSERT_EQ(si->mode, SolveItem::Mode::MAXIMIZE);
}

TEST(output_item) {
    auto m = parse("output [\"hello\"];");
    auto* oi = dynamic_cast<OutputItem*>(m.items[0].get());
    ASSERT_TRUE(oi != nullptr);
}

TEST(enum_decl) {
    auto m = parse("enum Color = {R, G, B};");
    auto* ed = dynamic_cast<EnumDeclItem*>(m.items[0].get());
    ASSERT_TRUE(ed != nullptr);
    ASSERT_EQ(ed->name, "Color");
    ASSERT_EQ(ed->constructors.size(), 3);
}

TEST(predicate_decl) {
    auto m = parse("predicate even(int: x) = x mod 2 = 0;");
    auto* pd = dynamic_cast<PredicateItem*>(m.items[0].get());
    ASSERT_TRUE(pd != nullptr);
    ASSERT_EQ(pd->name, "even");
    ASSERT_EQ(pd->params.size(), 1);
    ASSERT_TRUE(pd->body != nullptr);
}

TEST(function_decl) {
    auto m = parse("function int: sqr(int: x) = x * x;");
    auto* fd = dynamic_cast<FunctionItem*>(m.items[0].get());
    ASSERT_TRUE(fd != nullptr);
    ASSERT_EQ(fd->name, "sqr");
    ASSERT_TRUE(fd->body != nullptr);
}

TEST(if_then_else) {
    auto m = parse("var int: x = if c > 0 then a else b endif;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    auto* ite = vd->init->as<IfThenElse>();
    ASSERT_TRUE(ite != nullptr);
    ASSERT_EQ(ite->branches.size(), 1);
    ASSERT_TRUE(ite->else_branch != nullptr);
}

TEST(array_literal) {
    auto m = parse("array[1..3] of int: a = [1, 2, 3];");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    auto* arr = vd->init->as<ArrayLit>();
    ASSERT_TRUE(arr != nullptr);
    ASSERT_EQ(arr->elements.size(), 3);
}

TEST(set_literal) {
    auto m = parse("set of int: S = {1, 2, 3};");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    auto* setlit = vd->init->as<SetLit>();
    ASSERT_TRUE(setlit != nullptr);
    ASSERT_EQ(setlit->elements.size(), 3);
}

TEST(tuple_literal) {
    auto m = parse("tuple(int, int): t = (1, 2);");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    auto* tup = vd->init->as<TupleLit>();
    ASSERT_TRUE(tup != nullptr);
    ASSERT_EQ(tup->elements.size(), 2);
}

TEST(multiple_items) {
    auto m = parse("var int: x; constraint x > 0; solve satisfy;");
    ASSERT_EQ(m.items.size(), 3);
    ASSERT_TRUE(dynamic_cast<VarDeclItem*>(m.items[0].get()) != nullptr);
    ASSERT_TRUE(dynamic_cast<ConstraintItem*>(m.items[1].get()) != nullptr);
    ASSERT_TRUE(dynamic_cast<SolveItem*>(m.items[2].get()) != nullptr);
}

TEST(include_item) {
    auto m = parse("include \"globals.mzn\";");
    auto* ii = dynamic_cast<IncludeItem*>(m.items[0].get());
    ASSERT_TRUE(ii != nullptr);
    ASSERT_EQ(ii->filename, "globals.mzn");
}

TEST(nested_bool_expr) {
    auto m = parse("constraint a /\\ b \\/ c -> d;");
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[0].get());
    ASSERT_TRUE(ci != nullptr);
    ASSERT_EQ(ci->expr->kind, Expr::Kind::BINARY_OP);
}

TEST(arithmetic_expr) {
    auto m = parse("constraint x + y * z - 5 = 0;");
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[0].get());
    ASSERT_TRUE(ci != nullptr);
}

TEST(let_expr) {
    auto m = parse("var int: x = let { int: t = 1 } in t + 2;");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    auto* let = vd->init->as<LetExpr>();
    ASSERT_TRUE(let != nullptr);
    ASSERT_EQ(let->items.size(), 1);
}

TEST(array_access_expr) {
    auto m = parse("constraint a[i] > 0;");
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[0].get());
    auto* acc = ci->expr->as<ArrayAccess>();
    // a[i] > 0 is parsed as binary op, left side is array access
    ASSERT_TRUE(ci->expr->kind == Expr::Kind::BINARY_OP);
}

TEST(call_expr) {
    auto m = parse("constraint abs(x) > 0;");
    auto* ci = dynamic_cast<ConstraintItem*>(m.items[0].get());
    ASSERT_TRUE(ci->expr->kind == Expr::Kind::BINARY_OP);
}

TEST(quoted_ident_var) {
    auto m = parse("var int: 'foo bar';");
    auto* vd = dynamic_cast<VarDeclItem*>(m.items[0].get());
    ASSERT_TRUE(vd != nullptr);
    ASSERT_EQ(vd->name, "foo bar");
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Parser Tests =======\n\n";

    RUN_TEST(empty_model);
    RUN_TEST(var_decl_simple);
    RUN_TEST(var_decl_bool);
    RUN_TEST(var_decl_float);
    RUN_TEST(var_decl_with_domain);
    RUN_TEST(par_decl_with_init);
    RUN_TEST(array_decl_1d);
    RUN_TEST(constraint_simple);
    RUN_TEST(solve_satisfy);
    RUN_TEST(solve_minimize);
    RUN_TEST(solve_maximize);
    RUN_TEST(output_item);
    RUN_TEST(enum_decl);
    RUN_TEST(predicate_decl);
    RUN_TEST(function_decl);
    RUN_TEST(if_then_else);
    RUN_TEST(array_literal);
    RUN_TEST(set_literal);
    RUN_TEST(tuple_literal);
    RUN_TEST(multiple_items);
    RUN_TEST(include_item);
    RUN_TEST(nested_bool_expr);
    RUN_TEST(arithmetic_expr);
    RUN_TEST(let_expr);
    RUN_TEST(array_access_expr);
    RUN_TEST(call_expr);
    RUN_TEST(quoted_ident_var);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
