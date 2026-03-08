#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

// Test array creation and basic operations
void test_array_creation(SOMTParser::ParserPtr& parser) {
    std::cout << "=== Testing Array Creation ===" << std::endl;
    
    std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
    std::shared_ptr<SOMTParser::DAGNode> array = parser->mkArray("testArray", int_sort, int_sort);
    assert(array);
    assert(parser->toString(array).find("testArray") != std::string::npos);
    std::cout << "Created array: " << parser->toString(array) << std::endl;
    std::cout << std::endl;
}

// Test array store and select operations
void test_array_operations(SOMTParser::ParserPtr& parser) {
    std::vector<std::pair<std::string, std::string>> cases = {
        {"((as const (Array Int Int)) 0)", "0"},
        {"(store ((as const (Array Int Int)) 0) 1 10)", ""},
        {"(select (store ((as const (Array Int Int)) 0) 1 10) 1)", "10"},
        {"(select (store ((as const (Array Int Int)) 0) 1 10) 2)", "0"},
        {"(= (select (store ((as const (Array Int Int)) 0) 1 10) 1) 10)", "true"},
        {"(store (store ((as const (Array Int Int)) 0) 1 10) 2 20)", ""}
    };
    
    std::cout << "=== Testing Array Operations ===" << std::endl;
    for (const auto& p : cases) {
        std::cout << "Expression: " << p.first << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(p.first);
        assert(result);
        std::string got = parser->toString(result);
        std::cout << "  Result: " << got << std::endl;
        if (!p.second.empty()) {
            assert(got == p.second && "array operation result mismatch");
        } else {
            assert(got.find("store") != std::string::npos || got.find("select") != std::string::npos || got.find("const") != std::string::npos);
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "======= Array Theory Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_array_creation(parser);
    test_array_operations(parser);
    
    return 0;
} 