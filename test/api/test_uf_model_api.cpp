#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"
#include "somtparser/core/util.h"
#include "test_helpers.h"

static std::string uf_sanitize(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if (c == '(')
            out += "LP";
        else if (c == ')')
            out += "RP";
        else if (c == ' ')
            out += '_';
        else
            out += c;
    }
    return out;
}

static std::string uf_sym_key(const std::shared_ptr<SOMTParser::DAGNode>& app) {
    std::string k = "uf@@" + app->getName();
    for (size_t i = 0; i < app->getChildrenSize(); ++i)
        k += "@@" + uf_sanitize(SOMTParser::dumpSMTLIB2(app->getChild(i)));
    return k;
}

int main() {
    using namespace SOMTParser;

    VERIFY(sanitizeKey("a b\nc\t") == std::string("abc"));
    VERIFY(sanitizeKey("no_spaces") == "no_spaces");
    VERIFY(sanitizeKey("") == "");

    {
        ModelPtr m = newModel();
        ParserPtr p = newParser();
        auto v99 = p->mkConstInt(99);
        m->setUF("g", "k1", v99);
        VERIFY(m->hasUF("g"));
        VERIFY(!m->hasUF("h"));
        VERIFY(m->getUF("g", "k1") == v99);
        VERIFY(p->toString(m->getUF("g", "k1")) == "99");
        VERIFY(m->getUF("g", "missing")->isUnknown());
    }

    {
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)"));
        auto f_dec = p->mkFuncDec("f", {SortManager::INT_SORT}, SortManager::INT_SORT);
        auto h_dec = p->mkFuncDec("h", {SortManager::INT_SORT, SortManager::INT_SORT}, SortManager::INT_SORT);
        VERIFY(f_dec && !f_dec->isErr());
        VERIFY(h_dec && !h_dec->isErr());
        (void)f_dec;
        (void)h_dec;

        auto app_f = p->mkExpr("(f 1)");
        VERIFY(app_f && app_f->isUFApplication());
        std::string key_f = uf_sym_key(app_f);
        VERIFY(key_f == "uf@@f@@1");

        ModelPtr m = newModel();
        auto slot = p->mkVarInt(key_f);
        m->addVar(slot);
        auto golden = p->mkConstInt(42);
        m->add(slot, golden);

        auto ev = p->evaluate(app_f, m);
        VERIFY(ev && p->toString(ev) == "42");

        auto app_h = p->mkExpr("(h 2 3)");
        VERIFY(app_h && app_h->isUFApplication());
        std::string key_h = uf_sym_key(app_h);
        VERIFY(key_h == "uf@@h@@2@@3");
        auto slot_h = p->mkVarInt(key_h);
        m->addVar(slot_h);
        m->add(slot_h, p->mkConstInt(100));
        auto ev_h = p->evaluate(app_h, m);
        VERIFY(ev_h && p->toString(ev_h) == "100");
    }

    {
        ModelPtr m = newModel();
        ParserPtr p = newParser();
        auto defv = p->mkConstInt(0);
        auto at0 = p->mkConstInt(11);
        m->setArrayDefault("A", defv);
        m->setArrayStore("A", "#b0", at0);
        VERIFY(m->getArraySelect("A", "#b0") == at0);
        VERIFY(m->getArraySelect("A", "#b1") == defv);
        VERIFY(m->getArraySelect("B", "#b0")->isUnknown());
    }

    ParserPtr p2 = newParser();
    bool ok = p2->parseStr(R"(
; Parse regression: uninterpreted function applications in assertions
(set-logic ALL)

(declare-fun f (Int) Int)
(declare-fun h (Int Int) Int)

(assert (= (f 0) (f 0)))
(assert (> (+ (f 1) (f 2)) 0))
(assert (= (h 2 3) (h 2 3)))
(assert (not (= (h 1 1) (h 2 2))))

(check-sat)
(exit)
)");
    VERIFY(ok && "uf inline must parse");

    std::cout << "test_uf_model_api: all assertions passed\n";
    return 0;
}
