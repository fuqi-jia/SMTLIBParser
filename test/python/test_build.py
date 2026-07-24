"""Programmatic expression construction across all theories."""

import pytest

import somtparser as sp


class TestSorts:
    def test_basic_sorts(self):
        p = sp.Parser()
        assert p.int_sort().is_int
        assert p.real_sort().is_real
        assert p.bool_sort().is_bool
        assert p.string_sort().is_string
        assert p.regex_sort().is_regex
        assert p.rounding_mode_sort().is_rounding_mode

    def test_bv_fp_sorts(self):
        p = sp.Parser()
        bv = p.bv_sort(32)
        assert bv.is_bv and bv.bv_width == 32
        fp = p.fp_sort(11, 53)
        assert fp.is_fp
        assert fp.fp_exponent_width == 11
        assert fp.fp_significand_width == 53

    def test_array_sort(self):
        p = sp.Parser()
        arr = p.array_sort(p.int_sort(), p.real_sort())
        assert arr.is_array
        assert arr.index_sort.is_int
        assert arr.elem_sort.is_real

    def test_sort_equality(self):
        p = sp.Parser()
        assert p.int_sort() == p.int_sort()
        assert p.int_sort() != p.real_sort()
        assert p.bv_sort(8) == p.bv_sort(8)
        assert p.bv_sort(8) != p.bv_sort(16)

    def test_sort_str(self):
        p = sp.Parser()
        assert str(p.int_sort()) == "Int"
        assert "BitVec" in str(p.bv_sort(8))

    def test_declare_sort(self):
        p = sp.Parser()
        s = p.declare_sort("MySort", 0)
        assert s.name == "MySort"


class TestDeclareVar:
    def test_declare_var_with_string_sort(self):
        p = sp.Parser()
        v = p.declare_var("v", "Int")
        assert v.sort.is_int
        w = p.declare_var("w", "(_ BitVec 8)")
        assert w.sort.is_bv and w.sort.bv_width == 8

    def test_declare_var_with_sort_object(self):
        p = sp.Parser()
        v = p.declare_var("v", p.real_sort())
        assert v.sort.is_real

    def test_declare_var_bad_sort_raises(self):
        p = sp.Parser()
        with pytest.raises((ValueError, sp.ParseError)):
            p.declare_var("v", "NoSuchSort")

    def test_declared_var_visible_to_expr(self):
        p = sp.Parser()
        p.var_int("k")
        e = p.expr("(+ k 1)")
        assert e.kind == "+"


class TestBooleanConstruction:
    def test_nary_variants(self):
        p = sp.Parser()
        a, b, c = p.var_bool("a"), p.var_bool("b"), p.var_bool("c")
        assert p.and_([a, b, c]).num_children == 3
        assert p.or_([a, b, c]).num_children == 3
        e2 = p.and_(a, b)
        assert e2.is_and

    def test_eq_distinct(self):
        p = sp.Parser()
        x, y, z = p.var_int("x"), p.var_int("y"), p.var_int("z")
        assert p.eq(x, y).is_eq
        assert p.distinct(x, y).is_distinct
        assert p.eq([x, y, z]).is_eq
        assert p.distinct([x, y, z]).is_distinct

    def test_type_mismatch_raises(self):
        p = sp.Parser()
        x, y = p.var_int("x"), p.var_int("y")
        with pytest.raises((ValueError, sp.ParseError)):
            p.and_(x, y)
        with pytest.raises((ValueError, sp.ParseError)):
            p.not_(x)

    def test_constant_folding(self):
        p = sp.Parser()
        t, f = p.true_(), p.false_()
        assert p.and_(t, f).is_false
        assert p.or_(t, f).is_true
        assert p.not_(t).is_false


