#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/value.h"
#include "somtparser/model/model.h"
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
        {"((as const (Array Int Int)) 0)", "((as const (Array Int Int)) 0)"},
        {"(store ((as const (Array Int Int)) 0) 1 10)", ""},
        {"(select (store ((as const (Array Int Int)) 0) 1 10) 1)", "10"},
        {"(select (store ((as const (Array Int Int)) 0) 1 10) 2)", "(select (store ((as const (Array Int Int)) 0) 1 10) 2)"},
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
            auto expected = parser->mkExpr(p.second);
            assert(expected && result == expected &&
                   "array operation result mismatch");
        } else {
            assert(got.find("store") != std::string::npos || got.find("select") != std::string::npos || got.find("const") != std::string::npos);
        }
        std::cout << std::endl;
    }
}

// Model API: select uses array default + stored indices
void test_array_model_select_evaluation(SOMTParser::ParserPtr& p) {
    std::cout << "=== Array model + evaluate(select) ===" << std::endl;
    p->parseStr("(set-logic ALL)");
    p->parseStr("(declare-fun A () (Array Int Int))");

    SOMTParser::ModelPtr m = SOMTParser::newModel();
    auto v42 = p->mkExpr("42");
    auto v0 = p->mkExpr("0");
    m->setArrayDefault("A", v0);
    m->setArrayStore("A", "1", v42);

    auto sel = p->mkExpr("(select A 1)");
    assert(sel && !sel->isErr());
    auto ev = p->evaluate(sel, m);
    assert(ev && !ev->isErr());
    assert(ev->isCInt());
    assert(p->toInt(ev) == SOMTParser::Integer(42));

    auto sel2 = p->mkExpr("(select A 2)");
    assert(sel2 && !sel2->isErr());
    auto ev2 = p->evaluate(sel2, m);
    assert(ev2 && !ev2->isErr());
    assert(ev2->isCInt());
    assert(p->toInt(ev2) == SOMTParser::Integer(0));
}

void test_value_array_operators_ir() {
    std::cout << "=== Value class array store/select (IR) ===" << std::endl;
    SOMTParser::Value arr(SOMTParser::ARRAY);
    arr.setArrayDefault(SOMTParser::Value(SOMTParser::Number(SOMTParser::Integer(99))));

    SOMTParser::Value stored =
        arr.store("key1", SOMTParser::Value(SOMTParser::Number(SOMTParser::Integer(42))));
    assert(stored.getType() == SOMTParser::ARRAY);

    SOMTParser::Value selected = stored.select("key1");
    assert(selected.toNumber() == SOMTParser::Number(SOMTParser::Integer(42)));

    SOMTParser::Value def_val = stored.select("key2");
    assert(def_val.getNumberValue() ==
           SOMTParser::Number(SOMTParser::Integer(99)));
}

int main() {
    std::cout << "======= Array Theory Test =======" << std::endl;

    SOMTParser::ParserPtr parser = SOMTParser::newParser();

    test_array_creation(parser);
    test_array_operations(parser);
    test_array_model_select_evaluation(parser);
    test_value_array_operators_ir();

    return 0;
}
