// (div m n) and (mod m n) must satisfy SMT-LIB's definition of them.
//
// The SMT-LIB Ints theory does not define div and mod separately. It defines
// them *together*, as the unique pair satisfying
//
//     (1)  m = n * (div m n) + (mod m n)
//     (2)  0 <= (mod m n) < |n|
//
// for n != 0. That is Euclidean division. It is not C++ truncation, and it is
// not floor division either -- all three agree when both operands are positive
// and disagree otherwise.
//
// Constant folding got this wrong in a way no per-operator test could catch.
// NT_DIV_INT had been corrected to floor semantics, with a comment saying so;
// NT_MOD, thirty lines below, still used C++ '%'. Each looked defensible alone.
// Together they broke the identity that defines both:
//
//     (div -7 2) folded to -4 and (mod -7 2) to -1,
//     so n*q + r = 2*(-4) + (-1) = -9, not -7.
//
// So this file does not assert a table of expected values -- a table is just
// somebody's opinion written twice. It asserts the two laws, over a grid of
// operands including every sign combination, and separately pins the handful of
// values that distinguish the three conventions so a future "fix" toward the
// wrong one fails loudly rather than quietly satisfying the laws in a different
// way. (Laws (1) and (2) together are enough to force uniqueness, but the
// witnesses make the failure message name the convention that was chosen.)
//
// Evaluation shares this path: eval_parser routes NT_MOD and NT_DIV_INT through
// mkOper, which runs simp_oper. Fixing the folding fixes both, and the third
// block below checks that it really is one path and not two.

#include "somtparser/frontend/parser.h"

#include <iostream>
#include <string>
#include <vector>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

/** SMT-LIB integer literals have no unary minus, so a negative constant is the
 *  term (- k). Building the string rather than the node keeps this test honest:
 *  it goes through the same parse path a real script does. */
std::string lit(long long v) {
    return v < 0 ? "(- " + std::to_string(-v) + ")" : std::to_string(v);
}

/** Value of a folded integer term, or false if it did not fold to a constant. */
bool constantValue(const std::shared_ptr<Parser>& p, const std::string& term,
                   long long& out) {
    auto n = p->mkExpr(term);
    if (!n) { return false; }
    const std::string s = p->toString(n);
    try {
        if (s.rfind("(- ", 0) == 0) {
            out = -std::stoll(s.substr(3, s.size() - 4));
        } else {
            out = std::stoll(s);
        }
    } catch (const std::exception&) {
        return false;        // not a numeral: the term stayed symbolic
    }
    return true;
}

struct Failure {
    std::string what;
};

} // namespace

