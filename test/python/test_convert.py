"""Formula conversion: NNF, CNF, DNF, Tseitin, Boolean abstraction maps."""

import pytest

import somtparser as sp


@pytest.fixture
def p():
    return sp.parse("(declare-const x Int)(declare-const y Int)")


def bool_eval(parser, formula, assignment):
    """Evaluate a formula under an integer assignment given as a dict."""
    m = sp.Model()
    for name, val in assignment.items():
        m.add(name, parser.const_int(val))
    return parser.evaluate(formula, m).value


class TestNNF:
    def test_not_pushed_to_atoms(self, p):
        f = p.expr("(not (and (> x 0) (< y 2)))")
        nnf = p.to_nnf(f)
        text = nnf.to_smt2()
        assert nnf.is_or
        assert "not" not in text  # negation absorbed into comparisons

    def test_nnf_double_negation(self, p):
        f = p.expr("(not (not (> x 0)))")
        assert p.to_nnf(f).to_smt2() == "(> x 0)"

    def test_nnf_implies(self, p):
        f = p.expr("(=> (> x 0) (> y 0))")
        nnf = p.to_nnf(f)
        assert "=>" not in nnf.to_smt2()

    def test_nnf_preserves_semantics(self, p):
        f = p.expr("(not (or (and (> x 0) (< y 2)) (= x y)))")
        nnf = p.to_nnf(f)
        for ax in (-1, 0, 1, 3):
            for ay in (-1, 1, 2, 3):
                env = {"x": ax, "y": ay}
                assert bool_eval(p, f, env) == bool_eval(p, nnf, env), env

    def test_nnf_list(self, p):
        f1, f2 = p.expr("(> x 0)"), p.expr("(not (< y 1))")
        combined = p.to_nnf([f1, f2])
        assert combined is not None


class TestCNF:
    def test_to_cnf_structure(self, p):
        f = p.expr("(or (and (> x 0) (< x 9)) (= x 100))")
        cnf = p.to_cnf(f)
        assert p.is_cnf(cnf)

    def test_cnf_abstraction_maps(self, p):
        f = p.expr("(or (and (> x 0) (< x 9)) (= x 100))")
        p.to_cnf(f)
        atoms = p.cnf_atoms()
        bvars = p.cnf_bool_vars()
        assert len(atoms) == 3
        assert len(bvars) == 3
        # bidirectional mapping is consistent
        for bv in bvars:
            atom = p.cnf_atom(bv)
            assert atom is not None
            assert p.cnf_bool_var(atom) == bv

    def test_cnf_atoms_stable_across_calls(self, p):
        f = p.expr("(or (and (> x 0) (< x 9)) (= x 100))")
        p.to_cnf(f)
        a1 = sorted(a.to_smt2() for a in p.cnf_atoms())
        a2 = sorted(a.to_smt2() for a in p.cnf_atoms())
        assert a1 == a2 and len(a1) == 3

    def test_cnf_of_list(self, p):
        cnf = p.to_cnf([p.expr("(> x 0)"), p.expr("(< y 5)")])
        assert p.is_cnf(cnf)

    def test_is_cnf_negative(self, p):
        f = p.expr("(or (and (> x 0) (< x 9)) (= x 100))")
        assert not p.is_cnf(f)


class TestCNFAssertedPosition:
    """Top-level (asserted) structure is encoded directly: conjunctions are
    split into independently asserted conjuncts, literals become unit clauses,
    and an asserted or/xor/implies is emitted as its clausal form. No Tseitin
    definition variable is introduced for the root connective."""

    def test_atomic_conjuncts_become_units(self, p):
        # (and (xor A B) C) plus D: only the xor needs encoding (2 clauses);
        # C and D are asserted directly as unit clauses.
        f1 = p.expr("(and (xor (>= x 1) (>= y 2)) (>= y -5))")
        f2 = p.expr("(= x y)")
        cnf = p.to_cnf([f1, f2])
        assert p.is_cnf(cnf)
        assert cnf.is_and
        clauses = cnf.children
        assert len(clauses) == 4
        units = [c for c in clauses if not c.is_or]
        binaries = [c for c in clauses if c.is_or]
        assert len(units) == 2 and len(binaries) == 2
        assert all(c.num_children == 2 for c in binaries)
        # exactly the 4 atom abstractions -- no extra definition variables
        assert len(p.cnf_bool_vars()) == 4

    def test_asserted_disjunction_is_single_clause(self, p):
        cnf = p.to_cnf(p.expr("(or (> x 0) (< y 2) (= x y))"))
        assert cnf.is_or
        assert cnf.num_children == 3
        assert len(p.cnf_bool_vars()) == 3

    def test_asserted_implies_is_single_clause(self, p):
        cnf = p.to_cnf(p.expr("(=> (> x 0) (< y 2))"))
        assert cnf.is_or
        assert cnf.num_children == 2

    def test_nested_structure_still_gets_definitions(self, p):
        # (or (and A B) C): the nested conjunction still needs a definition
        # variable, so the result is the top clause plus definition clauses.
        cnf = p.to_cnf(p.expr("(or (and (> x 0) (< x 9)) (= x 100))"))
        assert p.is_cnf(cnf)
        assert cnf.is_and
        assert cnf.num_children > 1

    def test_asserted_xor_semantics_over_booleans(self):
        # Over Boolean variables no abstraction happens, so the CNF can be
        # evaluated directly and compared with the original formula.
        import itertools

        q = sp.parse("(declare-const a Bool)(declare-const b Bool)(declare-const c Bool)")
        f = q.expr("(and (xor a b) c)")
        cnf = q.to_cnf(f)
        assert q.is_cnf(cnf)
        for va, vb, vc in itertools.product([True, False], repeat=3):
            m = sp.Model()
            m.add("a", q.true_() if va else q.false_())
            m.add("b", q.true_() if vb else q.false_())
            m.add("c", q.true_() if vc else q.false_())
            expected = (va != vb) and vc
            assert q.evaluate(f, m).value == expected, (va, vb, vc)
            assert q.evaluate(cnf, m).value == expected, (va, vb, vc)


class TestTseitin:
    def test_standalone_tseitin(self, p):
        f = p.expr("(or (and (> x 0) (< x 9)) (= x 100))")
        top, clauses = p.to_tseitin_cnf(f)
        assert top is not None
        assert len(clauses) >= 3
        # every clause is a disjunction of literals / abstraction vars
        for cl in clauses:
            assert cl.sort.is_bool

    def test_tseitin_on_negation(self, p):
        f = p.expr("(not (and (> x 0) (< y 2)))")
        top, clauses = p.to_tseitin_cnf(f)
        assert top is not None and len(clauses) > 0

    def test_tseitin_repeated_calls_no_crash(self, p):
        for _ in range(3):
            f = p.expr("(or (> x 0) (< y 2))")
            top, clauses = p.to_tseitin_cnf(f)
            assert top is not None


class TestDNF:
    def test_to_dnf(self, p):
        f = p.expr("(and (or (> x 0) (< x 9)) (= y 1))")
        dnf = p.to_dnf(f)
        assert dnf is not None
        # DNF root is an OR of AND-terms (or degenerates to a single term)
        assert dnf.is_or or dnf.is_and or dnf.is_atom

    def test_dnf_list(self, p):
        dnf = p.to_dnf([p.expr("(> x 0)"), p.expr("(or (< y 1) (> y 5))")])
        assert dnf is not None


class TestArithNormalize:
    def test_normalize(self, p):
        f = p.expr("(> (+ x 1) (+ y 2))")
        n = p.arith_normalize(f)
        assert n is not None
        assert n.sort.is_bool
