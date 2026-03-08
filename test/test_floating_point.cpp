#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

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
    test_fp_arithmetic(parser);
    test_fp_comparisons(parser);
    test_fp_conversions(parser);
    
    return 0;
} 