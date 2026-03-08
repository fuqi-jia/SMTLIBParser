#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

// Test string constants
void test_string_constants(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "\"\"",                       // Empty string
        "\"Hello, World!\"",          // Simple string
        "\"String with \"\"quotes\"\"\"", // String with escaped quotes
        "\"String with \\\\backslash\"",  // String with escaped backslash
        "\"Multi-line\nstring\""      // Multi-line string
    };
    
/*
z3:
(
  (define-fun x2 () String
    "String with ""quotes""")
  (define-fun x3 () String
    "String with \\backslash")
  (define-fun x () String
    "")
  (define-fun x4 () String
    "Multi-line\nstring")
  (define-fun x1 () String
    "Hello, World!")
)
cvc5:
(
(define-fun x () String "")
(define-fun x1 () String "Hello, World!")
(define-fun x2 () String "String with ""quotes""")
(define-fun x3 () String "String with \u{5c}\u{5c}backslash")
(define-fun x4 () String "Multi-line\u{5c}nstring")
)
*/
    
    std::cout << "=== Testing String Constants ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test string operations
void test_string_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(str.len \"Hello\")",
        "(str.++ \"Hello, \" \"World!\")",
        "(str.at \"Hello\" 1)",
        "(str.substr \"Hello, World!\" 7 5)",
        "(str.indexof \"Hello, World!\" \"World\" 0)",
        "(str.replace \"Hello, World!\" \"World\" \"Universe\")",
        "(str.prefixof \"Hello\" \"Hello, World!\")",
        "(str.suffixof \"World!\" \"Hello, World!\")",
        "(str.contains \"Hello, World!\" \"World\")"
    };
    
    std::cout << "=== Testing String Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test string comparison operations
void test_string_comparisons(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(= \"Hello\" \"Hello\")",
        "(= \"Hello\" \"World\")",
        "(str.< \"Hello\" \"World\")",
        "(str.<= \"Hello\" \"Hello\")",
        "(str.> \"World\" \"Hello\")",
        "(str.>= \"World\" \"World\")"
    };
    
    std::cout << "=== Testing String Comparison Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        assert(result->isTrue() || result->isFalse());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test regular expression operations
void test_regex_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(str.in_re \"abc\" (re.* (re.range \"a\" \"z\")))",
        "(str.in_re \"123\" (re.+ (re.range \"0\" \"9\")))",
        "(str.in_re \"abc\" (re.union (re.* (re.range \"a\" \"z\")) (re.* (re.range \"0\" \"9\"))))",
        "(str.in_re \"\" re.allchar)",
        "(str.in_re \"abc\" (re.++ (re.range \"a\" \"a\") (re.range \"b\" \"b\") (re.range \"c\" \"c\")))"
    };
    
    std::cout << "=== Testing Regular Expression Operations ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "======= String Operations Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_string_constants(parser);
    test_string_operations(parser);
    test_string_comparisons(parser);
    test_regex_operations(parser);
    
    return 0;
} 