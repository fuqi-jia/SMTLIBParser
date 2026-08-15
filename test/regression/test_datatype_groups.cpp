// A group of mutually recursive datatypes is one command, and was two.
//
// SMT-LIB declares a whole group in a single `declare-datatypes` precisely so
// that its members may refer to each other: the sort names are all in scope
// before any constructor list is read. `dumpSMT2` emitted one command per SORT,
// which is right for a datatype that stands alone and impossible for a pair
// that does not:
//
//     (declare-datatypes ((T 0)) (((node (kids TL)) ...)))   <- TL not declared
//     (declare-datatypes ((TL 0)) (((tcons (thd T) ...))))
//
// Re-parsing that failed with `Unknown or unexpected symbol "TL"`. The dump
// returned the text and reported nothing, so a script this parser had read
// perfectly came back out unreadable -- by itself, and by anything else.
//
// The fix groups by strongly connected component and emits the groups in
// dependency order. A datatype that depends on nothing keeps its own command,
// so the output of every non-mutual script is byte-for-byte what it was.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

std::size_t count(const std::string& hay, const std::string& needle) {
    std::size_t n = 0;
    for (std::size_t i = hay.find(needle); i != std::string::npos;
         i = hay.find(needle, i + 1)) {
        ++n;
    }
    return n;
}

/** Parse, dump, and require the dump to read back -- which is the property
 *  this file is about, so it is checked on every case rather than once. */
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
    // And is stable: a second dump equals the first, so the grouping does not
    // drift on each pass.
    VERIFY(q->dumpSMT2() == out);
    return out;
}

} // namespace

int main() {
    std::cout << "======= datatype declaration groups =======\n";

    // ---- Mutually recursive: ONE command. ----------------------------------
    {
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((T 0) (TL 0))"
            " (((node (kids TL)) (leaf (val Int))) ((tnil) (tcons (thd T) (ttl TL)))))\n"
            "(declare-const t T)\n(assert ((_ is leaf) t))\n(check-sat)\n",
            "mutual recursion");
        VERIFY(count(out, "(declare-datatypes") == 1);
        VERIFY(has(out, "(T 0) (TL 0)"));
    }
    {
        // Three in a cycle, to check the grouping is not a two-element special
        // case dressed up as a general one.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((A 0) (B 0) (C 0))"
            " (((mkA (toB B))) ((mkB (toC C))) ((mkC (toA A)) (stop))))\n"
            "(declare-const a A)\n(assert ((_ is mkA) a))\n(check-sat)\n",
            "three-cycle");
        VERIFY(count(out, "(declare-datatypes") == 1);
    }

    // ---- Standing alone: unchanged, one command each. -----------------------
    {
        // The case every existing script and every corpus file is, so its
        // output must be exactly what it was before the grouping existed.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((Lst 0)) (((nil) (cons (hd Int) (tl Lst)))))\n"
            "(declare-const l Lst)\n(assert ((_ is cons) l))\n(check-sat)\n",
            "self-recursive");
        VERIFY(count(out, "(declare-datatypes") == 1);
        VERIFY(has(out,
                   "(declare-datatypes ((Lst 0)) (((nil) (cons (hd Int) (tl Lst)))))"));
    }
    {
        // Two that do not refer to each other stay two commands. Merging them
        // would be legal SMT-LIB and would still change the shape of a script
        // that had nothing wrong with it.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((A 0)) (((a1) (a2))))\n"
            "(declare-datatypes ((B 0)) (((b1) (b2))))\n"
            "(declare-const x A)\n(declare-const y B)\n"
            "(assert ((_ is a1) x))\n(check-sat)\n",
            "two independent");
        VERIFY(count(out, "(declare-datatypes") == 2);
    }

    // ---- One-way dependency: the dependency comes FIRST. --------------------
    {
        // Not mutual, so two commands -- but the order is forced, and it is not
        // the declaration order in general. Here it happens to agree; the next
        // case is the one where it does not.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((B 0)) (((b1) (b2))))\n"
            "(declare-datatypes ((A 0)) (((wrap (unwrap B)))))\n"
            "(declare-const x A)\n(assert ((_ is wrap) x))\n(check-sat)\n",
            "dependency in declaration order");
        VERIFY(count(out, "(declare-datatypes") == 2);
        VERIFY(out.find("(B 0)") < out.find("(A 0)"));
    }
    {
        // The same two declared in ONE command, where the group lists A before
        // the B it needs. Split into two, they must come out B first -- the
        // order the reader needs, not the order the writer used.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((A 0) (B 0)) (((wrap (unwrap B))) ((b1) (b2))))\n"
            "(declare-const x A)\n(assert ((_ is wrap) x))\n(check-sat)\n",
            "dependency against declaration order");
        VERIFY(out.find("(B 0)") < out.find("(A 0)"));
    }

    // ---- The members are still not declared twice. -------------------------
    {
        // A datatype declaration registers a function for every constructor,
        // selector and tester. Emitting those as declare-funs beside the
        // datatype makes the script refuse itself as a redeclaration, and the
        // grouping rewrite moved the code that collects their names.
        const std::string out = roundTrip(
            "(set-logic ALL)\n"
            "(declare-datatypes ((T 0) (TL 0))"
            " (((node (kids TL)) (leaf (val Int))) ((tnil) (tcons (thd T) (ttl TL)))))\n"
            "(declare-const t T)\n(assert ((_ is leaf) t))\n(check-sat)\n",
            "no duplicate members");
        VERIFY(!has(out, "(declare-fun node"));
        VERIFY(!has(out, "(declare-fun val"));
        VERIFY(!has(out, "(declare-fun is-leaf"));
    }

    std::cout << "All datatype-group tests passed." << std::endl;
    return 0;
}
