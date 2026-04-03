#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"
#include "somtparser/core/util.h"
#include <cassert>

static std::string dirname_of(const std::string& path) {
    auto p = path.find_last_of("/\\");
    return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

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

    assert(sanitizeKey("a b\nc\t") == std::string("abc"));
    assert(sanitizeKey("no_spaces") == "no_spaces");
    assert(sanitizeKey("") == "");

    {
        ModelPtr m = newModel();
        ParserPtr p = newParser();
        auto v99 = p->mkConstInt(99);
        m->setUF("g", "k1", v99);
        assert(m->hasUF("g"));
        assert(!m->hasUF("h"));
        assert(m->getUF("g", "k1") == v99);
        assert(p->toString(m->getUF("g", "k1")) == "99");
        assert(m->getUF("g", "missing")->isUnknown());
    }

    {
        ParserPtr p = newParser();
        assert(p->parseStr("(set-logic ALL)"));
        auto f_dec = p->mkFuncDec("f", {SortManager::INT_SORT}, SortManager::INT_SORT);
        auto h_dec = p->mkFuncDec("h", {SortManager::INT_SORT, SortManager::INT_SORT}, SortManager::INT_SORT);
        assert(f_dec && !f_dec->isErr());
        assert(h_dec && !h_dec->isErr());
        (void)f_dec;
        (void)h_dec;

        auto app_f = p->mkExpr("(f 1)");
        assert(app_f && app_f->isUFApplication());
        std::string key_f = uf_sym_key(app_f);
        assert(key_f == "uf@@f@@1");

        ModelPtr m = newModel();
        auto slot = p->mkVarInt(key_f);
        m->addVar(slot);
        auto golden = p->mkConstInt(42);
        m->add(slot, golden);

        auto ev = p->evaluate(app_f, m);
        assert(ev && p->toString(ev) == "42");

        auto app_h = p->mkExpr("(h 2 3)");
        assert(app_h && app_h->isUFApplication());
        std::string key_h = uf_sym_key(app_h);
        assert(key_h == "uf@@h@@2@@3");
        auto slot_h = p->mkVarInt(key_h);
        m->addVar(slot_h);
        m->add(slot_h, p->mkConstInt(100));
        auto ev_h = p->evaluate(app_h, m);
        assert(ev_h && p->toString(ev_h) == "100");
    }

    {
        ModelPtr m = newModel();
        ParserPtr p = newParser();
        auto defv = p->mkConstInt(0);
        auto at0 = p->mkConstInt(11);
        m->setArrayDefault("A", defv);
        m->setArrayStore("A", "#b0", at0);
        assert(m->getArraySelect("A", "#b0") == at0);
        assert(m->getArraySelect("A", "#b1") == defv);
        assert(m->getArraySelect("B", "#b0")->isUnknown());
    }

    std::string uf_inst = dirname_of(__FILE__) + "/instances/uf.smt2";
    ParserPtr p2 = newParser();
    assert(p2->parse(uf_inst));

    std::cout << "test_uf_model_api: all assertions passed\n";
    return 0;
}
