// `(declare-sort P 1)` makes P a sort CONSTRUCTOR, so `(P Int)` and `(P Bool)`
// are two sorts.
//
// SMT-LIB 2.6 §3.6: declare-sort takes an arity, and a sort term applies the
// constructor to that many sorts. `(P Int)` and `(P Bool)` are then as different
// as `(Array Int Int)` and `(Array Int Bool)`.
//
// parseSort already instantiated them correctly -- it clones the declared sort
// and fills `children` with the arguments -- and two places downstream then
// dropped the arguments again:
//
//   * Sort::operator== compared declared sorts by NAME AND ARITY ONLY, so the
//     two compared equal. `(= x y)` across them type-checked, and a script the
//     standard forbids parsed with no diagnostic.
//   * Sort::toString printed the bare name, so a variable of `(P Int)` dumped
//     as `(declare-fun x () P)` -- a sort constructor used with no arguments,
//     which this parser happens to read back and no other tool accepts.
//
// The array and set branches of operator== compare their children for exactly
// this reason. This file asserts the same property for a declared constructor,
// in both directions: same arguments must be the same sort, different arguments
// must not be.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

const char* kDecl = "(set-logic ALL)\n(declare-sort P 1)\n";

std::string dump(const std::string& script, const char* what) {
    ParserPtr p = newParser();
    if (!p->parseStr(script)) {
        std::cout << "  failed to parse (" << what << "):\n" << script;
        VERIFY(false);
    }
    return p->dumpSMT2();
}

} // namespace

