#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include "somtparser/core/util.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/sort.h"
#include "somtparser/ir/value.h"
#include <cassert>

using SOMTParser::BitVectorUtils;
using SOMTParser::fpNodeToFloat32;
using SOMTParser::Integer;
using SOMTParser::Number;
using SOMTParser::ParserPtr;
using SOMTParser::Value;

namespace {

void assert_fp32_near(const std::shared_ptr<SOMTParser::DAGNode>& n, float expected,
                      float eps = 1e-3f) {
    assert(n && !n->isErr() && n->isCFP());
    auto v = fpNodeToFloat32(n);
    assert(v.has_value());
    assert(std::fabs(*v - expected) < eps);
}

void simp_expect_fp32(ParserPtr& p, const char* smt, float expected, float eps = 1e-3f) {
    auto e = p->mkExpr(smt);
    assert(e && !e->isErr());
    assert_fp32_near(e, expected, eps);
}

void simp_expect_real_near(ParserPtr& p, const char* smt, double expected, double eps = 1e-6) {
    auto e = p->mkExpr(smt);
    assert(e && !e->isErr());
    assert(e->isCReal() || e->isCInt());
    double v;
    if (e->isCInt()) {
        v = static_cast<double>(p->toInt(e).toLong());
    } else {
        v = p->toReal(e).toDouble();
    }
    assert(std::fabs(v - expected) < eps);
}

void simp_expect_bool(ParserPtr& p, const char* smt, bool expected) {
    auto e = p->mkExpr(smt);
    assert(e && !e->isErr());
    if (expected) {
        assert(e->isTrue());
    } else {
        assert(e->isFalse());
    }
}

void simp_expect_cbv_nat(ParserPtr& p, const char* smt, const Integer& expected_nat) {
    auto e = p->mkExpr(smt);
    assert(e && !e->isErr());
    assert(e->isCBV());
    assert(BitVectorUtils::bvToNat(e->toString()) == expected_nat);
}

void simp_expect_cbv_int(ParserPtr& p, const char* smt, const Integer& expected_signed) {
    auto e = p->mkExpr(smt);
    assert(e && !e->isErr());
    assert(e->isCBV());
    assert(BitVectorUtils::bvToInt(e->toString()) == expected_signed);
}

// Parse-time constant folding for FP ops (simp)
void test_fp_simp_constant_folding(SOMTParser::ParserPtr& p) {
    std::cout << "=== FP simp constant folding ===" << std::endl;
    simp_expect_fp32(p, "(fp.add RNE ((_ to_fp 8 24) RNE 1.5) ((_ to_fp 8 24) RNE 2.5))", 4.0f);
    simp_expect_fp32(p, "(fp.sub RNE ((_ to_fp 8 24) RNE 10.0) ((_ to_fp 8 24) RNE 3.0))", 7.0f);
    simp_expect_fp32(p, "(fp.mul RNE ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 4.0))", 12.0f);
    simp_expect_fp32(p, "(fp.div RNE ((_ to_fp 8 24) RNE 15.0) ((_ to_fp 8 24) RNE 5.0))", 3.0f);
    simp_expect_fp32(p, "(fp.rem ((_ to_fp 8 24) RNE 7.0) ((_ to_fp 8 24) RNE 3.0))", 1.0f);
    simp_expect_fp32(p, "(fp.sqrt RNE ((_ to_fp 8 24) RNE 25.0))", 5.0f);
    simp_expect_fp32(p, "(fp.roundToIntegral RNE ((_ to_fp 8 24) RNE 3.5))", 4.0f);
    simp_expect_fp32(p, "(fp.roundToIntegral RTZ ((_ to_fp 8 24) RNE 3.7))", 3.0f);
    simp_expect_fp32(p, "(fp.fma RNE ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 1.0))",
                     7.0f);
    simp_expect_real_near(p, "(fp.to_real ((_ to_fp 8 24) RNE 3.14))", 3.14, 0.01);
}

// (=) / distinct on FP use FPValue bit-structural equality (NT_EQ); fp.eq uses IEEE fpCompare (NT_FP_EQ).
void test_fp_generic_eq_distinct_constant_folding(SOMTParser::ParserPtr& p) {
    std::cout << "=== FP (=) / distinct vs fp.eq (constant folding) ===" << std::endl;
    simp_expect_bool(p, "(= ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(distinct ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(distinct ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", false);
    // IEEE: +0 and -0 are equal under fp.eq; generic (=) compares bit patterns — distinct.
    simp_expect_bool(p, "(fp.eq (_ +zero 8 24) (_ -zero 8 24))", true);
    simp_expect_bool(p, "(= (_ +zero 8 24) (_ -zero 8 24))", false);
    simp_expect_bool(p, "(distinct (_ +zero 8 24) (_ -zero 8 24))", true);
}

void test_fp_comparison_constant_folding(SOMTParser::ParserPtr& p) {
    std::cout << "=== FP comparison constant folding ===" << std::endl;
    simp_expect_bool(p, "(fp.eq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(fp.lt ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(fp.gt ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(fp.leq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(fp.geq ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 3.0))", true);
    simp_expect_bool(p, "(fp.lt ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 3.0))", false);
}

void test_fp_to_bv_constant_folding(SOMTParser::ParserPtr& p) {
    std::cout << "=== fp.to_ubv / fp.to_sbv constant folding ===" << std::endl;
    simp_expect_cbv_nat(p, "((_ fp.to_ubv 16) RNE ((_ to_fp 8 24) RNE 5.0))", Integer(5));
    simp_expect_cbv_int(p, "((_ fp.to_sbv 16) RNE ((_ to_fp 8 24) RNE -3.0))", Integer(-3));
}

// Lenient surface: unary fp.sqrt / fp.roundToIntegral; flat to_fp / to_fp_unsigned
void test_fp_dialect_unary_and_to_fp_simp(SOMTParser::ParserPtr& p) {
    std::cout << "=== Lenient FP surface + to_fp simp ===" << std::endl;
    simp_expect_fp32(p, "(fp.sqrt ((_ to_fp 8 24) RNE 25.0))", 5.0f);
    simp_expect_fp32(p, "(fp.roundToIntegral ((_ to_fp 8 24) RNE 3.5))", 4.0f);
    simp_expect_fp32(p, "((_ to_fp 8 24) RNE 3.14)", 3.14f, 0.02f);
    simp_expect_fp32(p, "((_ to_fp 8 24) RNE ((_ to_fp 8 24) RNE 2.25))", 2.25f);
    simp_expect_fp32(p, "((_ to_fp_unsigned 8 24) RNE (_ bv5 32))", 5.0f);
}

void test_value_fp_operators_ir() {
    std::cout << "=== Value class FP arithmetic (IR) ===" << std::endl;
    Value a(Number(Integer(10)));
    a.setType(SOMTParser::FP);
    a.setFpValue("10.0", 8, 24);
    Value b(Number(Integer(3)));
    b.setType(SOMTParser::FP);
    b.setFpValue("3.0", 8, 24);

    Value sum = a.fadd(b);
    assert(sum.getType() == SOMTParser::FP);
    Value diff = a.fsub(b);
    assert(diff.getType() == SOMTParser::FP);
    Value prod = a.fmul(b);
    assert(prod.getType() == SOMTParser::FP);
}

}  // namespace

