// A quantifier binder that shadows an enclosing one keeps its own sort.
//
//     (forall ((x Int)) (and (P x) (forall ((x (_ BitVec 8))) (Q x))))
//
// parsed, printed and reported success as
//
//     (forall ((x Int)) (and (P x) (forall ((x Int))          (Q x))))
//
// so Q -- declared over (_ BitVec 8) -- was applied to an Int. Shadowing is
// legal SMT-LIB, so this was a well-formed input silently turned into a
// different problem, with no diagnostic anywhere.
//
// Two independent causes, and either one alone produces a wrong answer:
//
//   * mkQuantVar looked up an existing binder BY NAME and returned it whatever
//     its sort, so the inner binder simply became the outer one.
//   * popQuantScope erases by name. That is right when nothing was shadowed and
//     wrong when something was: leaving the inner scope removed the outer
//     binding too, and the outer body's remaining occurrences of `x` then
//     resolved as free symbols.
//
// The two groups below separate them: the first is about the inner binder's
// sort, the second about the outer binding still being there afterwards.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

std::string dumpOf(const std::string& src) {
    ParserPtr p = newParser();
    VERIFY(p->parseStr(src));
    return p->dumpSMT2();
}

} // namespace

int main() {
    std::cout << "======= shadowed quantifier binders =======\n";

    // ---- The inner binder keeps its own sort. ------------------------------
    {
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(assert (forall ((x Int)) (and (P x) (forall ((x (_ BitVec 8))) (Q x)))))\n");
        // Both sorts survive. The defect printed the inner binder as Int, so
        // the BitVec 8 vanished from the script entirely and Q was applied to
        // an argument of the wrong sort.
        VERIFY(has(s, "(x (_ BitVec 8))"));
        VERIFY(has(s, "(x Int)"));
    }
    {
        // The reverse nesting, so the fix is not "BitVec wins".
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(assert (forall ((x (_ BitVec 8))) (and (Q x) (forall ((x Int)) (P x)))))\n");
        VERIFY(has(s, "(x (_ BitVec 8))"));
        VERIFY(has(s, "(x Int)"));
    }
    {
        // Shadowing at the SAME sort is legal and must still work -- there the
        // inner binder may legitimately reuse the node.
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(assert (forall ((x Int)) (and (P x) (forall ((x Int)) (P x)))))\n");
        VERIFY(has(s, "(x Int)"));
    }

    // ---- The outer binding survives the inner scope. -----------------------
    {
        // `(P x)` comes AFTER the inner quantifier here, so it is resolved once
        // the inner scope has been left. With popQuantScope erasing rather than
        // restoring, `x` was already gone.
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(assert (forall ((x Int)) (and (forall ((x (_ BitVec 8))) (Q x)) (P x))))\n");
        VERIFY(has(s, "(x (_ BitVec 8))"));
        VERIFY(has(s, "(x Int)"));
    }
    {
        // Three deep, so a fix that restores exactly one level is not enough.
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(declare-fun R (Real) Bool)\n"
            "(assert (forall ((x Int))"
            " (and (forall ((x (_ BitVec 8))) (and (Q x) (forall ((x Real)) (R x))))"
            "      (P x))))\n");
        VERIFY(has(s, "(x Int)"));
        VERIFY(has(s, "(x (_ BitVec 8))"));
        VERIFY(has(s, "(x Real)"));
    }

    // ---- Distinct names were always fine, and must stay so. ----------------
    {
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(assert (forall ((x Int)) (and (P x) (forall ((y (_ BitVec 8))) (Q y)))))\n");
        VERIFY(has(s, "(x Int)"));
        VERIFY(has(s, "(y (_ BitVec 8))"));
    }

    // ---- Sibling scopes, not nested: the second must not inherit the first. -
    {
        const std::string s = dumpOf(
            "(declare-fun P (Int) Bool)\n"
            "(declare-fun Q ((_ BitVec 8)) Bool)\n"
            "(assert (and (forall ((x Int)) (P x))"
            "             (forall ((x (_ BitVec 8))) (Q x))))\n");
        VERIFY(has(s, "(x Int)"));
        VERIFY(has(s, "(x (_ BitVec 8))"));
    }

    std::cout << "All quantifier-shadowing tests passed." << std::endl;
    return 0;
}