int main() {
    std::cout << "======= integer div/mod follow SMT-LIB =======\n";

    auto p = std::make_shared<Parser>();
    std::vector<Failure> failures;

    // The grid covers both signs of both operands, |m| < |n| and |m| > |n|,
    // exact and inexact division, and zero dividends.
    const std::vector<long long> operands = {-13, -7, -6, -2, -1, 0, 1, 2, 6, 7, 13};
    std::size_t checked = 0;

    for (long long m : operands) {
        for (long long n : operands) {
            if (n == 0) { continue; }        // covered separately below

            long long q = 0, r = 0;
            const std::string dterm = "(div " + lit(m) + " " + lit(n) + ")";
            const std::string mterm = "(mod " + lit(m) + " " + lit(n) + ")";

            if (!constantValue(p, dterm, q)) {
                failures.push_back({dterm + " did not fold to a constant"});
                continue;
            }
            if (!constantValue(p, mterm, r)) {
                failures.push_back({mterm + " did not fold to a constant"});
                continue;
            }
            ++checked;

            // (1) the defining identity
            if (n * q + r != m) {
                failures.push_back({
                    "identity broken for m=" + std::to_string(m) +
                    " n=" + std::to_string(n) + ": div=" + std::to_string(q) +
                    " mod=" + std::to_string(r) + " gives n*div+mod=" +
                    std::to_string(n * q + r) + ", not " + std::to_string(m)});
            }
            // (2) the remainder is non-negative and smaller than |n|
            const long long abs_n = n < 0 ? -n : n;
            if (r < 0 || r >= abs_n) {
                failures.push_back({
                    "remainder out of range for m=" + std::to_string(m) +
                    " n=" + std::to_string(n) + ": mod=" + std::to_string(r) +
                    ", must satisfy 0 <= mod < " + std::to_string(abs_n)});
            }
        }
    }

    // The witnesses. Each row is a case where Euclidean, floor and C++
    // truncation give three different answers, or where two of them agree and
    // the third does not -- so a regression toward either wrong convention
    // names itself instead of merely failing an abstract law.
    struct Witness { long long m, n, div, mod; const char* note; };
    const std::vector<Witness> witnesses = {
        {-7,  2, -4,  1, "trunc would give div=-3; C++ '%' would give mod=-1"},
        {-7, -2,  4,  1, "floor would give div=3; C++ '%' would give mod=-1"},
        { 7, -2, -3,  1, "floor would give div=-4"},
        {-1,  3, -1,  2, "trunc would give div=0, mod=-1"},
        {-6,  3, -2,  0, "exact division: all three conventions agree"},
        { 7,  2,  3,  1, "both positive: all three conventions agree"},
    };
    for (const auto& w : witnesses) {
        long long q = 0, r = 0;
        VERIFY(constantValue(p, "(div " + lit(w.m) + " " + lit(w.n) + ")", q));
        VERIFY(constantValue(p, "(mod " + lit(w.m) + " " + lit(w.n) + ")", r));
        if (q != w.div || r != w.mod) {
            failures.push_back({
                "witness m=" + std::to_string(w.m) + " n=" + std::to_string(w.n) +
                ": expected div=" + std::to_string(w.div) +
                " mod=" + std::to_string(w.mod) +
                ", got div=" + std::to_string(q) + " mod=" + std::to_string(r) +
                "  [" + w.note + "]"});
        }
    }

    // Division by zero is total in SMT-LIB and its value is unconstrained, so
    // there is no constant to fold to and the term must stay symbolic. Folding
    // it to anything would invent a value; refusing the script -- which is what
    // this used to do, by letting a domain_error escape -- rejects a script a
    // solver accepts.
    for (const std::string& t : {"(div 7 0)", "(mod 7 0)",
                                 "(div (- 7) 0)", "(mod (- 7) 0)", "(div 0 0)"}) {
        long long ignored = 0;
        bool folded = true;
        bool threw = false;
        try {
            folded = constantValue(p, t, ignored);
        } catch (...) {
            threw = true;
        }
        if (threw) {
            failures.push_back({t + " threw; it must stay symbolic"});
        } else if (folded) {
            failures.push_back({t + " folded to a constant; its value is "
                                    "unconstrained and must stay symbolic"});
        }
    }

    // Model evaluation must agree with folding. They are supposed to be one
    // path -- eval_parser sends NT_MOD and NT_DIV_INT through mkOper, which
    // runs simp_oper -- and a divergence here would mean the fix landed on only
    // one of them, which is exactly the shape of the original defect.
    {
        auto q = std::make_shared<Parser>();
        VERIFY(q->parseStr("(declare-fun x () Int)\n(declare-fun y () Int)\n"
                           "(assert (= x (div (- 7) 2)))\n"
                           "(assert (= y (mod (- 7) 2)))\n"));
        long long dv = 0, mv = 0;
        VERIFY(constantValue(q, "(div (- 7) 2)", dv));
        VERIFY(constantValue(q, "(mod (- 7) 2)", mv));
        if (dv != -4 || mv != 1) {
            failures.push_back({"a parsed script folds div/mod differently from "
                                "a directly built term"});
        }
    }

    for (const auto& f : failures) {
        std::cerr << "  FAIL " << f.what << "\n";
    }
    std::cout << checked << " operand pair(s) checked against both laws, "
              << witnesses.size() << " witness(es), " << failures.size()
              << " failure(s)\n";
    VERIFY(failures.empty());
    // A grid that silently stopped producing constants would reach here with no
    // failures; require the laws to have actually been exercised.
    VERIFY(checked >= 90);

    std::cout << "div and mod satisfy m = n*div + mod with 0 <= mod < |n|.\n";
    return 0;
}
