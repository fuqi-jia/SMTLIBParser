#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

// Test strings of different lengths
void test_string_lengths(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> test_strings = {
        "", // Empty string
        "true",
        "false",
        "a_very_long_identifier_name_to_test_buffer_handling",
        "(and (or true false) (not true))",
        std::string(1000, 'a') // String of 1000 'a' characters
    };
    
    for (const auto& str : test_strings) {
        std::cout << "Testing string length: " << str.length() << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkConstStr(str);
        VERIFY(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
    }
}

// Test special characters
void test_special_chars(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> test_strings = {
        "\"quoted string\"",
        // "|symbol with spaces|", -> error
        // ";comment", -> error
        "(and (or true false) (not true))"
    };
    
    for (const auto& str : test_strings) {
        std::cout << "Testing special character: " << str << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(str);
        VERIFY(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
    }
}

int main() {
    std::cout << "======= String Handling Safety Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    std::cout << "\n--- Testing String Lengths ---" << std::endl;
    test_string_lengths(parser);
    
    std::cout << "\n--- Testing Special Characters ---" << std::endl;
    test_special_chars(parser);
    
    return 0;
} 