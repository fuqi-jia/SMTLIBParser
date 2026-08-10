#include <cmath>
#include <iostream>
#include <string>

#include "somtparser/core/kind.h"
#include "somtparser/core/util.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/number.h"
#include "test_helpers.h"

static void assert_fp32_near(const std::shared_ptr<SOMTParser::DAGNode>& ev, float expected, float eps = 5e-4f) {
    VERIFY(ev && !ev->isErr() && ev->isCFP());
    auto v = SOMTParser::fpNodeToFloat32(ev);
    VERIFY(v.has_value());
    VERIFY(std::fabs(*v - expected) < eps);
}

static void eval_expect_fp32(SOMTParser::ParserPtr& parser, SOMTParser::ModelPtr& model, const char* smt, float expected,
                             float eps = 5e-4f) {
    auto e = parser->mkExpr(smt);
    VERIFY(e && !e->isErr());
    auto ev = parser->evaluate(e, model);
    assert_fp32_near(ev, expected, eps);
}

static void eval_expect_bool(SOMTParser::ParserPtr& parser, SOMTParser::ModelPtr& model, const char* smt, bool want_true) {
    auto e = parser->mkExpr(smt);
    VERIFY(e && !e->isErr());
    auto ev = parser->evaluate(e, model);
    VERIFY(ev && !ev->isErr());
    if (want_true) {
        VERIFY(ev->isTrue());
    } else {
        VERIFY(ev->isFalse());
    }
}

/// For operators that do not fold to a single constant, check evaluate preserves shape and constant leaves.
static void eval_expect_structure_unchanged(SOMTParser::ParserPtr& parser, SOMTParser::ModelPtr& model, const char* smt) {
    auto e = parser->mkExpr(smt);
    VERIFY(e && !e->isErr());
    auto ev = parser->evaluate(e, model);
    VERIFY(ev && !ev->isErr());
    VERIFY(ev->getKind() == e->getKind());
    VERIFY(ev->getChildrenSize() == e->getChildrenSize());
    for (size_t i = 0; i < e->getChildrenSize(); ++i) {
        auto a = e->getChild(i);
        auto b = ev->getChild(i);
        VERIFY(a && b);
        if (a->isConst()) {
            VERIFY(b->isConst());
            VERIFY(parser->toString(a) == parser->toString(b));
        }
    }
}

static void assert_bv_unsigned_eq(const std::shared_ptr<SOMTParser::DAGNode>& ev, unsigned long expect_nat) {
    VERIFY(ev && !ev->isErr() && ev->isCBV());
    const std::string bits = ev->toString();
    VERIFY(bits.size() >= 3 && bits[0] == '#' && bits[1] == 'b');
    SOMTParser::Integer n = SOMTParser::BitVectorUtils::bvToNat(bits);
    VERIFY(n.toULong() == expect_nat);
}

static void assert_bv_signed_eq(const std::shared_ptr<SOMTParser::DAGNode>& ev, long expect_s) {
    VERIFY(ev && !ev->isErr() && ev->isCBV());
    const std::string bits = ev->toString();
    VERIFY(bits.size() >= 3 && bits.substr(0, 2) == "#b");
    SOMTParser::Integer s = SOMTParser::BitVectorUtils::bvToInt(bits);
    VERIFY(s.toLong() == expect_s);
}

