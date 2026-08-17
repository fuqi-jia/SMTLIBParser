// Parametric datatypes: `(par (X) ...)`, and the sorts it makes.
//
// SMT-LIB 2.6 lets a datatype declaration abstract over sorts:
//
//     (declare-datatype L (par (X) ((nil) (cons (hd X) (tl (L X))))))
//
// which declares ONE datatype and, with it, a family of sorts -- `(L Int)`,
// `(L Bool)`, `(L (L Int))` -- and members whose sorts follow the argument:
// `hd` of an `(L Int)` is an Int, of an `(L Bool)` a Bool. The parser had no
// `par` at all, so a conforming script stopped at its first declaration.
//
// Getting `par` read is the small half. The rest is that `L` is now a sort
// CONSTRUCTOR rather than a sort, and every place that treated a datatype name
// as a complete sort was quietly wrong:
//
//   * a selector's sort is the DECLARED one with the parameters substituted,
//     not the declared one as written -- `hd` returns `X` and must yield Int;
//   * a nullary constructor names a member of every instance at once, so with
//     no argument to infer from there is nothing to fix the instance. SMT-LIB
//     requires `(as nil (L Int))`, and the ascription has to SURVIVE PRINTING:
//     dumping the bare name gave a term this parser had built at a definite
//     sort and could not read back at any;
//   * `L` alone is not a sort, so a variable cannot be declared at it. It was
//     accepted, and dumped as `(declare-fun l () L)` -- an arity-1 name used
//     at arity 0.
//
// Every case below asserts the round trip, because "parses" and "prints
// something a reader accepts" came apart here in three separate places.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

const char* kL =
    "(declare-datatype L (par (X) ((nil) (cons (hd X) (tl (L X))))))\n";

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

/** Parse, dump, re-parse, and require the second dump to equal the first. */
std::string roundTrip(const std::string& script, const char* what) {
    ParserPtr p = newParser();
    if (!p->parseStr(script)) {
        std::cout << "  failed to parse (" << what << "):\n" << script;
        VERIFY(false);
    }
    const std::string out = p->dumpSMT2();
    ParserPtr q = newParser();
    if (!q->parseStr(out)) {
        std::cout << "  dump does not read back (" << what << "):\n" << out;
        VERIFY(false);
    }
    if (q->dumpSMT2() != out) {
        std::cout << "  dump is not stable (" << what << "):\n"
                  << out << "  became:\n" << q->dumpSMT2();
        VERIFY(false);
    }
    return out;
}

/** A script the parser must REFUSE. `parseStr` returns false rather than
 *  throwing, per the parser's contract, but a throw is also a refusal. */
void refuses(const std::string& script, const char* what) {
    ParserPtr p = newParser();
    bool accepted = false;
    try {
        accepted = p->parseStr(script);
    } catch (const std::exception&) {
        accepted = false;
    }
    if (accepted) {
        std::cout << "  accepted, should refuse (" << what << "):\n"
                  << p->dumpSMT2();
        VERIFY(false);
    }
}

} // namespace