class TestArithConstruction:
    def test_int_ops_smt2(self):
        p = sp.Parser()
        x, y = p.var_int("x"), p.var_int("y")
        # commutative n-ary constructors may canonicalize operand order
        assert p.add(x, y).to_smt2() in ("(+ x y)", "(+ y x)")
        assert p.sub(x, y).to_smt2() == "(- x y)"
        assert p.mul(x, y).to_smt2() in ("(* x y)", "(* y x)")
        assert p.mod(x, y).to_smt2() == "(mod x y)"
        assert p.neg(x).kind == "-"

    def test_div_dispatch(self):
        p = sp.Parser()
        i, j = p.var_int("i"), p.var_int("j")
        r, s = p.var_real("r"), p.var_real("s")
        # div() is SMT-LIB "/" (real division); div_int() is SMT-LIB "div"
        assert p.div_int(i, j).is_div_int
        assert p.div(r, s).is_div_real
        assert p.mod(i, j).to_smt2() == "(mod i j)"

    def test_comparison_chain(self):
        p = sp.Parser()
        x, y, z = p.var_int("x"), p.var_int("y"), p.var_int("z")
        chain = p.lt([x, y, z])
        assert chain.sort.is_bool

    def test_transcendental(self):
        p = sp.Parser()
        r = p.var_real("r")
        for f in (p.exp, p.ln, p.sin, p.cos, p.tan, p.sqrt, p.atan,
                  p.sinh, p.cosh, p.tanh):
            node = f(r)
            assert node.sort is not None
        assert p.atan2(r, r) is not None
        assert p.pow(r, p.const_real(2.0)) is not None

    def test_constants(self):
        p = sp.Parser()
        assert p.pi().is_pi
        assert p.e().is_e
        assert p.const_real(0.5).is_const_real

    def test_arith_constant_folding(self):
        p = sp.Parser()
        e = p.add(p.const_int(2), p.const_int(3))
        assert e.value == 5

    def test_gcd_lcm_factorial(self):
        p = sp.Parser()
        a, b = p.var_int("a"), p.var_int("b")
        # symbolic operands stay unfolded
        assert p.gcd(a, b).to_smt2() == "(gcd a b)"
        assert p.lcm(a, b).to_smt2() == "(lcm a b)"
        assert p.factorial(a) is not None
        # constant operands fold
        assert p.gcd(p.const_int(12), p.const_int(18)).value == 6
        assert p.lcm(p.const_int(4), p.const_int(6)).value == 12
        assert p.factorial(p.const_int(5)).value == 120

    def test_max_min(self):
        p = sp.Parser()
        a, b = p.var_int("a"), p.var_int("b")
        assert p.max_([a, b]) is not None
        assert p.min_([a, b]) is not None

    def test_conversions(self):
        p = sp.Parser()
        i, r = p.var_int("i"), p.var_real("r")
        assert p.to_real(i).sort.is_real
        assert p.to_int(r).sort.is_int
        assert p.is_int_pred(r).sort.is_bool


class TestBvConstruction:
    def test_bv_ops_roundtrip(self):
        p = sp.Parser()
        a, b = p.var_bv("a", 8), p.var_bv("b", 8)
        cases = {
            p.bv_not(a): "(bvnot a)",
            p.bv_and(a, b): "(bvand a b)",
            p.bv_or(a, b): "(bvor a b)",
            p.bv_xor(a, b): "(bvxor a b)",
            p.bv_add(a, b): "(bvadd a b)",
            p.bv_sub(a, b): "(bvsub a b)",
            p.bv_mul(a, b): "(bvmul a b)",
            p.bv_udiv(a, b): "(bvudiv a b)",
            p.bv_urem(a, b): "(bvurem a b)",
            p.bv_sdiv(a, b): "(bvsdiv a b)",
            p.bv_shl(a, b): "(bvshl a b)",
            p.bv_lshr(a, b): "(bvlshr a b)",
            p.bv_ashr(a, b): "(bvashr a b)",
            p.bv_neg(a): "(bvneg a)",
        }
        for node, expected in cases.items():
            assert node.to_smt2() == expected

    def test_bv_comparisons(self):
        p = sp.Parser()
        a, b = p.var_bv("a", 8), p.var_bv("b", 8)
        for f in (p.bv_ult, p.bv_ule, p.bv_ugt, p.bv_uge,
                  p.bv_slt, p.bv_sle, p.bv_sgt, p.bv_sge):
            assert f(a, b).sort.is_bool

    def test_bv_extract_concat(self):
        p = sp.Parser()
        a, b = p.var_bv("a", 8), p.var_bv("b", 8)
        ext = p.bv_extract(a, 7, 4)
        assert ext.sort.is_bv and ext.sort.bv_width == 4
        cat = p.bv_concat([a, b])
        assert cat.sort.bv_width == 16

    def test_bv_extensions(self):
        p = sp.Parser()
        a = p.var_bv("a", 8)
        assert p.bv_zero_extend(a, 8).sort.bv_width == 16
        assert p.bv_sign_extend(a, 8).sort.bv_width == 16
        assert p.bv_rotate_left(a, 3).sort.bv_width == 8
        assert p.bv_repeat(a, 2).sort.bv_width == 16

    def test_bv_width_mismatch_raises(self):
        p = sp.Parser()
        a, c = p.var_bv("a", 8), p.var_bv("c", 16)
        with pytest.raises((ValueError, sp.ParseError)):
            p.bv_add(a, c)

    def test_bv_const(self):
        p = sp.Parser()
        n = p.const_bv(10, 8)
        assert n.sort.is_bv and n.sort.bv_width == 8

    def test_bv_int_conversion(self):
        p = sp.Parser()
        a, i = p.var_bv("a", 8), p.var_int("i")
        assert p.bv_to_nat(a).sort.is_int or p.bv_to_nat(a).sort is not None
        assert p.int_to_bv(i, 8).sort.is_bv


