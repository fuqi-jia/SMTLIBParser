// `str.indexof_re` takes three arguments.
//
// SMT-LIB's Unicode Strings theory declares
//
//     (str.indexof_re String RegLan Int) Int
//
// -- the third argument is the index to start searching from, exactly as
// `str.indexof` takes one. This parser built it with two, so a conforming
// script was a parse failure and a term built through the API named the
// operator at an arity nothing else uses.
//
// The arity table in getArity already said 3. The builder and the dispatch
// said 2. That is the same disagreement between copies that `<=` had, in a
// smaller place: one operator's arity written down more than once, and the
// copies drifting.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

const char* kDecl = "(set-logic ALL)\n(declare-const t String)\n"
                    "(declare-const i Int)\n";

} // namespace

int main() {
    std::cout << "======= str.indexof_re =======\n";

    // ---- Three arguments parse, and survive a round trip. ------------------
    {
        ParserPtr p = newParser();
        const std::string src = std::string(kDecl)
            + "(assert (> (str.indexof_re t (str.to_re \"a\") 0) 0))\n(check-sat)\n";
        if (!p->parseStr(src)) {
            std::cout << "  three arguments did not parse\n";
            VERIFY(false);
        }
        const std::string out = p->dumpSMT2();
        VERIFY(has(out, "str.indexof_re"));
        ParserPtr q = newParser();
        VERIFY(q->parseStr(out));
        VERIFY(q->dumpSMT2() == out);
    }
    {
        // A non-constant start index is ordinary: it is a term, not an index in
        // the `(_ ...)` sense.
        ParserPtr p = newParser();
        VERIFY(p->parseStr(std::string(kDecl)
                           + "(assert (> (str.indexof_re t (str.to_re \"a\") i) 0))\n"
                             "(check-sat)\n"));
    }

    // ---- Two arguments are an error, not a lenient shorthand. --------------
    {
        // Accepting them would leave the start index unstated, and the operator
        // has no default: `str.indexof` does not have one either.
        ParserPtr p = newParser();
        VERIFY(!p->parseStr(std::string(kDecl)
                            + "(assert (> (str.indexof_re t (str.to_re \"a\")) 0))\n"
                              "(check-sat)\n"));
    }
    {
        // ...and the third argument must be an Int, not another regular
        // expression, which is the mistake the two-argument form invited.
        ParserPtr p = newParser();
        VERIFY(!p->parseStr(std::string(kDecl)
                            + "(assert (> (str.indexof_re t (str.to_re \"a\") "
                              "(str.to_re \"b\")) 0))\n(check-sat)\n"));
    }

    // ---- The two neighbours it sits between are unchanged. -----------------
    {
        // str.replace_re and str.replace_re_all were already three-argument;
        // this checks the dispatch edit did not shift them.
        ParserPtr p = newParser();
        VERIFY(p->parseStr(std::string(kDecl)
                           + "(assert (= (str.replace_re t (str.to_re \"a\") t) t))\n"
                             "(assert (= (str.replace_re_all t (str.to_re \"a\") t) t))\n"
                             "(check-sat)\n"));
    }

    std::cout << "All str.indexof_re tests passed." << std::endl;
    return 0;
}