int main() {
    std::cout << "======= parametric declare-sort =======\n";

    // ---- Same arguments: one sort. -----------------------------------------
    const std::string out = dump(
        std::string(kDecl) + "(declare-const x (P Int))\n(declare-const z (P Int))\n"
        "(assert (= x z))\n(check-sat)\n", "(P Int) = (P Int)");
    // The application survives into the output. Printing `P` here would name a
    // constructor at the wrong arity.
    VERIFY(has(out, "(declare-fun x () (P Int))"));
    VERIFY(has(out, "(declare-sort P 1)"));

    // ---- Different arguments: different sorts. ------------------------------
    {
        ParserPtr p = newParser();
        const bool parsed = p->parseStr(
            std::string(kDecl) + "(declare-const x (P Int))\n"
            "(declare-const y (P Bool))\n(assert (= x y))\n(check-sat)\n");
        VERIFY(!parsed);
    }

    // ---- The output reads back, and is stable. ------------------------------
    //
    // Not just "parses": a second dump must equal the first. A sort that prints
    // one way and reads back as another would round-trip once and drift on the
    // next pass.
    {
        ParserPtr again = newParser();
        VERIFY(again->parseStr(out));
        VERIFY(again->dumpSMT2() == out);
    }

    // ---- Nesting, and arity greater than one. -------------------------------
    {
        const std::string nested = dump(
            "(set-logic ALL)\n(declare-sort P 1)\n(declare-sort Q 2)\n"
            "(declare-const a (P (P Int)))\n"
            "(declare-const b (Q Int (P Bool)))\n"
            "(assert (= a a))\n(assert (= b b))\n(check-sat)\n", "nested");
        VERIFY(has(nested, "(P (P Int))"));
        VERIFY(has(nested, "(Q Int (P Bool))"));
        ParserPtr p = newParser();
        VERIFY(p->parseStr(nested));
    }
    {
        // `(P (P Int))` and `(P Int)` differ, which only holds if the
        // comparison recurses rather than stopping at the outermost name.
        ParserPtr p = newParser();
        VERIFY(!p->parseStr(
            std::string(kDecl) + "(declare-const a (P (P Int)))\n"
            "(declare-const b (P Int))\n(assert (= a b))\n(check-sat)\n"));
    }

    // ---- Arity 0 is unchanged. ---------------------------------------------
    {
        // The ordinary uninterpreted sort has no children, so it must still
        // print as a bare name and still compare by name alone.
        const std::string plain = dump(
            "(set-logic ALL)\n(declare-sort U 0)\n(declare-const u U)\n"
            "(declare-const v U)\n(assert (= u v))\n(check-sat)\n", "arity 0");
        VERIFY(has(plain, "(declare-fun u () U)"));
        VERIFY(!has(plain, "(U)"));
    }

    // ---- A constructor still works as a function's sort. --------------------
    {
        const std::string uf = dump(
            std::string(kDecl) + "(declare-fun f ((P Int)) (P Bool))\n"
            "(declare-const x (P Int))\n(assert (= (f x) (f x)))\n(check-sat)\n",
            "uninterpreted function over applied sorts");
        VERIFY(has(uf, "(declare-fun f ((P Int)) (P Bool))"));
        ParserPtr p = newParser();
        VERIFY(p->parseStr(uf));
    }
    {
        // ...and applying it to the WRONG instantiation is refused. This needs
        // both halves: the two sorts must differ (this file's subject) AND the
        // application must consult the declared parameter sort, which is
        // test_uf_argument_sorts.cpp's.
        ParserPtr p = newParser();
        VERIFY(!p->parseStr(
            std::string(kDecl) + "(declare-fun f ((P Int)) Bool)\n"
            "(declare-const y (P Bool))\n(assert (f y))\n(check-sat)\n"));
    }

    // ---- The same rules through the API, which text cannot reach. -----------
    //
    // Every case above goes through parseStr, so it only ever sees sorts that
    // parseSort built. A caller working through the C++ API takes a different
    // path into the same rules, and until mkSortApp existed it could not take
    // it at all: mkSortDec gives back a CONSTRUCTOR, and there was no way to
    // apply it short of reaching into Sort::children.
    {
        ParserPtr p = newParser();
        auto S = p->mkSortDec("S", 1);
        VERIFY(S != nullptr);
        auto s_bool = p->mkSortApp(S, {p->mkBoolSort()});
        auto s_int  = p->mkSortApp(S, {p->mkIntSort()});
        auto s_bool2 = p->mkSortApp(S, {p->mkBoolSort()});
        VERIFY(s_bool && s_int && s_bool2);

        // The rule, on the path the round trip does not cover.
        VERIFY(*s_bool == *s_bool2);
        VERIFY(!(*s_bool == *s_int));
        VERIFY(s_bool->toString() == "(S Bool)");
        VERIFY(s_int->toString() == "(S Int)");
        // The constructor is not one of its own applications.
        VERIFY(!(*S == *s_bool));

        // Nesting, through the API.
        auto s_s_bool = p->mkSortApp(S, {s_bool});
        VERIFY(s_s_bool->toString() == "(S (S Bool))");
        VERIFY(!(*s_s_bool == *s_bool));

        // Applying the constructor twice must not accumulate arguments: the
        // builder clones, because filling the interned constructor's children
        // would make every later instantiation see the previous one's.
        VERIFY(S->children.empty());
        VERIFY(s_bool->children.size() == 1);
    }
    {
        // Arity is checked, and arity 0 hands back the constructor rather than
        // a clone that would compare equal and duplicate.
        ParserPtr p = newParser();
        auto S = p->mkSortDec("S", 1);
        VERIFY(p->mkSortApp(S, {})->isNull());
        VERIFY(p->mkSortApp(S, {p->mkIntSort(), p->mkIntSort()})->isNull());
        auto U = p->mkSortDec("U", 0);
        VERIFY(p->mkSortApp(U, {}) == U);
    }
    {
        // A model built entirely through the API dumps a script that says what
        // the model said -- which is the property an API caller depends on and
        // the text tests above cannot check, since they start from text.
        ParserPtr p = newParser();
        auto S = p->mkSortDec("S", 1);
        auto s_bool = p->mkSortApp(S, {p->mkBoolSort()});
        auto s_int  = p->mkSortApp(S, {p->mkIntSort()});
        auto a = p->mkVar(s_bool, "a");
        auto b = p->mkVar(s_bool, "b");
        auto c = p->mkVar(s_int, "c");
        // Parser::assert is a member function and parser.h undefines the macro
        // (CLAUDE.md invariant 5), so this is the ordinary call it looks like.
        VERIFY(p->assert(p->mkEq(a, b)));
        const std::string out = p->dumpSMT2();
        VERIFY(has(out, "(declare-sort S 1)"));
        VERIFY(has(out, "(declare-fun a () (S Bool))"));
        VERIFY(has(out, "(declare-fun c () (S Int))"));
        ParserPtr q = newParser();
        if (!q->parseStr(out)) {
            std::cout << "  API-built script does not parse:\n" << out;
            VERIFY(false);
        }
    }

    std::cout << "All parametric-sort tests passed." << std::endl;
    return 0;
}