// Test basic floating-point constants and representation
void test_fp_constants(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(_ +zero 8 24)",        // IEEE 754 single-precision +0.0
        "(_ -zero 8 24)",        // IEEE 754 single-precision -0.0
        "(_ +oo 8 24)",          // IEEE 754 single-precision +inf
        "(_ -oo 8 24)",          // IEEE 754 single-precision -inf
        "(_ NaN 8 24)",          // IEEE 754 single-precision NaN
        "(fp #b0 #b01111111 #b00000000000000000000000)", // IEEE 754 single-precision 1.0 bit representation
        "(fp #b1 #b10000010 #b01100000000000000000000)"  // IEEE 754 single-precision -6.5 bit representation
    };
    
    std::cout << "=== Test floating-point constants ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test floating-point arithmetic operations
void test_fp_arithmetic(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(fp.add RNE ((_ to_fp 8 24) RNE 3.14) ((_ to_fp 8 24) RNE 2.71))",    // add, round to nearest even
        "(fp.sub RNE ((_ to_fp 8 24) RNE 10.5) ((_ to_fp 8 24) RNE 4.2))",     // subtract, round to nearest even
        "(fp.mul RNE ((_ to_fp 8 24) RNE 2.5) ((_ to_fp 8 24) RNE 4.0))",      // multiply, round to nearest even
        "(fp.div RNE ((_ to_fp 8 24) RNE 15.0) ((_ to_fp 8 24) RNE 3.0))",     // divide, round to nearest even
        "(fp.fma RNE ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 1.0))", // fused multiply-add
        "(fp.sqrt RNE ((_ to_fp 8 24) RNE 16.0))",                              // square root
        "(fp.rem ((_ to_fp 8 24) RNE 17.5) ((_ to_fp 8 24) RNE 5.2))",         // remainder
        "(fp.roundToIntegral RNE ((_ to_fp 8 24) RNE 3.7))",                    // round to integral
        "(fp.min ((_ to_fp 8 24) RNE 4.2) ((_ to_fp 8 24) RNE 4.3))",          // minimum
        "(fp.max ((_ to_fp 8 24) RNE 4.2) ((_ to_fp 8 24) RNE 4.3))"           // maximum
    };
    
    std::cout << "=== Test floating-point arithmetic ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test floating-point comparison operations