class TestFpConstruction:
    def test_fp_arith(self):
        p = sp.Parser()
        f, g = p.var_fp("f", 8, 24), p.var_fp("g", 8, 24)
        rm = p.rounding_mode("RNE")
        assert p.fp_add(rm, f, g).to_smt2() == "(fp.add RNE f g)"
        assert p.fp_sub(rm, f, g).to_smt2() == "(fp.sub RNE f g)"
        assert p.fp_mul(rm, f, g).to_smt2() == "(fp.mul RNE f g)"
        assert p.fp_div(rm, f, g).to_smt2() == "(fp.div RNE f g)"
        assert p.fp_sqrt(rm, f).kind == "fp.sqrt"
        assert p.fp_fma(rm, f, g, f) is not None

    def test_fp_comparisons_and_preds(self):
        p = sp.Parser()
        f, g = p.var_fp("f", 8, 24), p.var_fp("g", 8, 24)
        for fn in (p.fp_le, p.fp_lt, p.fp_ge, p.fp_gt, p.fp_eq):
            assert fn(f, g).sort.is_bool
        for fn in (p.fp_is_normal, p.fp_is_subnormal, p.fp_is_zero,
                   p.fp_is_inf, p.fp_is_nan, p.fp_is_neg, p.fp_is_pos):
            assert fn(f).sort.is_bool

    def test_rounding_modes(self):
        p = sp.Parser()
        for mode in ("RNE", "RNA", "RTP", "RTN", "RTZ"):
            assert p.rounding_mode(mode) is not None

    def test_fp_conversions(self):
        p = sp.Parser()
        f = p.var_fp("f", 8, 24)
        rm = p.rounding_mode("RTZ")
        assert p.fp_to_real(f).sort.is_real
        assert p.fp_to_ubv(rm, f, 32).sort.is_bv
        assert p.fp_to_sbv(rm, f, 32).sort.is_bv
        r = p.var_real("r")
        assert p.to_fp(8, 24, rm, r).sort.is_fp


class TestStringConstruction:
    def test_str_basics(self):
        p = sp.Parser()
        s, t = p.var_str("s"), p.var_str("t")
        assert p.str_len(s).sort.is_int
        assert p.str_concat([s, t]).sort.is_string
        assert p.str_contains(s, t).sort.is_bool
        assert p.str_prefixof(s, t).sort.is_bool
        assert p.str_suffixof(s, t).sort.is_bool

    def test_str_const_literal(self):
        p = sp.Parser()
        lit = p.const_str('"hello"')
        assert lit.is_const_str
        assert lit.value == "hello"

    def test_str_manipulation(self):
        p = sp.Parser()
        s = p.var_str("s")
        i = p.var_int("i")
        assert p.str_at(s, i).sort.is_string
        assert p.str_substr(s, i, i).sort.is_string
        assert p.str_to_lower(s).sort.is_string
        assert p.str_to_upper(s).sort.is_string
        assert p.str_rev(s).sort.is_string
        assert p.str_to_int(s).sort.is_int
        assert p.str_from_int(i).sort.is_string

    def test_regex(self):
        p = sp.Parser()
        s = p.var_str("s")
        ab = p.str_to_re(p.const_str('"ab"'))
        assert p.str_in_re(s, p.re_star(ab)).sort.is_bool
        assert p.re_union([ab, p.re_allchar()]) is not None
        assert p.re_concat([ab, ab]) is not None
        assert p.re_plus(ab) is not None
        assert p.re_opt(ab) is not None
        assert p.re_complement(ab) is not None
        lo, hi = p.const_str('"a"'), p.const_str('"z"')
        assert p.re_range(lo, hi) is not None


class TestArrayConstruction:
    def test_select_store(self):
        p = sp.Parser()
        arr = p.var_array("arr", p.int_sort(), p.int_sort())
        i, v = p.const_int(1), p.const_int(99)
        st = p.store(arr, i, v)
        assert st.is_store
        sel = p.select(arr, i)
        assert sel.sort.is_int
        # read-over-write is simplified to the stored value
        assert p.select(st, i).value == 99

    def test_const_array(self):
        p = sp.Parser()
        asort = p.array_sort(p.int_sort(), p.int_sort())
        ca = p.const_array(asort, p.const_int(0))
        assert ca.is_const_array


