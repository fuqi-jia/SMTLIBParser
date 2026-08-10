// dumpSMT2() must emit a script this parser can read back, and reading it back
// must produce the same script again.  Three defects broke that:
//
//   * datatypes were dumped as plain declare-funs over a sort that was never
//     declared, so re-parsing failed on the undeclared sort name;
//   * declarations were emitted in unordered_map iteration order, so
//     dump -> parse -> dump did not converge;
//   * an input without (set-logic ...) dumped the placeholder UNKNOWN_LOGIC,
//     which is not a logic name any parser accepts.
//
// The cases below deliberately avoid operators whose printing is fixed
// elsewhere, so this file stands on its own.

#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

using namespace SOMTParser;

namespace {

// Dump, re-parse the dump, dump again; require a fixed point.  Returns it.
std::string dumpFixedPoint(const std::string& label, const std::string& script) {
    ParserPtr p = newParser();
    p->parseStr(script);
    const std::string once = p->dumpSMT2();

    ParserPtr q = newParser();
    q->parseStr(once);  // must not reject our own output
    const std::string twice = q->dumpSMT2();

    if (once != twice) {
        std::cerr << "FAILED " << label << ": dump is not a fixed point\n"
                  << "--- first ---\n" << once << "--- second ---\n" << twice;
        std::abort();
    }
    std::cout << "  ok: " << label << "\n";
    return once;
}

void expectContains(const std::string& what, const std::string& hay, const std::string& needle) {
    if (hay.find(needle) == std::string::npos) {
        std::cerr << "FAILED " << what << ": expected to find\n  " << needle
                  << "\nin\n" << hay;
        std::abort();
    }
}

void expectAbsent(const std::string& what, const std::string& hay, const std::string& needle) {
    if (hay.find(needle) != std::string::npos) {
        std::cerr << "FAILED " << what << ": did not expect\n  " << needle
                  << "\nin\n" << hay;
        std::abort();
    }
}

// ── A datatype must be dumped as a datatype. ──────────────────────────────
void test_datatype_dump() {
    std::cout << "-- datatype declarations --\n";
    const std::string dump = dumpFixedPoint(
        "datatype round trip",
        "(declare-datatypes ((L 0)) (((nil) (cons (hd Int) (tl L)))))\n"
        "(declare-fun l () L)\n"
        "(assert (= 1 (hd (cons 1 nil))))\n");

    expectContains("datatype declaration is emitted", dump,
                   "(declare-datatypes ((L 0)) (((nil) (cons (hd Int) (tl L)))))");
    // The constructors, selectors and testers come with the datatype; a
    // declare-fun for any of them would be a redeclaration on re-parse.
    expectAbsent("constructor is not also declared", dump, "(declare-fun nil ");
    expectAbsent("constructor is not also declared", dump, "(declare-fun cons ");
    expectAbsent("selector is not also declared", dump, "(declare-fun hd ");
    expectAbsent("selector is not also declared", dump, "(declare-fun tl ");
    expectAbsent("tester is not also declared", dump, "(declare-fun is-nil ");
    // The datatype-sorted variable is still declared normally.
    expectContains("variable of datatype sort is declared", dump, "(declare-fun l () L)");
    std::cout << "  ok: constructors/selectors/testers are not re-declared\n";

    // Several constructors each carrying selectors.
    dumpFixedPoint("multi-constructor datatype",
                   "(declare-datatypes ((P 0)) (((mk (fst Int) (snd Bool)) (none))))\n"
                   "(declare-fun p () P)\n(assert (= p (mk 1 true)))\n");
}

// ── Declaration order must follow the input, not a hash. ──────────────────
void test_declaration_order_is_stable() {
    std::cout << "-- declaration order --\n";
    const std::string dump = dumpFixedPoint(
        "declaration order round trip",
        "(declare-fun x () (_ BitVec 8))\n"
        "(declare-fun i () Int)\n"
        "(declare-fun a () Bool)\n"
        "(declare-fun s () String)\n"
        "(assert a)\n");

    const size_t px = dump.find("(declare-fun x ");
    const size_t pi = dump.find("(declare-fun i ");
    const size_t pa = dump.find("(declare-fun a ");
    const size_t ps = dump.find("(declare-fun s ");
    VERIFY(px != std::string::npos && pi != std::string::npos);
    VERIFY(pa != std::string::npos && ps != std::string::npos);
    if (!(px < pi && pi < pa && pa < ps)) {
        std::cerr << "FAILED declarations are not in input order:\n" << dump;
        std::abort();
    }
    std::cout << "  ok: declarations follow input order\n";

    // Two parsers fed the same text must produce byte-identical dumps.
    const std::string script =
        "(declare-fun b () Bool)\n(declare-fun c () Int)\n(declare-fun d () Real)\n(assert b)\n";
    ParserPtr p1 = newParser();
    p1->parseStr(script);
    ParserPtr p2 = newParser();
    p2->parseStr(script);
    VERIFY(p1->dumpSMT2() == p2->dumpSMT2());
    std::cout << "  ok: the same input dumps identically twice\n";
}

// ── Uninterpreted sorts and functions. ────────────────────────────────────
void test_declare_sort_and_functions() {
    std::cout << "-- declare-sort / functions --\n";
    const std::string dump = dumpFixedPoint(
        "declare-sort round trip",
        "(declare-sort U 0)\n(declare-fun u () U)\n(declare-fun f (U) Bool)\n(assert (f u))\n");
    expectContains("sort declaration is emitted", dump, "(declare-sort U 0)");
    expectContains("uninterpreted function is emitted", dump, "(declare-fun f ");

    dumpFixedPoint("define-fun round trip",
                   "(define-fun double ((n Int)) Int (* 2 n))\n"
                   "(declare-fun k () Int)\n(assert (= (double k) 4))\n");
}

// ── The logic line must be a logic. ───────────────────────────────────────
void test_logic_line_is_parsable() {
    std::cout << "-- set-logic line --\n";
    ParserPtr p = newParser();
    p->parseStr("(declare-fun q () Bool)\n(assert q)\n");  // no (set-logic ...)
    const std::string dump = p->dumpSMT2();
    expectAbsent("placeholder logic is never emitted", dump, "UNKNOWN_LOGIC");
    // And whatever it does emit has to be readable.
    ParserPtr q = newParser();
    q->parseStr(dump);
    std::cout << "  ok: a script without set-logic dumps a parsable logic\n";

    // An explicitly stated logic survives unchanged.
    const std::string with_logic = dumpFixedPoint(
        "explicit logic round trip",
        "(set-logic QF_LIA)\n(declare-fun n () Int)\n(assert (> n 0))\n");
    expectContains("stated logic is preserved", with_logic, "(set-logic QF_LIA)");
}

}  // namespace

int main() {
    std::cout << "======= dumpSMT2 re-parse tests =======\n";
    test_datatype_dump();
    test_declaration_order_is_stable();
    test_declare_sort_and_functions();
    test_logic_line_is_parsable();
    std::cout << "All dumpSMT2 re-parse tests passed.\n";
    return 0;
}
