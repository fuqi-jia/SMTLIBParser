/**
 * Regression tests for issues.md Issue 1 (builtin vs UF), Issue 7 (BV + BitVectorUtils),
 * Issue 4 (FP getValue), Issue 2 (sort-classified variable accessors).
 */

#include "somtparser/core/kind.h"
#include "somtparser/core/util.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/value.h"
#include <iostream>
#include "test_helpers.h"

using SOMTParser::NODE_KIND;
using SOMTParser::ParserPtr;
using SOMTParser::SortManager;

static void test_issue1_reserved_and_ge_kind(ParserPtr& p) {
    std::cout << "=== Issue 1: reserved built-ins and (>= ...) kind ===" << std::endl;
    VERIFY(SOMTParser::isBuiltinNameReservedAgainstUserFun(">="));
    VERIFY(!SOMTParser::builtinOperMayBeShadowedByUserFun(">="));
    VERIFY(!SOMTParser::isBuiltinNameReservedAgainstUserFun("factorial"));

    auto ok = p->mkFuncDec("factorial", {SortManager::INT_SORT}, SortManager::INT_SORT);
    VERIFY(!ok->isUnknown() && !ok->isErr());

    p->mkVarInt("x");
    p->mkVarInt("y");
    auto ge = p->mkExpr("(>= x y)");
    VERIFY(ge && ge->isGe());
    VERIFY(p->getKind(">=") == NODE_KIND::NT_GE);
}

static void test_issue7_bv_utils_not_get_number_value(ParserPtr& p) {
    std::cout << "=== Issue 7: BV literal + BitVectorUtils (getNumberValue is NUMBER-only) ===" << std::endl;
    auto n = p->mkExpr("(_ bv42 8)");
    VERIFY(n && n->isCBV());
    auto v = n->getValue();
    VERIFY(v && v->getType() == SOMTParser::BV);
    VERIFY(SOMTParser::BitVectorUtils::bvToNat(n->toString()) == SOMTParser::Integer(42));
}

static void test_issue4_fp_const_get_value(ParserPtr& p) {
    std::cout << "=== Issue 4: FP const getValue after mkConstFp path ===" << std::endl;
    auto n = p->mkExpr("((_ to_fp 8 24) RNE 1.0)");
    VERIFY(n && !n->isErr() && n->isCFP());
    auto val = n->getValue();
    VERIFY(val != nullptr);
    VERIFY(val->getType() == SOMTParser::FP);
}

// ─── Issue #2: Sort-classified variable accessors ────────────────────────────
// Verifies that getBoolVars / getIntVars / getRealVars / getBvVars / getFpVars /
// getRoundingModeVars / getDatatypeVars / getStringVars / getArrayVars
// (and the corresponding getNum*Vars counters) correctly classify declared
// variables by theory.
static void test_issue2_sort_classified_vars() {
    std::cout << "=== Issue #2: sort-classified variable accessors ===" << std::endl;

    // ── Bool vars ──────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarBool("b1");
        p->mkVarBool("b2");
        VERIFY(p->getNumBoolVars() == 2);
        VERIFY(p->getBoolVars().size() == 2);
        VERIFY(p->getNumIntVars() == 0);
        VERIFY(p->getNumFpVars() == 0);
        std::cout << "  Bool vars: OK\n";
    }

    // ── Int vars ───────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarInt("i1");
        p->mkVarInt("i2");
        p->mkVarInt("i3");
        VERIFY(p->getNumIntVars() == 3);
        VERIFY(p->getNumBoolVars() == 0);
        VERIFY(p->getNumRealVars() == 0);
        std::cout << "  Int vars: OK\n";
    }

    // ── Real vars ──────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarReal("r1");
        VERIFY(p->getNumRealVars() == 1);
        VERIFY(p->getNumIntVars() == 0);
        std::cout << "  Real vars: OK\n";
    }

    // ── BV vars ────────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarBv("bv1", 32);
        p->mkVarBv("bv2", 8);
        VERIFY(p->getNumBvVars() == 2);
        VERIFY(p->getNumBoolVars() == 0);
        std::cout << "  BV vars: OK\n";
    }

    // ── FP vars ────────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarFp("fp1", 8, 24);   // Float32
        p->mkVarFp("fp2", 11, 53);  // Float64
        VERIFY(p->getNumFpVars() == 2);
        VERIFY(p->getFpVars().size() == 2);
        VERIFY(p->getNumBvVars() == 0);
        std::cout << "  FP vars: OK\n";
    }

    // ── Rounding mode vars ─────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->mkVarRoundingMode("rm1");
        VERIFY(p->getNumRoundingModeVars() == 1);
        VERIFY(p->getNumFpVars() == 0);
        std::cout << "  RoundingMode vars: OK\n";
    }

    // ── Mixed problem: Int + FP + Bool ─────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->parseStr(
            "(set-logic ALL)\n"
            "(declare-fun b () Bool)\n"
            "(declare-fun i () Int)\n"
            "(declare-fun r () Real)\n"
            "(declare-fun fp32 () (_ FloatingPoint 8 24))\n"
            "(declare-fun bv16 () (_ BitVec 16))\n");
        VERIFY(p->getNumBoolVars() == 1);
        VERIFY(p->getNumIntVars()  == 1);
        VERIFY(p->getNumRealVars() == 1);
        VERIFY(p->getNumFpVars()   == 1);
        VERIFY(p->getNumBvVars()   == 1);
        // Total through getDeclaredVariables
        VERIFY(p->getDeclaredVariables().size() == 5);
        std::cout << "  Mixed problem variable classification: OK\n";
    }

    // ── String vars ────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->parseStr(
            "(set-logic QF_S)\n"
            "(declare-fun s1 () String)\n"
            "(declare-fun s2 () String)\n");
        VERIFY(p->getNumStringVars() == 2);
        VERIFY(p->getNumBoolVars() == 0);
        std::cout << "  String vars: OK\n";
    }

    // ── DT vars ────────────────────────────────────────────────────────────
    {
        ParserPtr p = SOMTParser::newParser();
        p->parseStr(
            "(set-logic ALL)\n"
            "(declare-datatypes ((Color 0)) (((red) (green) (blue))))\n"
            "(declare-fun c1 () Color)\n"
            "(declare-fun c2 () Color)\n");
        VERIFY(p->getNumDatatypeVars() == 2);
        VERIFY(p->getNumBoolVars() == 0);
        std::cout << "  Datatype vars: OK\n";
    }

    std::cout << "  Issue #2 sort-classified var accessors: all assertions passed\n";
}

int main() {
    std::cout << "======= Issue 1 / 2 / 4 / 7 regression =======" << std::endl;

    ParserPtr p1 = SOMTParser::newParser();
    test_issue1_reserved_and_ge_kind(p1);

    ParserPtr p2 = SOMTParser::newParser();
    test_issue7_bv_utils_not_get_number_value(p2);

    ParserPtr p3 = SOMTParser::newParser();
    test_issue4_fp_const_get_value(p3);

    test_issue2_sort_classified_vars();

    std::cout << "All issue 1/2/4/7 tests passed." << std::endl;
    return 0;
}
