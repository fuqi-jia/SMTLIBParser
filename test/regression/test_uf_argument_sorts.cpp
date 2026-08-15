// A declaration is a signature, and an application has to honour it.
//
// `applyFun` checked the NUMBER of arguments and not their sorts, so half of
// every signature -- the half that says what the arguments mean -- was carried
// by nothing:
//
//     (declare-fun f (Int) Bool)
//     (declare-const b Bool)
//     (assert (f b))              -- accepted
//
// A wrong-width bit-vector, a String where an Int was declared, a `(P Bool)`
// where a `(P Int)` was declared: all accepted, all reported as success. The
// arity error existed and was checked from the first, which is what made the
// missing half easy to miss -- an application that got the count right looked
// checked.
//
// The rule is Sort::operator==, so the numeric leniency the rest of the parser
// depends on is unchanged. That is the half of this file worth reading: most of
// it asserts what must still be ACCEPTED, because a sort check on the hot path
// of every function application is far likelier to break correct input than to
// miss bad input.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

/** Must parse. */
void ok(const std::string& body, const char* what) {
    ParserPtr p = newParser();
    const std::string s = "(set-logic ALL)\n" + body + "(check-sat)\n";
    if (!p->parseStr(s)) {
        std::cout << "  wrongly REFUSED (" << what << "):\n" << s;
        VERIFY(false);
    }
}

/** Must not parse. */
void no(const std::string& body, const char* what) {
    ParserPtr p = newParser();
    const std::string s = "(set-logic ALL)\n" + body + "(check-sat)\n";
    if (p->parseStr(s)) {
        std::cout << "  wrongly ACCEPTED (" << what << "):\n" << s;
        VERIFY(false);
    }
}

} // namespace

int main() {
    std::cout << "======= uninterpreted function argument sorts =======\n";

    // ---- Refused: the argument does not have the declared sort. -------------
    no("(declare-fun f (Int) Bool)\n(declare-const b Bool)\n(assert (f b))\n",
       "Bool where Int was declared");
    no("(declare-fun f (Int) Bool)\n(declare-const s String)\n(assert (f s))\n",
       "String where Int was declared");
    no("(declare-fun f ((_ BitVec 8)) Bool)\n(declare-const c (_ BitVec 16))\n"
       "(assert (f c))\n",
       "bit-vector of the wrong WIDTH -- the sorts differ only in a number");
    no("(declare-fun f ((_ FloatingPoint 8 24)) Bool)\n"
       "(declare-const g (_ FloatingPoint 11 53)) \n(assert (f g))\n",
       "float of the wrong FORMAT");
    no("(declare-fun f ((Array Int Int)) Bool)\n"
       "(declare-const a (Array Int Bool))\n(assert (f a))\n",
       "array with the wrong element sort");
    no("(declare-fun f (Int Int) Bool)\n(declare-const b Bool)\n"
       "(assert (f 1 b))\n",
       "the SECOND argument wrong -- the loop must not stop at the first");

    // ---- Still accepted, and this is the half that matters. -----------------
    //
    // Sort::operator== decides, so every leniency it already grants survives.
    ok("(declare-fun f (Int) Bool)\n(assert (f 1))\n",
       "an integer literal against an Int parameter");
    ok("(declare-fun f (Real) Bool)\n(assert (f 1))\n",
       "an integer literal against a Real parameter -- a literal carries "
       "IntOrReal and matches both");
    ok("(declare-fun f (Real) Bool)\n(assert (f 1.5))\n",
       "a decimal against a Real parameter");
    ok("(declare-fun f (Int) Bool)\n(declare-const x Int)\n(assert (f (+ x 1)))\n",
       "a computed argument");
    ok("(declare-fun f (Int) Int)\n(declare-fun g (Int) Bool)\n"
       "(declare-const x Int)\n(assert (g (f x)))\n",
       "nested applications");
    ok("(declare-fun f ((Array Int Int)) Bool)\n"
       "(declare-const a (Array Int Int))\n(assert (f (store a 0 1)))\n",
       "an array-valued argument built by store");
    ok("(declare-sort U 0)\n(declare-fun f (U) Bool)\n(declare-const u U)\n"
       "(assert (f u))\n",
       "an uninterpreted sort");
    ok("(declare-fun f (Bool) Bool)\n(declare-const p Bool)\n"
       "(assert (f (and p p)))\n",
       "a Bool-valued argument");
    ok("(declare-fun f (Int) Bool)\n"
       "(assert (forall ((z Int)) (f z)))\n",
       "a quantified variable -- its sort comes from the binder");
    ok("(declare-fun f (Int) Bool)\n(declare-const x Int)\n"
       "(assert (let ((y x)) (f y)))\n",
       "a let-bound argument");
    ok("(define-fun d ((x Int)) Bool (> x 0))\n(assert (d 1))\n",
       "define-fun, which goes through the same path");
    ok("(declare-datatypes ((Lst 0)) (((nil) (cons (hd Int) (tl Lst)))))\n"
       "(declare-const l Lst)\n(assert (= (hd (cons 1 nil)) (hd l)))\n",
       "datatype constructors and selectors, whose signatures are declared "
       "the same way");

    // ---- The arity check still reports arity. -------------------------------
    //
    // Two different mistakes must stay two different diagnostics: a count error
    // reported as a sort error would send a reader to the wrong argument.
    no("(declare-fun f (Int Int) Bool)\n(assert (f 1))\n", "too few arguments");
    no("(declare-fun f (Int Int) Bool)\n(assert (f 1 2 3))\n", "too many arguments");

    std::cout << "All UF argument-sort tests passed." << std::endl;
    return 0;
}