class TestQuantifiers:
    def test_forall_exists_shape(self):
        p = sp.Parser()
        qx = p.quant_var("qx", p.int_sort())
        body = p.gt(qx, p.const_int(0))
        f = p.forall([qx], body)
        assert f.to_smt2() == "(forall ((qx Int)) (> qx 0))"
        e = p.exists([qx], body)
        assert e.to_smt2() == "(exists ((qx Int)) (> qx 0))"

    def test_multi_var_quantifier(self):
        p = sp.Parser()
        qx = p.quant_var("qx", p.int_sort())
        qy = p.quant_var("qy", p.int_sort())
        f = p.forall([qx, qy], p.ge(qx, qy))
        assert "qx" in f.to_smt2() and "qy" in f.to_smt2()

    def test_matches_parsed_form(self):
        p = sp.Parser()
        qx = p.quant_var("w", p.int_sort())
        built = p.forall([qx], p.gt(qx, p.const_int(0)))
        parsed = sp.parse("(assert (forall ((w Int)) (> w 0)))").assertions[0]
        assert built.to_smt2() == parsed.to_smt2()


class TestFunctions:
    def test_declare_and_apply_uf(self):
        p = sp.Parser()
        f = p.declare_fun("f", [p.int_sort(), p.int_sort()], p.int_sort())
        x = p.var_int("x")
        app = p.apply_uf(f, [x, p.const_int(1)])
        assert app.sort.is_int

    def test_define_and_expand(self):
        p = sp.Parser()
        a = p.declare_var("a", "Int")
        # g(u) = u + 1, defined via parsed text for simplicity
        p.parse_string("(define-fun g ((u Int)) Int (+ u 1))")
        e = p.expr("(g a)")
        assert e.sort.is_int

    def test_apply_fun_builds_application(self):
        p = sp.Parser()
        u = p.declare_var("u0", "Int")  # helper var for the body
        fn = p.define_fun("h", [u], p.int_sort(), p.add(u, p.const_int(1)))
        r = p.apply_fun(fn, [p.const_int(41)])
        # the application node stays symbolic (bodies are not inlined eagerly)
        assert r.to_smt2() == "(h 41)"
        assert r.sort.is_int


class TestTransformations:
    def test_substitute(self):
        p = sp.Parser()
        v = p.var_int("v")
        g = p.gt(v, p.const_int(0))
        r = p.substitute(g, {"v": p.const_int(5)})
        assert r.is_true  # (> 5 0) folds to true

    def test_replace_nodes(self):
        p = sp.Parser()
        v, w = p.var_int("v"), p.var_int("w")
        g = p.gt(v, p.const_int(0))
        r = p.replace_nodes(g, {v: w})
        assert r.to_smt2() == "(> w 0)"

    def test_negate_converse_mirror_comp(self):
        p = sp.Parser()
        v = p.var_int("v")
        lt = p.lt(v, p.const_int(3))
        # negate: logical negation, operands unchanged: not(v < 3) == (v >= 3)
        neg = p.negate_comp(lt)
        assert neg.is_ge
        assert neg[1].value == 3
        # converse: reversed relation, operator unchanged: (v < 3) -> (3 < v)
        flipped = p.converse_comp(lt)
        assert flipped.is_lt
        assert flipped[0].value == 3
        # mirror: equivalent rewrite: (v < 3) -> (3 > v)
        mirrored = p.mirror_comp(lt)
        assert mirrored.is_gt
        assert mirrored[0].value == 3
        # eq/distinct are symmetric: both leave them unchanged
        eq = p.eq(v, p.const_int(3))
        assert p.converse_comp(eq) == eq
        assert p.mirror_comp(eq) == eq

    def test_collect_vars(self):
        p = sp.parse("(declare-const x Int)(declare-const y Int)(assert (> (+ x y) 0))")
        vs = p.collect_vars(p.assertions[0])
        assert {v.name for v in vs} == {"x", "y"}
        vs2 = p.collect_vars(p.assertions)
        assert {v.name for v in vs2} == {"x", "y"}

    def test_collect_atoms(self):
        p = sp.parse(
            "(declare-const x Int)(assert (or (> x 0) (and (< x 9) (= x 4))))")
        atoms = p.collect_atoms(p.assertions[0])
        assert {a.to_smt2() for a in atoms} == {"(> x 0)", "(< x 9)", "(= x 4)"}

    def test_expand_let(self):
        p = sp.parse("(declare-const x Int)(assert (let ((t (+ x 1))) (> t 0)))")
        expanded = p.expand_let(p.assertions[0])
        assert "let" not in expanded.to_smt2()

    def test_binarize(self):
        p = sp.parse("(declare-const x Int)(assert (= (+ x x x x) 4))")
        b = p.binarize_op(p.assertions[0])
        assert b is not None