int main() {
    std::cout << "======= parametric datatypes =======\n";

    // ---- The declaration itself, in both spellings. -------------------------
    {
        // Singular `declare-datatype` with the parameter list.
        const std::string out = roundTrip(
            std::string(kL) + "(declare-const l (L Int))\n"
            "(assert (= (hd l) 3))\n(check-sat)\n",
            "singular declare-datatype");
        // The arity is 1, and the `par` is what carries it.
        VERIFY(has(out, "(declare-datatypes ((L 1)) ((par (X) "));
        // The parameter is NOT a sort in its own right, so it must not be
        // emitted as one.
        VERIFY(!has(out, "(declare-sort X"));
        // A datatype declaration provides its members; declaring them again
        // beside it makes the script refuse itself.
        VERIFY(!has(out, "(declare-fun hd"));
    }
    {
        // Plural `declare-datatypes`, where the arity in the header and the
        // length of the `par` list are two statements of the same fact.
        const std::string out = roundTrip(
            "(declare-datatypes ((L 1)) ((par (X) ((nil) (cons (hd X) (tl (L X)))))))\n"
            "(declare-const b (L Bool))\n(assert (hd b))\n(check-sat)\n",
            "plural declare-datatypes");
        VERIFY(has(out, "(L 1)"));
    }
    {
        // Two parameters, so nothing here is a one-parameter special case and
        // the substitution has to be positional.
        const std::string out = roundTrip(
            "(declare-datatype P (par (A B) ((mk (fst A) (snd B)))))\n"
            "(declare-const p (P Int Bool))\n"
            "(assert (and (snd p) (= (fst p) 1)))\n(check-sat)\n",
            "two parameters");
        VERIFY(has(out, "(P 2)"));
        VERIFY(has(out, "(par (A B)"));
    }

    // ---- The selector's sort follows the ARGUMENT. --------------------------
    {
        // Both instances in one script. `hd` is used as an Int in one line and
        // as a Bool in the next, and both must typecheck -- which is only
        // possible if the sort comes from the argument rather than from the
        // declaration.
        roundTrip(std::string(kL) +
                  "(declare-const a (L Int))\n(declare-const b (L Bool))\n"
                  "(assert (and (hd b) (= (hd a) 1)))\n(check-sat)\n",
                  "two instances at once");
    }
    {
        // And the negative: `hd` of an `(L Bool)` is not an Int. Without the
        // substitution `hd` would have sort `X` and compare against anything.
        refuses(std::string(kL) + "(declare-const l (L Bool))\n"
                "(assert (= (hd l) 3))\n(check-sat)\n",
                "selector at the wrong instance");
    }
    {
        // Nested: `(L (L Int))`, where the argument is itself an instance.
        roundTrip(std::string(kL) +
                  "(declare-const ll (L (L Int)))\n"
                  "(assert (= (hd (hd ll)) 1))\n(check-sat)\n",
                  "instance as its own argument");
    }

    // ---- `as` on a nullary constructor, and the print that keeps it. -------
    {
        // In the OPERATOR position of an application.
        const std::string out =
            roundTrip(std::string(kL) + "(declare-const l (L Int))\n"
                      "(assert (= l (as nil (L Int))))\n(check-sat)\n",
                      "as-ascribed nullary constructor");
        // This is the assertion the dump used to fail: the ascription is the
        // only thing naming the instance, so printing `nil` loses it.
        VERIFY(has(out, "(as nil (L Int))"));
    }
    {
        // As an ARGUMENT, where the constructor around it is what pins the
        // instance -- the ascription is still required, because `cons` cannot
        // constrain its second argument before knowing its first.
        const std::string out =
            roundTrip(std::string(kL) + "(declare-const l (L Int))\n"
                      "(assert (= l (cons 1 (as nil (L Int)))))\n(check-sat)\n",
                      "ascription nested in an application");
        VERIFY(has(out, "(as nil (L Int))"));
    }
    {
        // Two ascriptions of the SAME constructor at different instances are
        // different terms. Nodes are hash-consed on name and children, and a
        // nullary constructor has neither to tell them apart -- so if the sort
        // did not participate, these would be one node and this would
        // typecheck.
        refuses(std::string(kL) +
                "(assert (= (as nil (L Int)) (as nil (L Bool))))\n(check-sat)\n",
                "equality across two instances");
    }
    {
        // `as` is for constructors. A selector ascribed this way is not a term.
        refuses(std::string(kL) + "(declare-const l (L Int))\n"
                "(assert (= l (as hd (L Int))))\n(check-sat)\n",
                "as applied to a selector");
    }

    // ---- A sort constructor is not a sort. ---------------------------------
    {
        // Used with no arguments.
        refuses(std::string(kL) + "(declare-const l L)\n(check-sat)\n",
                "parametric datatype used bare");
    }
    {
        // Used with too many.
        refuses(std::string(kL) + "(declare-const l (L Int Int))\n(check-sat)\n",
                "parametric datatype over-applied");
    }
    {
        // The parameter is in scope only inside the declaration, so it must
        // not leak out as a declared sort afterwards.
        refuses(std::string(kL) + "(declare-const x X)\n(check-sat)\n",
                "parameter used as a sort outside its declaration");
    }

    // ---- Non-parametric datatypes are untouched. ---------------------------
    {
        // The case every existing script is. `par` adds a form; it must not
        // change the output of a declaration that does not use it, so this is
        // asserted as text.
        const std::string out = roundTrip(
            "(declare-datatype Lst ((nil) (cons (hd Int) (tl Lst))))\n"
            "(declare-const l Lst)\n(assert ((_ is cons) l))\n(check-sat)\n",
            "non-parametric datatype");
        VERIFY(has(out,
                   "(declare-datatypes ((Lst 0)) (((nil) (cons (hd Int) (tl Lst)))))"));
        VERIFY(!has(out, "par"));
        // A nullary constructor of a non-parametric datatype has no instance to
        // name, so it must keep printing bare -- the ascription is added only
        // where it is required.
        const std::string out2 = roundTrip(
            "(declare-datatype Lst ((nil) (cons (hd Int) (tl Lst))))\n"
            "(declare-const l Lst)\n(assert (= l nil))\n(check-sat)\n",
            "non-parametric nullary constructor");
        VERIFY(has(out2, "nil)"));
        VERIFY(!has(out2, "(as nil"));
    }

    std::cout << "All parametric-datatype tests passed." << std::endl;
    return 0;
}
