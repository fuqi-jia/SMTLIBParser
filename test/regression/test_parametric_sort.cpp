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
        // Applying it to the WRONG instantiation is still accepted, and that is
        // recorded here rather than asserted away: uninterpreted-function
        // argument sorts are not checked at all, so `(f y)` with y at
        // `(P Bool)` is accepted for the same reason `(f b)` is when f takes an
        // Int and b is a Bool. That is a separate gap in mkApplyUF, not
        // something this comparison reaches -- the sorts DO now differ, and
        // nothing asks them to.
        ParserPtr p = newParser();
        VERIFY(p->parseStr(
            std::string(kDecl) + "(declare-fun f ((P Int)) Bool)\n"
            "(declare-const y (P Bool))\n(assert (f y))\n(check-sat)\n"));
    }

    std::cout << "All parametric-sort tests passed." << std::endl;
    return 0;
}
