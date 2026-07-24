"""Robustness: adversarial inputs must raise Python exceptions, never crash."""

import gc

import pytest

import somtparser as sp


class TestMalformedInput:
    @pytest.mark.parametrize("text", [
        "(",
        ")",
        "(assert",
        "(assert)",
        "(assert (> x))",
        "(declare-const)",
        "(declare-const x)",
        "(declare-const x NoSort)",
        "(assert (and (> 1 0) 5))",
        "(assert (bvadd #b01 #b0011))",
        "(assert (= 1 true))",
        "((((((",
        "(assert (let ((x)) x))",
    ])
    def test_bad_input_raises_not_crashes(self, text):
        # NOTE: pytest.fail is used (not a sentinel exception) so that the
        # failure cannot be swallowed by the pytest.raises block itself.
        p = sp.Parser()
        try:
            p.parse_string(text)
        except (sp.ParseError, ValueError):
            return
        pytest.fail("parser accepted malformed input: " + text)

    def test_empty_input_ok(self):
        p = sp.Parser()
        p.parse_string("")
        p.parse_string("   \n\t  ")
        p.parse_string("; just a comment\n")
        assert len(p.assertions) == 0

    def test_unicode_and_long_symbols(self):
        p = sp.Parser()
        p.parse_string("(declare-const |weird symbol +| Int)(assert (> |weird symbol +| 0))")
        assert len(p.assertions) == 1
        long_name = "v" * 5000
        p2 = sp.Parser()
        p2.parse_string(f"(declare-const {long_name} Int)(assert (> {long_name} 0))")
        assert p2.assertions[0][0].name == long_name


class TestNumericEdgeCases:
    def test_huge_integers(self):
        p = sp.Parser()
        huge = 10**100
        n = p.const_int(huge)
        assert n.value == huge
        m = p.parse_model(f"(model (define-fun h () Int {huge}))")
        assert m["h"].value == huge

    def test_const_int_rejects_junk(self):
        p = sp.Parser()
        with pytest.raises((TypeError, ValueError, sp.ParseError)):
            p.const_int("not-a-number")
        with pytest.raises(TypeError):
            p.const_int([1, 2])
        with pytest.raises(TypeError):
            p.const_int(None)

    def test_arith_on_huge_numbers_folds_exactly(self):
        p = sp.Parser()
        a, b = 2**100, 3**80
        r = p.add(p.const_int(a), p.const_int(b))
        assert r.value == a + b

    def test_zero_width_bv_rejected(self):
        p = sp.Parser()
        with pytest.raises(ValueError):
            p.var_bv("z", 0)
        with pytest.raises(ValueError):
            p.bv_sort(0)
        with pytest.raises(ValueError):
            p.const_bv(0, 0)


class TestApiMisuse:
    def test_wrong_arg_types_raise_typeerror(self):
        p = sp.Parser()
        x = p.var_int("x")
        with pytest.raises(TypeError):
            p.add(x, 5)  # raw int is not a Node
        with pytest.raises(TypeError):
            p.and_("true", "false")

    def test_sort_mismatch_raises(self):
        p = sp.Parser()
        x, s = p.var_int("x"), p.var_str("s")
        with pytest.raises((ValueError, sp.ParseError)):
            p.add(x, s)

    def test_empty_arg_lists(self):
        p = sp.Parser()
        for f in (p.and_, p.or_, p.add):
            try:
                r = f([])
                # empty n-ary application either simplifies or errors, but
                # must not return a broken node silently
                assert r is None or not r.is_err
            except (ValueError, sp.ParseError, TypeError):
                pass

    def test_assert_non_bool_raises(self):
        p = sp.Parser()
        x = p.var_int("x")
        with pytest.raises((ValueError, sp.ParseError)):
            p.assert_(x)

    def test_double_declaration_keeps_original(self):
        # The parser is lenient about re-declaration: the second declaration
        # is ignored and the original sort is kept (no crash, no corruption).
        p = sp.Parser()
        p.parse_string("(declare-const d Int)")
        p.parse_string("(declare-const d Bool)")
        assert p.expr("d").sort.is_int


class TestStress:
    def test_wide_conjunction(self):
        p = sp.Parser()
        xs = [p.var_bool(f"b{i}") for i in range(500)]
        conj = p.and_(xs)
        assert conj.num_children == 500
        assert p.to_nnf(p.not_(conj)).is_or

    def test_deep_nesting_parse(self):
        depth = 200
        text = ("(declare-const n Int)(assert " + "(not " * depth +
                "(> n 0)" + ")" * depth + ")")
        p = sp.parse(text)
        assert len(p.assertions) == 1
        nnf = p.to_nnf(p.assertions[0])
        assert nnf.to_smt2() in ("(> n 0)", "(<= n 0)")

    def test_repeated_parse_reset_cycles(self):
        p = sp.Parser()
        for i in range(50):
            p.parse_string(f"(declare-const k{i} Int)(assert (> k{i} {i}))")
            assert len(p.assertions) == 1
            p.reset()

    def test_gc_stress_interleaved(self):
        kept = []
        for i in range(30):
            p = sp.Parser()
            v = p.var_int("m")
            e = p.mul(v, p.const_int(i))
            if i % 3 == 0:
                kept.append(e)
            del p
            if i % 10 == 0:
                gc.collect()
        gc.collect()
        for e in kept:
            assert e.kind in ("*", "CONST", "VAR") or e.is_const

    def test_shared_structure_large_dag(self):
        p = sp.Parser()
        e = p.var_int("x0")
        # 2^60-leaf tree collapses to 60 shared nodes in a DAG
        for _ in range(60):
            e = p.add(e, e)
        assert e.kind == "+"
        vs = p.collect_vars(e)
        assert len(vs) == 1
