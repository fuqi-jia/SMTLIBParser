/**
 * Regression tests for issues.md Issue 1 (builtin vs UF), Issue 7 (BV + BitVectorUtils), Issue 4 (FP getValue).
 */

#include "somtparser/core/kind.h"
#include "somtparser/core/util.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/value.h"
#include <cassert>
#include <iostream>

using SOMTParser::NODE_KIND;
using SOMTParser::ParserPtr;
using SOMTParser::SortManager;

static void test_issue1_reserved_and_ge_kind(ParserPtr& p) {
    std::cout << "=== Issue 1: reserved built-ins and (>= ...) kind ===" << std::endl;
    assert(SOMTParser::isBuiltinNameReservedAgainstUserFun(">="));
    assert(!SOMTParser::builtinOperMayBeShadowedByUserFun(">="));
    assert(!SOMTParser::isBuiltinNameReservedAgainstUserFun("factorial"));

    auto ok = p->mkFuncDec("factorial", {SortManager::INT_SORT}, SortManager::INT_SORT);
    assert(!ok->isUnknown() && !ok->isErr());

    p->mkVarInt("x");
    p->mkVarInt("y");
    auto ge = p->mkExpr("(>= x y)");
    assert(ge && ge->isGe());
    assert(p->getKind(">=") == NODE_KIND::NT_GE);
}

static void test_issue7_bv_utils_not_get_number_value(ParserPtr& p) {
    std::cout << "=== Issue 7: BV literal + BitVectorUtils (getNumberValue is NUMBER-only) ===" << std::endl;
    auto n = p->mkExpr("(_ bv42 8)");
    assert(n && n->isCBV());
    auto v = n->getValue();
    assert(v && v->getType() == SOMTParser::BV);
    assert(SOMTParser::BitVectorUtils::bvToNat(n->toString()) == SOMTParser::Integer(42));
}

static void test_issue4_fp_const_get_value(ParserPtr& p) {
    std::cout << "=== Issue 4: FP const getValue after mkConstFp path ===" << std::endl;
    auto n = p->mkExpr("((_ to_fp 8 24) RNE 1.0)");
    assert(n && !n->isErr() && n->isCFP());
    auto val = n->getValue();
    assert(val != nullptr);
    assert(val->getType() == SOMTParser::FP);
}

int main() {
    std::cout << "======= Issue 1 / 4 / 7 regression =======" << std::endl;

    ParserPtr p1 = SOMTParser::newParser();
    test_issue1_reserved_and_ge_kind(p1);

    ParserPtr p2 = SOMTParser::newParser();
    test_issue7_bv_utils_not_get_number_value(p2);

    ParserPtr p3 = SOMTParser::newParser();
    test_issue4_fp_const_get_value(p3);

    std::cout << "All issue 1/4/7 tests passed." << std::endl;
    return 0;
}
