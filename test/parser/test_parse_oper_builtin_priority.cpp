#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"
#include "somtparser/model/model.h"
#include <iostream>
#include "test_helpers.h"

// Issue 1 regression: indexed (_ op ...) uses builtin NODE_KIND (reserved names cannot be declare-fun).
int main() {
    using namespace SOMTParser;

    ParserPtr p = newParser();
    VERIFY(p->parseStr("(set-logic ALL)"));

    p->mkVarInt("x");
    {
        auto e = p->mkExpr("(_ to_real x)");
        VERIFY(e && !e->isErr());
        VERIFY(e->getKind() == NODE_KIND::NT_TO_REAL);
        VERIFY(e->isToReal());
    }

    // Note: do not use (_ bvadd ...): the "_" fast-path treats "bv*" as (_ bvNN width) literals.
    {
        auto e = p->mkExpr("(_ abs x)");
        VERIFY(e && !e->isErr());
        VERIFY(e->getKind() == NODE_KIND::NT_ABS);
    }

    {
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)"));
        auto px = p->mkFunParamVar(SortManager::INT_SORT, "x");
        p->getSymbolManager()->registerFunVar("x", px);
        auto body = p->mkExpr("(ite (= x 0) 5 0)");
        p->getSymbolManager()->eraseFunVar("x");
        auto fd = p->mkFuncDef("f_issue2", {px}, SortManager::INT_SORT, body);
        VERIFY(fd && !fd->isErr());
        auto app = p->mkExpr("(f_issue2 0)");
        VERIFY(app && app->isFuncApplication());
        ModelPtr m = newModel();
        auto r = p->evaluate(app, m);
        VERIFY(r && p->toString(r) == "5");
    }

    std::cout << "test_parse_oper_builtin_priority: ok\n";
    return 0;
}