void test_fp_comparisons(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(fp.eq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",           // equal
        "(fp.lt ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 4.0))",           // less than
        "(fp.gt ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 4.0))",           // greater than
        "(fp.leq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",          // less than or equal
        "(fp.geq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",          // greater than or equal
        "(fp.isNormal ((_ to_fp 8 24) RNE 1.0))",                               // is normal
        "(fp.isSubnormal ((_ to_fp 11 53) RNE 0.0001))",                      // is subnormal
        "(fp.isZero ((_ to_fp 8 24) RNE 0.0))",                                 // is zero
        "(fp.isInfinite (_ +oo 8 24))",                                       // is infinite
        "(fp.isNaN (_ NaN 8 24))",                                            // is NaN
        "(fp.isNegative ((_ to_fp 8 24) RNE -1.0))",                            // is negative
        "(fp.isPositive ((_ to_fp 8 24) RNE 1.0))"                              // is positive
    };
    
    std::cout << "=== Test floating-point comparisons ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test floating-point conversion operations
void test_fp_const_get_value_set(SOMTParser::ParserPtr& parser) {
    std::cout << "=== FP const getValue (mkConstFp / to_fp) ===" << std::endl;
    auto n = parser->mkExpr("((_ to_fp 8 24) RNE 1.0)");
    assert(n && !n->isErr() && n->isCFP());
    auto val = n->getValue();
    assert(val != nullptr);
    assert(val->getType() == SOMTParser::FP);
}

void test_fp_sort_width_contract(SOMTParser::ParserPtr& parser) {
    std::cout << "=== FP Sort getExponentWidth / getSignificandWidth contract ===" << std::endl;
    auto sm = parser->getSortManager();
    auto custom = sm->createFPSort(11, 53);
    assert(custom->getExponentWidth() == 11);
    assert(custom->getSignificandWidth() == 53);
    auto f32 = SOMTParser::SortManager::getFloat32();
    assert(f32->getExponentWidth() == 8);
    assert(f32->getSignificandWidth() == 24);
    auto n = parser->mkExpr("((_ to_fp 8 24) RNE 1.0)");
    assert(n && n->getSort() && n->getSort()->isFp());
    assert(n->getSort()->getExponentWidth() == 8);
    assert(n->getSort()->getSignificandWidth() == 24);
}

void test_fp_conversions(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // real to floating-point
        "((_ to_fp 8 24) RNE 3.14159)",
        // integer to floating-point
        "((_ to_fp 8 24) RNE 42)",
        // binary string to floating-point
        "((_ to_fp 8 24) #b01000001001000000000000000000000)",
        // floating-point to real
        "(fp.to_real ((_ to_fp 8 24) RNE 3.14))",
        // floating-point to signed bit-vector (round toward zero)
        "((_ fp.to_sbv 32) RTZ ((_ to_fp 8 24) RNE 3.14))",
        // floating-point to unsigned bit-vector (round toward zero)
        "((_ fp.to_ubv 32) RTZ ((_ to_fp 8 24) RNE 3.14))",
        // real to different precision floating-point
        "((_ to_fp 11 53) RNE 3.14)",  // real to double precision
        "((_ to_fp 5 11) RNE 3.14)",   // real to half precision
        // conversion between different precision floating-point
        "((_ to_fp 11 53) RNE ((_ to_fp 8 24) RNE 3.14))",  // single to double precision
        "((_ to_fp 5 11) RNE ((_ to_fp 8 24) RNE 3.14))"    // single to half precision
    };

    std::cout << "=== Test floating-point conversions ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "======= Floating-point theory test =======" << std::endl;

    SOMTParser::ParserPtr parser = SOMTParser::newParser();

    test_fp_constants(parser);
    test_fp_const_get_value_set(parser);
    test_fp_sort_width_contract(parser);
    test_fp_arithmetic(parser);
    test_fp_comparisons(parser);
    test_fp_conversions(parser);

    test_fp_simp_constant_folding(parser);
    test_fp_generic_eq_distinct_constant_folding(parser);
    test_fp_comparison_constant_folding(parser);
    test_fp_to_bv_constant_folding(parser);
    test_fp_dialect_unary_and_to_fp_simp(parser);
    test_value_fp_operators_ir();

    return 0;
} 