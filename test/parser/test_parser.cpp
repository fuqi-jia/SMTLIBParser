#include <iostream>
#include <string>
#include "somtparser/frontend/parser.h"
#include <cassert>

int main() {
    std::cout << "======= SOMTParser Test Program =======" << std::endl;

    std::cout << "\n--- Testing Boolean Value Parsing ---" << std::endl;
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr("true");
        assert(result && result->isTrue() && !result->isFalse());
        std::cout << "Input: true" << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
        std::cout << "isTrue: yes" << std::endl;
    }
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr("false");
        assert(result && result->isFalse() && !result->isTrue());
        std::cout << "Input: false" << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
        std::cout << "isFalse: yes" << std::endl;
    }

    std::cout << "\n--- Testing Integer Parsing ---" << std::endl;
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr("42");
        assert(result && parser->toString(result) == "42");
        std::cout << "Input: 42" << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
    }

    std::cout << "\n--- Testing Real Number Parsing ---" << std::endl;
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr("3.14");
        assert(result && result->isCReal() &&
               parser->toString(result) == "(/ 157 50)");
        std::cout << "Input: 3.14" << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
    }

    std::cout << "\n--- Testing Expression Parsing ---" << std::endl;
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr("(and true false)");
        assert(result && result->isFalse());
        std::cout << "Input: (and true false)" << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
    }

    return 0;
}