int main() {
    using namespace SOMTParser;

    ParserPtr parser = newParser();
    ModelPtr model = newModel();

    // Phase A regression: run before other mkExpr/evaluate on the main parser — shared lexer/parser
    // global state can otherwise break a fresh Parser's declare-fun + mkExpr sequence.
    {
        ParserPtr p = newParser();
        if (!p->parseStr("(set-logic ALL)") || !p->parseStr("(declare-fun x_fp () (_ FloatingPoint 8 24))")) {
            std::cerr << "test_evaluate_fp_dispatch: declare-fun setup failed\n";
            return 1;
        }
        auto phi = p->mkExpr("(fp.add RNE x_fp ((_ to_fp 8 24) RNE 1.0))");
        if (!phi || phi->isErr()) {
            std::cerr << "test_evaluate_fp_dispatch: mkExpr fp.add with x_fp failed\n";
            return 1;
        }
        auto ev = p->evaluate(phi, model);
        VERIFY(ev && !ev->isErr());
        VERIFY(ev->getKind() == NODE_KIND::NT_FP_ADD);
        VERIFY(ev->getChildrenSize() == 3u);
        VERIFY(ev->getChild(2)->isCFP());
        assert_fp32_near(ev->getChild(2), 1.0f);
    }

    {
        ParserPtr p = newParser();
        bool ok = p->parseStr(R"(
; Parser smoke test: string/regex ops used by evaluateSimpleOp dispatch (Phase B)
(set-logic ALL)

(declare-const s String)
(declare-const t String)
(assert (= s "aba"))
(assert (= t "aaa"))

; Binary: str.indexof_re
(assert (>= (str.indexof_re s (str.to_re "b")) 0))

; Ternary: str.replace_re / str.replace_re_all
(assert (= (str.replace_re s (str.to_re "a") "x") "xbx"))
(assert (= (str.replace_re_all t (str.to_re "a") "b") "bbb"))

; Ternary: ((_ re.loop m n) Reg) — Reg, then loop bounds as Int children in internal DAG
(assert (str.in_re "xx" ((_ re.loop 1 2) (str.to_re "x"))))

(check-sat)
(exit)
)");
        VERIFY(ok && "eval_dispatch_qf_s inline must parse");
    }
    {
        ParserPtr p = newParser();
        bool ok = p->parseStr(R"(
; Parser smoke test: const array + store/select (evaluates via NT_CONST_ARRAY / array simplification)
(set-logic ALL)

(declare-const i Int)
(declare-const a (Array Int Int))

(assert (= a (store ((as const (Array Int Int)) 0) i 1)))
(assert (= (select ((as const (Array Int Int)) 7) 42) 7))

(check-sat)
(exit)
)");
        VERIFY(ok && "eval_const_array inline must parse");
    }

    // --- Phase A: FP constant folding (expected float32 values) ---
    eval_expect_fp32(parser, model, "(fp.abs ((_ to_fp 8 24) RNE -2.0))", 2.0f);
    eval_expect_fp32(parser, model, "(fp.neg ((_ to_fp 8 24) RNE 3.5))", -3.5f);
    eval_expect_fp32(parser, model, "(fp.add RNE ((_ to_fp 8 24) RNE 1.25) ((_ to_fp 8 24) RNE 2.0))", 3.25f);
    eval_expect_fp32(parser, model, "(fp.sub RNE ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 2.0))", 3.0f);
    eval_expect_fp32(parser, model, "(fp.mul RNE ((_ to_fp 8 24) RNE 2.5) ((_ to_fp 8 24) RNE 4.0))", 10.0f);
    eval_expect_fp32(parser, model, "(fp.div RNE ((_ to_fp 8 24) RNE 9.0) ((_ to_fp 8 24) RNE 3.0))", 3.0f);
    eval_expect_fp32(parser, model,
                     "(fp.fma RNE ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 1.0))", 7.0f);
    eval_expect_fp32(parser, model, "(fp.sqrt RNE ((_ to_fp 8 24) RNE 16.0))", 4.0f);
    eval_expect_fp32(parser, model, "(fp.rem ((_ to_fp 8 24) RNE 7.0) ((_ to_fp 8 24) RNE 3.0))", 1.0f);
    eval_expect_fp32(parser, model, "(fp.roundToIntegral RNE ((_ to_fp 8 24) RNE 3.7))", 4.0f);
    eval_expect_fp32(parser, model, "(fp.min ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 5.0))", 2.0f);
    eval_expect_fp32(parser, model, "(fp.max ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 5.0))", 5.0f);

    eval_expect_bool(parser, model, "(fp.eq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", true);
    eval_expect_bool(parser, model, "(fp.lt ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 4.0))", true);
    eval_expect_bool(parser, model, "(fp.gt ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 4.0))", true);
    eval_expect_bool(parser, model, "(fp.leq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))", true);
    eval_expect_bool(parser, model, "(fp.geq ((_ to_fp 8 24) RNE 4.0) ((_ to_fp 8 24) RNE 3.0))", true);

    eval_expect_bool(parser, model, "(fp.eq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 4.0))", false);
    eval_expect_bool(parser, model, "(fp.lt ((_ to_fp 8 24) RNE 4.0) ((_ to_fp 8 24) RNE 3.0))", false);

    eval_expect_bool(parser, model, "(fp.isNormal ((_ to_fp 8 24) RNE 1.0))", true);
    eval_expect_bool(parser, model, "(fp.isSubnormal (fp #b0 #b00000000 #b00000000000000000000001))", true);
    eval_expect_bool(parser, model, "(fp.isZero ((_ to_fp 8 24) RNE 0.0))", true);
    eval_expect_bool(parser, model, "(fp.isInfinite (_ +oo 8 24))", true);
    eval_expect_bool(parser, model, "(fp.isNaN (_ NaN 8 24))", true);
    eval_expect_bool(parser, model, "(fp.isNegative ((_ to_fp 8 24) RNE -1.0))", true);
    eval_expect_bool(parser, model, "(fp.isPositive ((_ to_fp 8 24) RNE 1.0))", true);

    eval_expect_bool(parser, model, "(fp.isPositive ((_ to_fp 8 24) RNE -1.0))", false);
    eval_expect_bool(parser, model, "(fp.isNegative ((_ to_fp 8 24) RNE 1.0))", false);
    eval_expect_bool(parser, model, "(fp.isZero ((_ to_fp 8 24) RNE 1.0))", false);
    eval_expect_bool(parser, model, "(fp.isNaN ((_ to_fp 8 24) RNE 1.0))", false);
    eval_expect_bool(parser, model, "(fp.isNormal (fp #b0 #b00000000 #b00000000000000000000001))", false);

    // --- Phase B: const_array default value simplifies to integer 3 ---
    {
        auto ca = parser->mkExpr("((as const (Array Int Int)) (+ 1 2))");
        VERIFY(ca && !ca->isErr());
        auto ev = parser->evaluate(ca, model);
        VERIFY(ev && !ev->isErr());
        VERIFY(ev->isConstArray());
        VERIFY(ev->getChildrenSize() >= 1u);
        auto defv = ev->getChild(0);
        VERIFY(defv && defv->isConst());
        VERIFY(parser->toInt(defv) == Integer(3));
    }
    {
        auto ca = parser->mkExpr("((as const (Array Int Int)) 0)");
        VERIFY(ca && !ca->isErr());
        auto ev = parser->evaluate(ca, model);
        VERIFY(ev && !ev->isErr());
        (void)ev;
    }

    // --- Phase B: string / regex dispatch (no constant folding yet): structure + constant leaves preserved ---
    eval_expect_structure_unchanged(parser, model, "(str.indexof_re \"abc\" (str.to_re \"b\"))");
    eval_expect_structure_unchanged(parser, model, "(str.replace_re \"aba\" (str.to_re \"a\") \"x\")");
    eval_expect_structure_unchanged(parser, model, "(str.replace_re_all \"aaa\" (str.to_re \"a\") \"b\")");
    eval_expect_structure_unchanged(parser, model, "((_ re.loop 1 2) (str.to_re \"x\"))");

    // --- Phase C: fp <-> bv / to_fp (exact bit patterns where defined) ---
    {
        auto e = parser->mkExpr("((_ fp.to_ubv 16) RNE ((_ to_fp 8 24) RNE 2.0))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert_bv_unsigned_eq(ev, 2ul);
    }
    {
        auto e = parser->mkExpr("((_ fp.to_sbv 16) RNE ((_ to_fp 8 24) RNE -1.0))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        assert_bv_signed_eq(ev, -1l);
    }
    eval_expect_fp32(parser, model, "((_ to_fp 8 24) RNE 3.14)", 3.14f, 1e-2f);

    std::cout << "test_evaluate_fp_dispatch: all assertions passed\n";
    return 0;
}
