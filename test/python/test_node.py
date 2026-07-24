"""Node protocol: children access, kinds, values, equality, lifetime safety."""

import gc
from fractions import Fraction

import pytest

import somtparser as sp


class TestNodeStructure:
    def test_children_protocol(self):
        p = sp.parse("(declare-const x Int)(declare-const y Int)(assert (= (+ x y) 5))")
        eq = p.assertions[0]
        assert eq.kind == "="
        assert len(eq) == 2 == eq.num_children
        # n-ary constructors may canonicalize child order, so identify the
        # children by shape instead of by position
        kids = list(eq)
        add = next(c for c in kids if c.kind == "+")
        five = next(c for c in kids if c.value == 5)
        assert add.num_children == 2
        assert five.value == 5
        assert eq[-1] == kids[-1]  # negative indexing
        with pytest.raises(IndexError):
            _ = eq[2]
        assert [c.kind for c in eq] == [c.kind for c in eq.children]

    def test_leaf_and_internal(self):
        p = sp.Parser()
        x = p.var_int("x")
        e = p.add(x, p.const_int(1))
        assert x.is_leaf and not x.is_internal
        assert e.is_internal and not e.is_leaf

    def test_kind_strings(self):
        p = sp.Parser()
        x, y = p.var_bool("x"), p.var_bool("y")
        assert p.and_(x, y).is_and
        assert p.or_(x, y).is_or
        assert p.not_(x).is_not
        assert p.implies(x, y).is_implies
        assert p.xor_(x, y).is_xor
        assert p.ite(x, y, p.true_()).is_ite

    def test_arith_kind_checks(self):
        p = sp.Parser()
        a, b = p.var_int("a"), p.var_int("b")
        assert p.add(a, b).is_add
        assert p.sub(a, b).is_sub
        assert p.mul(a, b).is_mul
        assert p.le(a, b).is_le
        assert p.lt(a, b).is_lt
        assert p.ge(a, b).is_ge
        assert p.gt(a, b).is_gt
        assert p.gt(a, b).is_arith_comp
        assert p.add(a, b).is_arith_op

    def test_equality_and_hash(self):
        p = sp.Parser()
        x = p.var_int("x")
        e1 = p.gt(x, p.const_int(0))
        e2 = p.gt(x, p.const_int(0))
        e3 = p.lt(x, p.const_int(0))
        assert e1 == e2
        assert hash(e1) == hash(e2)
        assert e1 != e3
        assert e1 is not None
        assert e1 != None  # noqa: E711 - compare against None must not crash

    def test_structural_sharing(self):
        # Equal subterms must be the same DAG node
        p = sp.parse("(declare-const x Int)(assert (> (+ x 1) 0))(assert (< (+ x 1) 9))")
        a0, a1 = p.assertions
        assert a0[0] == a1[0]

    def test_repr_and_str(self):
        p = sp.Parser()
        x = p.var_int("x")
        e = p.gt(x, p.const_int(0))
        assert str(e) == "(> x 0)" == e.to_smt2()
        assert "Node" in repr(e)


class TestNodeValues:
    def test_bool_values(self):
        p = sp.Parser()
        assert p.true_().value is True
        assert p.false_().value is False
        assert p.true_().is_true
        assert p.false_().is_false

    def test_int_values(self):
        p = sp.Parser()
        assert p.const_int(42).value == 42
        assert p.const_int(-7).value == -7
        assert p.const_int(0).value == 0

    def test_bigint_values_exact(self):
        p = sp.Parser()
        big = 2**200 + 12345
        n = p.const_int(big)
        assert n.value == big
        assert p.const_int(str(-big)).value == -big

    def test_real_values(self):
        p = sp.Parser()
        assert p.const_real(1.5).value == 1.5
        assert p.const_real("2.25").value == 2.25

    def test_rational_value_is_fraction(self):
        p = sp.Parser()
        n = p.const_real("1/3")
        assert n.value == Fraction(1, 3)

    def test_bv_values(self):
        p = sp.parse("(declare-const a (_ BitVec 8))(assert (= a #b00001010))")
        bv = p.assertions[0][1]
        assert bv.is_const_bv
        assert bv.value == 10
        assert bv.bit_width == 8

        p2 = sp.parse("(declare-const a (_ BitVec 8))(assert (= a #xFF))")
        assert p2.assertions[0][1].value == 255

    def test_string_values(self):
        p = sp.parse('(declare-const s String)(assert (= s "hello"))')
        lit = p.assertions[0][1]
        assert lit.is_const_str
        assert lit.value == "hello"

    def test_non_const_value_is_none(self):
        p = sp.Parser()
        x = p.var_int("x")
        assert x.value is None
        assert p.add(x, p.const_int(1)).value is None

    def test_var_flags(self):
        p = sp.Parser()
        x = p.var_int("x")
        assert x.is_var and not x.is_const
        assert x.name == "x"
        c = p.const_int(3)
        assert c.is_const and not c.is_var
        assert c.is_numeral and c.is_const_int


class TestNodeSorts:
    def test_node_sort(self):
        p = sp.Parser()
        assert p.var_int("i").sort.is_int
        assert p.var_real("r").sort.is_real
        assert p.var_bool("b").sort.is_bool
        assert p.var_str("s").sort.is_string
        bv = p.var_bv("v", 16)
        assert bv.sort.is_bv and bv.sort.bv_width == 16
        fp = p.var_fp("f", 8, 24)
        assert fp.sort.is_fp
        assert fp.sort.fp_exponent_width == 8
        assert fp.sort.fp_significand_width == 24

    def test_predicate_sorts_are_bool(self):
        p = sp.Parser()
        x = p.var_int("x")
        assert p.gt(x, p.const_int(0)).sort.is_bool


class TestLifetimeSafety:
    def test_node_survives_parser_gc(self):
        p = sp.parse("(declare-const z Int)(assert (> (+ z 1) 5))")
        node = p.assertions[0]
        del p
        gc.collect()
        assert node.to_smt2() == "(> (+ z 1) 5)"
        assert node[0].kind == "+"
        assert node[0][0].name == "z"

    def test_deep_subtree_survives_parser_gc(self):
        p = sp.Parser()
        x = p.var_int("x")
        e = x
        for i in range(100):
            e = p.add(e, p.const_int(i))
        del p, x
        gc.collect()
        # whole 100-deep chain must remain intact
        assert e.kind == "+"
        s = e.to_smt2()
        assert s.count("x") == 1

    def test_model_values_survive_parser_gc(self):
        p = sp.parse("(declare-const x Int)(assert (> x 0))")
        m = p.parse_model("(model (define-fun x () Int 3))")
        del p
        gc.collect()
        assert m["x"].value == 3

    def test_many_parsers_lifecycle(self):
        nodes = []
        for i in range(20):
            p = sp.Parser()
            v = p.var_int(f"v{i}")
            nodes.append(p.gt(v, p.const_int(i)))
            del p
        gc.collect()
        for i, n in enumerate(nodes):
            assert n.to_smt2() == f"(> v{i} {i})"
