// `((_ divisible n) t)` is the operator SMT-LIB defines; `is_divisible` is not.
//
// SMT-LIB 2.6, theory Ints, declares
//
//     ((_ divisible n) Int Bool)   for all n in N>0
//
// -- an INDEXED operator taking one argument, where n is part of the operator
// rather than an operand. This engine had only a flat two-argument
// `(is_divisible t n)`, under a name the standard does not define, so:
//
//   * a conforming script could not be READ. `((_ divisible 3) x)` reached the
//     two-parameter assert with one parameter and aborted.
//   * a term containing one could not be WRITTEN for anything else to read. It
//     dumped as `(is_divisible x 3)`, which no other solver accepts -- and
//     which, once the standard spelling was added, this parser would itself
//     read back as an application of a two-argument function.
//
// Both spellings are still accepted on input: leniency is deliberate here, and
// a script this project has already produced must keep parsing. Only the OUTPUT
// is now the standard one, because output is what another tool has to read.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

/** Parse, requiring success, and return the script as the engine prints it. */
std::string dump(const std::string& script, const char* what) {
    ParserPtr p = newParser();
    if (!p->parseStr(script)) {
        std::cout << "  failed to parse (" << what << "):\n" << script;
        VERIFY(false);
    }
    return p->dumpSMT2();
}

const char* kDecl = "(set-logic ALL)\n(declare-const x Int)\n";

} // namespace

int main() {
    std::cout << "======= (_ divisible n) =======\n";

    // ---- The standard form parses. -----------------------------------------
    const std::string out =
        dump(std::string(kDecl) + "(assert ((_ divisible 3) x))\n(check-sat)\n",
             "indexed form");
    VERIFY(has(out, "((_ divisible 3) x)"));

    // ---- The flat form still parses, and prints as the standard one. --------
    //
    // Not merely "still accepted": the two spellings must build the SAME term.
    // If they did not, a script written one way and a script written the other
    // would be different problems while looking like the same one.
    const std::string flat =
        dump(std::string(kDecl) + "(assert (is_divisible x 3))\n(check-sat)\n",
             "flat form");
    VERIFY(has(flat, "((_ divisible 3) x)"));
    VERIFY(flat == out);

    // ---- What the dump says must be readable back. -------------------------
    //
    // This is the half that was broken in the direction nobody looks: the old
    // output named an operator that, after this change, resolves to the indexed
    // one and would arrive with the wrong argument count.
    {
        ParserPtr again = newParser();
        VERIFY(again->parseStr(out));
        VERIFY(again->dumpSMT2() == out);
    }

    // ---- The index is not an argument. -------------------------------------
    {
        // `(divisible x 3)` -- the standard name in the FLAT shape -- is
        // accepted by the same leniency that keeps `is_divisible` working, and
        // must build the same term rather than a different two-argument one.
        const std::string mixed =
            dump(std::string(kDecl) + "(assert (divisible x 3))\n(check-sat)\n",
                 "standard name, flat shape");
        VERIFY(mixed == out);
    }
    {
        // A VARIABLE divisor. SMT-LIB has no such operator -- an index is a
        // numeral -- but this engine has always had one in the flat form, where
        // mkIsDivisible checks only that both arguments are Int. What matters
        // is that the extension did not fork: the indexed spelling and the flat
        // one build the same node, so a script written either way asks the same
        // question. The alternative -- accepting both and meaning two different
        // things -- is the failure this whole change is about.
        const std::string decls = "(set-logic ALL)\n(declare-const x Int)\n"
                                  "(declare-const y Int)\n";
        const std::string a = dump(decls + "(assert ((_ divisible y) x))\n(check-sat)\n",
                                   "variable divisor, indexed shape");
        const std::string b = dump(decls + "(assert (is_divisible x y))\n(check-sat)\n",
                                   "variable divisor, flat shape");
        VERIFY(a == b);
        // And it still reads back, which is the property that stops the
        // extension from becoming unwritable now that the printer emits the
        // indexed shape for it.
        ParserPtr again = newParser();
        VERIFY(again->parseStr(a));
        VERIFY(again->dumpSMT2() == a);
    }

    // ---- Distinct divisors stay distinct. ----------------------------------
    {
        // Hash-consing interns structurally equal terms, and the index is part
        // of the structure. If it were dropped on the way in, these two would
        // become one node and one of the two constraints would vanish.
        const std::string two =
            dump(std::string(kDecl) + "(assert ((_ divisible 3) x))\n"
                 "(assert ((_ divisible 5) x))\n(check-sat)\n", "two divisors");
        VERIFY(has(two, "((_ divisible 3) x)"));
        VERIFY(has(two, "((_ divisible 5) x)"));
    }

    std::cout << "All divisible tests passed." << std::endl;
    return 0;
}
