"""Model parsing, dict protocol, and formula evaluation."""

import pytest

import somtparser as sp


@pytest.fixture
def lia_parser():
    return sp.parse("""
(set-logic QF_LIA)
(declare-const x Int)
(declare-const y Int)
(assert (> x 0))
(assert (< y 10))
""")


class TestModelProtocol:
    def test_parse_model_and_getitem(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int 3)(define-fun y () Int 4))")
        assert len(m) == 2
        assert m["x"].value == 3
        assert m["y"].value == 4
        assert "x" in m and "zzz" not in m
        with pytest.raises(KeyError):
            _ = m["zzz"]

    def test_get_with_default(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int 3))")
        assert m.get("x").value == 3
        assert m.get("nope") is None
        assert m.get("nope", 7) == 7

    def test_keys_values_items(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int 3)(define-fun y () Int 4))")
        assert set(m.keys()) == {"x", "y"}
        assert sorted(v.value for v in m.values()) == [3, 4]
        assert dict((k, v.value) for k, v in m.items()) == {"x": 3, "y": 4}

    def test_manual_model(self):
        p = sp.Parser()
        p.var_int("a")
        m = sp.Model()
        assert m.is_empty
        m.add("a", p.const_int(11))
        assert m["a"].value == 11
        assert not m.is_empty

    def test_negative_values(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int (- 5)))")
        assert m["x"].value == -5

    def test_various_solver_output_styles(self, lia_parser):
        # cvc5-style without "model" keyword
        m = lia_parser.parse_model("((define-fun x () Int 1)(define-fun y () Int 2))")
        assert m["x"].value == 1 and m["y"].value == 2


class TestEvaluate:
    def test_evaluate_full_model(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int 3)(define-fun y () Int 4))")
        assert lia_parser.evaluate(lia_parser.assertions[0], m).value is True
        assert lia_parser.evaluate(lia_parser.assertions[1], m).value is True
        e = lia_parser.expr("(+ x y)")
        assert lia_parser.evaluate(e, m).value == 7

    def test_evaluate_false(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int (- 1))(define-fun y () Int 4))")
        assert lia_parser.evaluate(lia_parser.assertions[0], m).value is False

    def test_evaluate_partial_model(self, lia_parser):
        m = lia_parser.parse_model("(model (define-fun x () Int 3))")
        # y unknown: (< y 10) can not be fully evaluated; result is a node,
        # not a crash
        r = lia_parser.evaluate(lia_parser.assertions[1], m)
        assert r is not None
        assert r.value is not False

    def test_evaluate_arith_expr(self):
        p = sp.parse("(declare-const a Int)(declare-const b Int)")
        m = p.parse_model("(model (define-fun a () Int 6)(define-fun b () Int 7))")
        assert p.evaluate(p.expr("(* a b)"), m).value == 42
        assert p.evaluate(p.expr("(- a b)"), m).value == -1
        assert p.evaluate(p.expr("(ite (< a b) a b)"), m).value == 6

    def test_evaluate_bool_structure(self):
        p = sp.parse("(declare-const p1 Bool)(declare-const p2 Bool)")
        m = p.parse_model(
            "(model (define-fun p1 () Bool true)(define-fun p2 () Bool false))")
        assert p.evaluate(p.expr("(and p1 (not p2))"), m).value is True
        assert p.evaluate(p.expr("(xor p1 p2)"), m).value is True
        assert p.evaluate(p.expr("(=> p1 p2)"), m).value is False

    def test_evaluate_none_model_raises(self, lia_parser):
        with pytest.raises((TypeError, ValueError)):
            lia_parser.evaluate(lia_parser.assertions[0], None)

    def test_evaluate_rational(self):
        p = sp.parse("(declare-const r Real)")
        m = p.parse_model("(model (define-fun r () Real (/ 1 2)))")
        v = p.evaluate(p.expr("(+ r r)"), m)
        assert v.value == 1 or float(v.value) == 1.0

    def test_evaluate_precision_options(self):
        p = sp.parse("(declare-const r Real)")
        p.set_evaluate_precision(128)
        p.set_evaluate_use_floating(False)
        m = p.parse_model("(model (define-fun r () Real 2.0))")
        r = p.evaluate(p.expr("(* r r)"), m)
        assert float(r.value) == 4.0


class TestGetModelFromInput:
    def test_solver_output_is_not_input_language(self):
        # Solver responses ("sat", "(model ...)") are not SMT-LIB commands;
        # feeding them back as input is a parse error. parse_model() is the
        # supported channel for solver model text.
        with pytest.raises(sp.ParseError):
            sp.parse("""
(set-logic QF_LIA)
(declare-const x Int)
(assert (> x 0))
(check-sat)
sat
(model (define-fun x () Int 5))
""")
        p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
        m = p.parse_model("(model (define-fun x () Int 5))")
        assert m["x"].value == 5

    def test_get_model_none_when_absent(self):
        p = sp.parse("(set-logic QF_LIA)")
        m = p.get_model()
        assert m is None or len(m) == 0


class TestUFModel:
    def test_uf_application_stays_symbolic(self):
        # parse_model records function DECLARATIONS from define-fun entries
        # with parameters but does not interpret their bodies, so an
        # uninterpreted-function application is left symbolic while its
        # arguments are still substituted.
        p = sp.parse("""
(declare-fun f (Int) Int)
(declare-const x Int)
(assert (= (f x) 1))
""")
        m = p.parse_model("""
(model
  (define-fun x () Int 2)
  (define-fun f ((a Int)) Int (ite (= a 2) 1 0))
)
""")
        r = p.evaluate(p.assertions[0], m)
        assert r.value is None          # not fully evaluated
        text = r.to_smt2()
        assert "(f 2)" in text          # argument x was substituted
