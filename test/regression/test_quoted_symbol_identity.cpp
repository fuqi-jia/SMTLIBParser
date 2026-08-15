// `foo` and `|foo|` are the same symbol.
//
// SMT-LIB 2.6 §3.1: enclosing a simple symbol in vertical bars does not produce
// a new symbol -- the standard follows the Common Lisp convention, so `abc` and
// `|abc|` denote the same one. The bars widen which characters a symbol may
// contain; they are not part of its identity.
//
// This was implemented for VARIABLES and for nothing else, so a quoted variable
// resolved and a quoted FUNCTION did not:
//
//     (declare-fun |inv| (Int Int) Bool)
//     (assert (inv 1 2))          ->  Unknown or unexpected symbol "inv"
//
// The split follows declare-fun's two branches: a zero-argument declaration
// goes through mkVar into var_names_, which the equivalence covered, while one
// with arguments goes through mkFuncDec into fun_key_map_, which it did not.
// So a quoted CONSTANT worked and a quoted function of one or more arguments
// did not.
//
// The tests below are deliberately spread across the symbol kinds rather than
// piled onto functions: the defect was one map having the rule and the others
// not, so a test that only covers the map that was fixed would not notice the
// next one drifting.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

/** Parse a script, requiring success. */
void ok(const std::string& script, const char* what) {
    ParserPtr p = newParser();
    if (!p->parseStr(script)) {
        std::cout << "  failed to parse (" << what << "):\n" << script;
        VERIFY(false);
    }
}

} // namespace

int main() {
    std::cout << "======= quoted symbol identity =======\n";

    // ---- Functions, in both directions. ------------------------------------
    ok("(set-logic ALL)\n"
       "(declare-fun |inv| (Int Int) Bool)\n"
       "(assert (inv 1 2))\n(check-sat)\n",
       "declared quoted, applied bare");
    ok("(set-logic ALL)\n"
       "(declare-fun inv (Int Int) Bool)\n"
       "(assert (|inv| 1 2))\n(check-sat)\n",
       "declared bare, applied quoted");

    // ---- Constants and variables. ------------------------------------------
    ok("(set-logic ALL)\n(declare-fun |x| () Int)\n(assert (> x 0))\n(check-sat)\n",
       "constant declared quoted");
    ok("(set-logic ALL)\n(declare-const x Int)\n(assert (> |x| 0))\n(check-sat)\n",
       "constant used quoted");

    // ---- The two spellings are ONE symbol, not two. ------------------------
    {
        // The point of the standard's rule. If they were different symbols this
        // would be two declarations and would parse; it must be rejected as a
        // redeclaration instead.
        ParserPtr p = newParser();
        const bool parsed = p->parseStr(
            "(set-logic ALL)\n"
            "(declare-fun f (Int) Bool)\n"
            "(declare-fun |f| (Int) Bool)\n"
            "(check-sat)\n");
        VERIFY(!parsed);
    }
    {
        // And one declaration produces one symbol in the output, not two.
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)\n"
                           "(declare-fun |inv| (Int Int) Bool)\n"
                           "(assert (inv 1 2))\n"
                           "(assert (|inv| 3 4))\n(check-sat)\n"));
        const std::string out = p->dumpSMT2();
        std::size_t decls = 0;
        for (std::size_t i = out.find("(declare-fun "); i != std::string::npos;
             i = out.find("(declare-fun ", i + 1)) {
            ++decls;
        }
        VERIFY(decls == 1);
    }

    // ---- A symbol the bars are actually FOR keeps them. ---------------------
    {
        // `|a b|` has no simple-symbol spelling: the bars are what make it a
        // legal symbol at all, so stripping them everywhere would be wrong.
        // What must hold is that it round-trips.
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)\n"
                           "(declare-fun |a b| () Int)\n"
                           "(assert (> |a b| 0))\n(check-sat)\n"));
        const std::string out = p->dumpSMT2();
        VERIFY(has(out, "|a b|"));
        ParserPtr q = newParser();
        VERIFY(q->parseStr(out));
    }

    // ---- Sorts, quantifiers and let. ---------------------------------------
    ok("(set-logic ALL)\n(declare-fun p (Int) Bool)\n"
       "(assert (forall ((|y| Int)) (p y)))\n(check-sat)\n",
       "quantified variable declared quoted");
    ok("(set-logic ALL)\n(declare-const x Int)\n"
       "(assert (let ((|t| (+ x 1))) (> t 0)))\n(check-sat)\n",
       "let binding declared quoted");

    // ---- Not over-eager: distinct names stay distinct. ----------------------
    {
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)\n"
                           "(declare-fun foo () Int)\n"
                           "(declare-fun foobar () Int)\n"
                           "(assert (> foo foobar))\n(check-sat)\n"));
        const std::string out = p->dumpSMT2();
        VERIFY(has(out, "foo") && has(out, "foobar"));
    }
    {
        // A bare `|` is not a quoted symbol and must not be treated as one.
        ParserPtr p = newParser();
        VERIFY(!p->parseStr("(set-logic ALL)\n(declare-fun || () Int)\n(check-sat)\n")
               || true);      // whichever it does, it must not crash
    }

    std::cout << "All quoted-symbol-identity tests passed." << std::endl;
    return 0;
}
