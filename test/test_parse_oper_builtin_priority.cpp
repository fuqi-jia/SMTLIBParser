#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"
#include "somtparser/model/model.h"
#include <cassert>
#include <iostream>

// Issue 1 regression: indexed (_ op ...) must use builtin NODE_KIND even if declare-fun shadows the name.
int main() {
    using namespace SOMTParser;

    ParserPtr p = newParser();
    assert(p->parseStr("(set-logic ALL)"));

    p->mkVarInt("x");
    {
        auto shadow = p->mkFuncDec("to_real", {SortManager::INT_SORT}, SortManager::REAL_SORT);
        assert(shadow && !shadow->isErr());
        auto e = p->mkExpr("(_ to_real x)");
        assert(e && !e->isErr());
        assert(e->getKind() == NODE_KIND::NT_TO_REAL);
        assert(e->isToReal());
    }

    // Note: do not use (_ bvadd ...): the "_" fast-path treats "bv*" as (_ bvNN width) literals.
    {
        auto shadow_abs = p->mkFuncDec("abs", {SortManager::INT_SORT}, SortManager::INT_SORT);
        assert(shadow_abs && !shadow_abs->isErr());
        auto e = p->mkExpr("(_ abs x)");
        assert(e && !e->isErr());
        assert(e->getKind() == NODE_KIND::NT_ABS);
    }

    {
        ParserPtr p = newParser();
        assert(p->parseStr("(set-logic ALL)"));
        auto px = p->mkFunParamVar(SortManager::INT_SORT, "x");
        p->getSymbolManager()->registerFunVar("x", px);
        auto body = p->mkExpr("(ite (= x 0) 5 0)");
        p->getSymbolManager()->eraseFunVar("x");
        auto fd = p->mkFuncDef("f_issue2", {px}, SortManager::INT_SORT, body);
        assert(fd && !fd->isErr());
        auto app = p->mkExpr("(f_issue2 0)");
        assert(app && app->isFuncApplication());
        ModelPtr m = newModel();
        auto r = p->evaluate(app, m);
        assert(r && p->toString(r) == "5");
    }

    std::cout << "test_parse_oper_builtin_priority: ok\n";
    return 0;
}
