// `(<= a b c)` is legal SMT-LIB and this parser could not read it.
//
// SMT-LIB 2.6 declares `<`, `<=`, `>` and `>=` with the `:chainable`
// attribute, exactly as it declares `=`: an application with three or more
// arguments means the conjunction of ADJACENT pairs, so
//
//     (<= a b c)   is   (and (<= a b) (<= b c))
//
// The n-ary builders that do that expansion have been in op_parser.cpp from the
// first -- `mkLe(const std::vector<...>&)` and its three siblings -- and
// nothing called them. Both dispatches asserted exactly two parameters, so a
// conforming script was a PARSE FAILURE with no message at all: condAssert
// throws, parseStr catches, and the caller sees `false`.
//
// Two dispatches, because there are two: the frame-based loop in the iterative
// expression parser and `parseOper`. They disagreed with each other and with
// the builders, which is how three copies of one operator's arity drifted
// apart. `getArity`'s table said `binary` too -- a fourth copy, and dead code
// with no callers, fixed here so it does not mislead whoever wires it up.
//
// Adjacent pairs matter, and so does ORDER. These are not commutative: reading
// `(< a b c)` as all-pairs would add `(< a c)`, which is implied and so is
// merely redundant, but reordering the operands would produce a different
// constraint entirely.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

const char* kDecl = "(set-logic ALL)\n(declare-const a Int)\n(declare-const b Int)\n"
                    "(declare-const c Int)\n(declare-const d Int)\n";

std::string dump(const std::string& assertion, const char* what) {
    ParserPtr p = newParser();
    const std::string s = std::string(kDecl) + "(assert " + assertion + ")\n(check-sat)\n";
    if (!p->parseStr(s)) {
        std::cout << "  failed to parse (" << what << "): " << assertion << "\n";
        VERIFY(false);
    }
    return p->dumpSMT2();
}

} // namespace

int main() {
    std::cout << "======= chainable comparisons =======\n";

    // ---- Three and four arguments, all four orderings. ---------------------
    VERIFY(has(dump("(<= a b c)", "<="), "(and (<= a b) (<= b c))"));
    VERIFY(has(dump("(< a b c)", "<"), "(and (< a b) (< b c))"));
    VERIFY(has(dump("(>= a b c)", ">="), "(and (>= a b) (>= b c))"));
    VERIFY(has(dump("(> a b c)", ">"), "(and (> a b) (> b c))"));
    VERIFY(has(dump("(< a b c d)", "< four"), "(and (< a b) (< b c) (< c d))"));

    // ---- The order of the operands is preserved. ----------------------------
    {
        // The property that makes this an expansion rather than a rewrite.
        // `(< a b c)` and `(< c b a)` are different constraints, and a chain
        // built from a canonicalised operand list would confuse them -- which
        // is a risk for these and not for `=`, because `=` is symmetric.
        const std::string up = dump("(< a b c)", "ascending");
        const std::string down = dump("(< c b a)", "descending");
        VERIFY(has(up, "(< a b)") && has(up, "(< b c)"));
        VERIFY(has(down, "(< c b)") && has(down, "(< b a)"));
        VERIFY(up != down);
    }

    // ---- Two arguments are unchanged. --------------------------------------
    {
        // The binary case must not grow an `and` around it: every existing
        // script is this case, and a wrapper would change the shape of every
        // term in the corpus.
        const std::string s = dump("(<= a b)", "binary");
        VERIFY(has(s, "(assert (<= a b))"));
    }

    // ---- One argument is still an error. ------------------------------------
    {
        ParserPtr p = newParser();
        VERIFY(!p->parseStr(std::string(kDecl) + "(assert (<= a))\n(check-sat)\n"));
    }

    // ---- What it emits, it reads back. -------------------------------------
    {
        const std::string out = dump("(< a b c d)", "round trip");
        ParserPtr again = newParser();
        VERIFY(again->parseStr(out));
        VERIFY(again->dumpSMT2() == out);
    }

    // ---- Mixed with the two that already worked. ---------------------------
    {
        // `=` and `distinct` were wired to their n-ary forms from the first, so
        // this checks the four new ones compose with them rather than only
        // working alone.
        const std::string s = dump("(and (<= a b c) (= a b c) (distinct c d))", "mixed");
        VERIFY(has(s, "(<= a b)"));
        ParserPtr p = newParser();
        VERIFY(p->parseStr(s));
    }
    {
        // Reals, and a mixed Int/Real chain -- the numeric leniency the rest of
        // the parser depends on applies to the expanded pairs too.
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(set-logic ALL)\n(declare-const r Real)\n"
                           "(declare-const s Real)\n(declare-const i Int)\n"
                           "(assert (<= r s 3.5))\n(assert (< i r 9))\n(check-sat)\n"));
    }

    std::cout << "All chainable-comparison tests passed." << std::endl;
    return 0;
}
