#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include <cassert>

static std::string dirname_of(const std::string& path) {
    auto p = path.find_last_of("/\\");
    return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

int main() {
    using namespace SOMTParser;

    std::string instance = dirname_of(__FILE__) + "/instances/datatypes.smt2";

    ParserPtr parser = newParser();
    bool parsed = parser->parse(instance);
    if (!parsed) {
        parser = newParser();
        assert(parser->parseStr("(set-logic ALL)"));
        assert(parser->parseStr(
            "(declare-datatypes ((Either 0)) (((left (lv Int)) (right (rv Int)))))"));
        std::cerr << "note: parseStr fallback (could not read " << instance << ")\n";
    } else {
        std::cout << "parsed " << instance << "\n";
    }

    ModelPtr model = newModel();

    {
        auto e = parser->mkExpr("(left 7)");
        assert(e && !e->isErr());
        assert(e->isConstructorApp());
        assert(e->getName() == "left");
        auto ev = parser->evaluate(e, model);
        assert(ev && ev->isConstructorApp());
        assert(ev->getName() == "left");
        assert(ev->getChildrenSize() == 1);
        assert(parser->toString(ev->getChild(0)) == "7");
    }

    {
        auto e = parser->mkExpr("(lv (left 7))");
        assert(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert(ev && parser->toString(ev) == "7");
    }

    {
        auto e = parser->mkExpr("(is-left (left 7))");
        assert(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert(ev && ev->isTrue());
    }

    {
        auto e = parser->mkExpr("(is-right (left 7))");
        assert(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert(ev && ev->isFalse());
    }

    {
        auto e = parser->mkExpr("(lv (right 3))");
        assert(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert(ev && !parser->toString(ev).empty());
        assert(parser->toString(ev) != "3");
    }

    std::cout << "test_datatype_eval: all assertions passed\n";
    return 0;
}
