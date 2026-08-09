// Regression tests for the fixes and features brought over from the
// SMTStabilizer fork of SOMTParser (https://github.com/shaowei-cai-group/SMTStabilizer).
//
// Each case below either pins a printer/parser bug that used to produce
// unparsable output, or covers a theory/annotation feature that the parser
// previously rejected outright.

#include <iostream>
#include <string>
#include <vector>

#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

using namespace SOMTParser;

namespace {

// Parse a single expression under the given declarations and print it back.
std::string roundTrip(const std::string& decls, const std::string& expr) {
    ParserPtr parser = newParser();
    if (!decls.empty()) parser->parseStr(decls);
    auto node = parser->mkExpr(expr);
    VERIFY(node);
    return parser->toString(node);
}

// Parse a whole script and print back its first assertion.
std::string firstAssertion(const std::string& script, bool preserve_annotations = false) {
    ParserPtr parser = newParser();
    parser->setOption(std::string("preserve_annotations"),
                      std::string(preserve_annotations ? "true" : "false"));
    parser->parseStr(script);
    auto assertions = parser->getAssertions();
    VERIFY(!assertions.empty());
    return parser->toString(assertions[0]);
}

void expectEq(const std::string& what, const std::string& got, const std::string& want) {
    if (got != want) {
        std::cerr << "FAILED " << what << "\n  got : " << got << "\n  want: " << want << "\n";
        std::abort();
    }
    std::cout << "  ok: " << what << " -> " << got << "\n";
}

// ── A1/A2: bv<->int conversions used to print as UNKNOWN_KIND, and the
// indexed forms lost their ((_ ...) ...) shape entirely. ──────────────────
void test_bv_int_conversions() {
    std::cout << "-- bitvector/integer conversions --\n";
    const std::string bv_decl = "(declare-fun x () (_ BitVec 8))";
    const std::string int_decl = "(declare-fun i () Int)";

    expectEq("bv2int", roundTrip(bv_decl, "(bv2int x)"), "(bv2int x)");
    expectEq("bv2nat", roundTrip(bv_decl, "(bv2nat x)"), "(bv2nat x)");
    expectEq("int2bv indexed", roundTrip(int_decl, "((_ int2bv 8) i)"), "((_ int2bv 8) i)");
    // B: the indexed nat2bv form used to abort on a 2-parameter assertion.
    expectEq("nat2bv indexed", roundTrip(int_decl, "((_ nat2bv 8) i)"), "((_ nat2bv 8) i)");

    // C3: SMT-LIB 2.7 spellings.
    expectEq("ubv_to_int", roundTrip(bv_decl, "(ubv_to_int x)"), "(ubv_to_int x)");
    expectEq("sbv_to_int", roundTrip(bv_decl, "(sbv_to_int x)"), "(sbv_to_int x)");
}

// ── A3: the regex sort printed as "(RegEx String)", which the parser's own
// sort reader (which accepts "RegLan") could not read back. ───────────────
void test_reglan_sort_round_trips() {
    std::cout << "-- RegLan sort --\n";
    ParserPtr parser = newParser();
    parser->parseStr("(declare-fun r () RegLan)");
    auto vars = parser->getVariables();
    VERIFY(vars.size() == 1);
    const std::string printed = vars[0]->getSort()->toString();
    expectEq("RegLan sort", printed, "RegLan");

    // The printed spelling must parse back into the same sort.
    ParserPtr again = newParser();
    again->parseStr("(declare-fun r2 () " + printed + ")");
    auto vars2 = again->getVariables();
    VERIFY(vars2.size() == 1 && vars2[0]->getSort()->isReg());
}

// ── A4/C4: datatype tester and updater. ───────────────────────────────────
void test_datatype_tester_and_updater() {
    std::cout << "-- datatype tester/updater --\n";
    const std::string decls =
        "(declare-datatypes ((L 0)) (((nil) (cons (hd Int) (tl L)))))\n"
        "(declare-fun l () L)\n";

    // A4: the tester used to print in the legacy (is-nil l) form, which is not
    // SMT-LIB 2.6 syntax; both spellings must parse and print the standard one.
    expectEq("tester indexed input", roundTrip(decls, "((_ is nil) l)"), "((_ is nil) l)");
    expectEq("tester legacy input", roundTrip(decls, "(is-nil l)"), "((_ is nil) l)");

    // C4: ((_ update <selector>) t v) was rejected outright.
    expectEq("updater", roundTrip(decls, "((_ update hd) l 7)"), "((_ update hd) l 7)");
}

// ── C2: tuple theory was entirely missing; (Tuple Int Int) did not parse. ──
void test_tuple_theory() {
    std::cout << "-- tuple theory --\n";
    const std::string decls =
        "(declare-fun t () (Tuple Int Int Bool))\n"
        "(declare-fun u () UnitTuple)\n";

    ParserPtr parser = newParser();
    parser->parseStr(decls);
    for (auto& v : parser->getVariables()) {
        if (v->getName() == "t") {
            expectEq("tuple sort", v->getSort()->toString(), "(Tuple Int Int Bool)");
            VERIFY(v->getSort()->isTuple());
        }
        if (v->getName() == "u") {
            expectEq("unit tuple sort", v->getSort()->toString(), "UnitTuple");
        }
    }

    expectEq("tuple constructor", roundTrip(decls, "(tuple 1 2 true)"), "(tuple 1 2 true)");
    expectEq("tuple.unit", roundTrip(decls, "tuple.unit"), "tuple.unit");
    expectEq("tuple.select", roundTrip(decls, "((_ tuple.select 0) t)"), "((_ tuple.select 0) t)");
    expectEq("tuple.update", roundTrip(decls, "((_ tuple.update 0) t 5)"), "((_ tuple.update 0) t 5)");
    expectEq("tuple.project", roundTrip(decls, "((_ tuple.project 2 0) t)"), "((_ tuple.project 2 0) t)");

    // Element sorts must follow the tuple's field sorts.
    ParserPtr p2 = newParser();
    p2->parseStr(decls);
    auto sel = p2->mkExpr("((_ tuple.select 2) t)");
    VERIFY(sel && sel->getSort()->isBool());
    auto proj = p2->mkExpr("((_ tuple.project 2 0) t)");
    VERIFY(proj && proj->getSort()->toString() == "(Tuple Bool Int)");
}

// ── C1: quantifier annotations were parsed and then dropped, so triggers
// silently vanished from the printed formula. ─────────────────────────────
void test_quantifier_annotations() {
    std::cout << "-- quantifier annotations --\n";
    const std::string script =
        "(declare-fun f (Int) Int)\n"
        "(assert (forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :qid myq :weight 5)))\n";

    // Default: annotations are dropped, exactly as before.
    expectEq("annotations dropped by default", firstAssertion(script, false),
             "(forall ((x Int)) (> (f x) 0))");

    // Opt in and they survive the round trip.
    expectEq("annotations preserved", firstAssertion(script, true),
             "(forall ((x Int)) (! (> (f x) 0) :pattern ((f x)) :qid myq :weight 5))");

    const std::string no_pattern =
        "(declare-fun f (Int) Int)\n"
        "(assert (forall ((x Int)) (! (> (f x) 1) :no-pattern (f x))))\n";
    expectEq(":no-pattern preserved", firstAssertion(no_pattern, true),
             "(forall ((x Int)) (! (> (f x) 1) :no-pattern (f x)))");

    // Unknown annotations must still be skipped rather than aborting the parse,
    // in both modes.
    const std::string unknown_kw =
        "(declare-fun f (Int) Int)\n"
        "(assert (forall ((x Int)) (! (> (f x) 2) :lblpos lbl :skolemid sk)))\n";
    expectEq("unknown annotations skipped", firstAssertion(unknown_kw, true),
             "(forall ((x Int)) (> (f x) 2))");

    // :named keeps its existing behaviour: recorded, not wrapped.
    ParserPtr parser = newParser();
    parser->setOption(std::string("preserve_annotations"), std::string("true"));
    parser->parseStr("(declare-fun p () Bool)\n(assert (! p :named a0))\n");
    expectEq(":named unchanged", parser->toString(parser->getAssertions()[0]), "p");
}

// ── D: default logic is ALL, and an unknown logic name falls back to ALL
// instead of disabling every theory. ──────────────────────────────────────
void test_logic_defaults() {
    std::cout << "-- logic defaults --\n";
    ParserPtr parser = newParser();
    expectEq("default logic", parser->getOptions()->getLogic(), "ALL");

    VERIFY(parser->getOptions()->setLogic("QF_LIA"));
    expectEq("explicit logic", parser->getOptions()->getLogic(), "QF_LIA");

    VERIFY(!parser->getOptions()->setLogic("QF_MADE_UP"));
    expectEq("unknown logic falls back", parser->getOptions()->getLogic(), "ALL");
}

// ── Commutative operands get a canonical order; equal-hash ties keep input
// order so the AST is reproducible. ───────────────────────────────────────
void test_stable_commutative_order() {
    std::cout << "-- stable commutative ordering --\n";
    const std::string decls = "(declare-fun a () Int)\n(declare-fun b () Int)\n(declare-fun c () Int)\n";
    const std::string one = roundTrip(decls, "(+ a b c)");
    expectEq("(+ a b c) == (+ c b a)", roundTrip(decls, "(+ c b a)"), one);
    expectEq("(+ a b c) == (+ b c a)", roundTrip(decls, "(+ b c a)"), one);

    // Newly recognised symmetric operators.
    const std::string bv_decls = "(declare-fun x () (_ BitVec 4))\n(declare-fun y () (_ BitVec 4))\n";
    expectEq("bvnand symmetric", roundTrip(bv_decls, "(bvnand y x)"), roundTrip(bv_decls, "(bvnand x y)"));
    expectEq("bvxnor symmetric", roundTrip(bv_decls, "(bvxnor y x)"), roundTrip(bv_decls, "(bvxnor x y)"));

    // Quantifiers must NOT be reordered: the bound-variable list stays first.
    ParserPtr parser = newParser();
    parser->parseStr("(declare-fun f (Int Int) Bool)\n(assert (forall ((x Int) (y Int)) (f x y)))\n");
    expectEq("quantifier shape kept", parser->toString(parser->getAssertions()[0]),
             "(forall ((x Int) (y Int)) (f x y))");
}

// ── C5: small utility additions. ──────────────────────────────────────────
void test_utility_additions() {
    std::cout << "-- utility additions --\n";
    VERIFY(BitVectorUtils::bvIsMaxSigned("#b0111"));
    VERIFY(!BitVectorUtils::bvIsMaxSigned("#b1111"));
    VERIFY(BitVectorUtils::bvIsMinSigned("#b1000"));
    VERIFY(!BitVectorUtils::bvIsMinSigned("#b1001"));
    VERIFY(BitVectorUtils::bvIsMaxUnsigned("#b1111"));
    VERIFY(BitVectorUtils::bvIsNegOne("#b1111"));
    VERIFY(!BitVectorUtils::bvIsNegOne("#b1110"));
    VERIFY(BitVectorUtils::bvCompareToUint("#b0101", 5) == 0);
    VERIFY(BitVectorUtils::bvCompareToUint("#b0101", 6) < 0);
    VERIFY(BitVectorUtils::bvCompareToUint("#b0101", 4) > 0);
    // A value wider than 64 bits with a set high bit exceeds any uint64_t.
    VERIFY(BitVectorUtils::bvCompareToUint("#b1" + std::string(64, '0'), UINT64_MAX) > 0);
    expectEq("mkOnes", BitVectorUtils::mkOnes(Integer(4)), "#b1111");

    expectEq("strUnquote", StringUtils::strUnquote("\"abc\""), "abc");
    expectEq("strUnquote unquoted", StringUtils::strUnquote("abc"), "abc");

    // 64-bit unsigned values must survive without narrowing.
    Integer big(static_cast<unsigned long long>(UINT64_MAX));
    expectEq("Integer(unsigned long long)", big.toString(), "18446744073709551615");
    std::cout << "  ok: utility additions\n";
}

}  // namespace

int main() {
    std::cout << "======= SMTStabilizer backport regression tests =======\n";
    test_bv_int_conversions();
    test_reglan_sort_round_trips();
    test_datatype_tester_and_updater();
    test_tuple_theory();
    test_quantifier_annotations();
    test_logic_defaults();
    test_stable_commutative_order();
    test_utility_additions();
    std::cout << "All SMTStabilizer backport tests passed.\n";
    return 0;
}
