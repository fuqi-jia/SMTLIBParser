#include <iostream>
#include <string>
#include <vector>
#include "somtparser/core/util.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/value.h"
#include <cassert>

// Test bitvector constants
void test_bv_const_value_and_utils_nat(SOMTParser::ParserPtr& parser) {
    std::cout << "=== BV (_ bv n w) getValue + BitVectorUtils (not getNumberValue) ===" << std::endl;
    auto n = parser->mkExpr("(_ bv42 8)");
    assert(n && n->isCBV());
    auto v = n->getValue();
    assert(v && v->getType() == SOMTParser::BV);
    assert(v->getBvWidth() == 8);
    assert(SOMTParser::BitVectorUtils::bvToNat(n->toString()) == SOMTParser::Integer(42));
}

void test_bitvector_constants(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "#b1010",                     // 4-bit binary
        "#x1A",                       // hexadecimal (26 in decimal)
        "(_ bv42 8)",              // 42 as an 8-bit bitvector
        "(_ bv255 8)"              // 255 as an 8-bit bitvector
    };
    
    std::cout << "=== Testing Bitvector Constants ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test bitvector logical operations
void test_bv_logical_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(bvnot #b1010)",
        "(bvand #b1010 #b1100)",
        "(bvor #b1010 #b1100)",
        "(bvxor #b1010 #b1100)",
        "(bvnand #b1010 #b1100)",
        "(bvnor #b1010 #b1100)",
        "(bvxnor #b1010 #b1100)"
    };
    
    std::cout << "=== Testing Bitvector Logical Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test bitvector arithmetic operations
void test_bv_arithmetic_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(bvneg #b0101)",
        "(bvadd #b0101 #b0011)",
        "(bvsub #b1010 #b0011)",
        "(bvmul #b0101 #b0011)",
        "(bvudiv #b1010 #b0011)",
        "(bvurem #b1010 #b0011)",
        "(bvshl #b0101 #b0011)",
        "(bvlshr #b1010 #b0001)",
        "(bvashr #b1010 #b0001)"
    };
    
    std::cout << "=== Testing Bitvector Arithmetic Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test bitvector comparison operations
void test_bv_comparison_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(bvult #b0101 #b1010)",     // unsigned less than
        "(bvule #b0101 #b0101)",     // unsigned less than or equal
        "(bvugt #b1010 #b0101)",     // unsigned greater than
        "(bvuge #b0101 #b0101)",     // unsigned greater than or equal
        "(bvslt #b0101 #b1010)",     // signed less than
        "(bvsle #b0101 #b0101)",     // signed less than or equal
        "(bvsgt #b0000 #b1111)",     // signed greater than
        "(bvsge #b0101 #b0101)"      // signed greater than or equal
    };
    
    std::cout << "=== Testing Bitvector Comparison Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && (result->isTrue() || result->isFalse()));
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

void test_value_bv_operators_ir() {
    std::cout << "=== Value class BV logical/shift (IR) ===" << std::endl;
    SOMTParser::Value a(SOMTParser::Number(SOMTParser::Integer(0xA)));
    a.setType(SOMTParser::BV);
    a.setBvWidth(4);
    SOMTParser::Value b(SOMTParser::Number(SOMTParser::Integer(0x5)));
    b.setType(SOMTParser::BV);
    b.setBvWidth(4);

    SOMTParser::Value r_and = a.andOp(b);
    assert(r_and.getType() == SOMTParser::BV);
    assert(r_and.toNumber() == SOMTParser::Number(SOMTParser::Integer(0x0)));

    SOMTParser::Value r_or = a.orOp(b);
    assert(r_or.toNumber() == SOMTParser::Number(SOMTParser::Integer(0xF)));

    SOMTParser::Value r_xor = a.xorOp(b);
    assert(r_xor.toNumber() == SOMTParser::Number(SOMTParser::Integer(0xF)));

    SOMTParser::Value r_shl = a.shift_left(SOMTParser::Number(SOMTParser::Integer(1)));
    assert(r_shl.toNumber() == SOMTParser::Number(SOMTParser::Integer(20)));
}

int main() {
    std::cout << "======= Bitvector Operations Test =======" << std::endl;

    SOMTParser::ParserPtr parser = SOMTParser::newParser();

    test_bitvector_constants(parser);
    test_bv_const_value_and_utils_nat(parser);
    test_bv_logical_operations(parser);
    test_bv_arithmetic_operations(parser);
    test_bv_comparison_operations(parser);
    test_value_bv_operators_ir();

    return 0;
} 