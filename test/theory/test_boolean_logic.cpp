#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

// Test basic boolean constants
void test_boolean_constants(SOMTParser::ParserPtr& parser) {
    std::cout << "=== Testing Boolean Constants ===" << std::endl;
    auto t = parser->mkExpr("true");
    VERIFY(t && t->isTrue() && !t->isFalse());
    std::cout << "Expression: true" << std::endl;
    std::cout << "  Result: " << parser->toString(t) << std::endl;
    std::cout << "  isTrue: yes" << std::endl;
    std::cout << "  isFalse: no" << std::endl << std::endl;

    auto f = parser->mkExpr("false");
    VERIFY(f && f->isFalse() && !f->isTrue());
    std::cout << "Expression: false" << std::endl;
    std::cout << "  Result: " << parser->toString(f) << std::endl;
    std::cout << "  isTrue: no" << std::endl;
    std::cout << "  isFalse: yes" << std::endl << std::endl;
}

// Test logical operations (expected: true/false)
void test_logical_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::pair<std::string, bool>> cases = {
        {"(not true)", false},
        {"(not false)", true},
        {"(and true true)", true},
        {"(and true false)", false},
        {"(and false false)", false},
        {"(or true true)", true},
        {"(or true false)", true},
        {"(or false false)", false},
        {"(=> true true)", true},
        {"(=> true false)", false},
        {"(=> false true)", true},
        {"(=> false false)", true},
        {"(xor true true)", false},
        {"(xor true false)", true},
        {"(xor false false)", false}
    };
    
    std::cout << "=== Testing Logical Operations ===" << std::endl;
    for (const auto& p : cases) {
        std::cout << "Expression: " << p.first << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(p.first);
        VERIFY(result);
        VERIFY(result->isTrue() == p.second || result->isFalse() == !p.second);
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test complex boolean expressions
void test_complex_expressions(SOMTParser::ParserPtr& parser) {
    std::vector<std::pair<std::string, bool>> cases = {
        {"(and (or true false) (not false))", true},
        {"(or (and true false) (not true))", false},
        {"(=> (or true false) (and true true))", true},
        {"(xor (not false) (and true false))", true},
        {"(and (=> true false) (or false true))", false},
        {"(not (and (or true false) (=> false true)))", false}
    };
    
    std::cout << "=== Testing Complex Boolean Expressions ===" << std::endl;
    for (const auto& p : cases) {
        std::cout << "Expression: " << p.first << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(p.first);
        VERIFY(result);
        VERIFY(result->isTrue() == p.second || result->isFalse() == !p.second);
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "======= Boolean Logic Operations Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_boolean_constants(parser);
    test_logical_operations(parser);
    test_complex_expressions(parser);
    
    return 0;
} 