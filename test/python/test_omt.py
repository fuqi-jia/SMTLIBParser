"""OMT: objectives, soft assertions, optimization command parsing."""

import pytest

import somtparser as sp


class TestSingleObjectives:
    def test_minimize_maximize(self, sample_omt):
        p = sp.parse(sample_omt)
        objs = p.objectives
        assert len(objs) == 2
        mn, mx = objs
        assert mn.is_minimize and mn.kind == sp.OptKind.MINIMIZE
        assert mx.is_maximize and mx.kind == sp.OptKind.MAXIMIZE
        assert mn.is_single and not mn.is_multi
        assert mn.term is not None
        assert mn.term.name == "x"
        assert mx.term.name == "y"

    def test_minimize_expression_term(self):
        p = sp.parse("""
(declare-const a Int)(declare-const b Int)
(minimize (+ a (* 2 b)))
""")
        obj = p.objectives[0]
        assert obj.term.kind == "+"


class TestMultiObjectives:
    # Multi-objective syntax: name single objectives with define-objective,
    # then combine the names: (lex-optimize (obj1 obj2)). The named
    # objectives AND the composite all appear in parser.objectives.

    def test_lex_optimize(self):
        p = sp.parse("""
(declare-const a Int)(declare-const b Int)
(define-objective o1 (minimize a))
(define-objective o2 (maximize b))
(lex-optimize (o1 o2))
""")
        objs = p.objectives
        assert len(objs) == 3
        lex = objs[-1]
        assert lex.is_lex and lex.is_multi
        assert lex.num_subobjectives == 2
        subs = lex.subobjectives
        assert subs[0].is_minimize
        assert subs[1].is_maximize
        with pytest.raises(IndexError):
            lex.subobjective(5)

    def test_pareto_optimize(self):
        p = sp.parse("""
(declare-const a Int)(declare-const b Int)
(define-objective o1 (minimize a))
(define-objective o2 (minimize b))
(pareto-optimize (o1 o2))
""")
        assert p.objectives[-1].is_pareto

    def test_box_optimize(self):
        p = sp.parse("""
(declare-const a Int)(declare-const b Int)
(define-objective o1 (minimize a))
(define-objective o2 (maximize b))
(box-optimize (o1 o2))
""")
        assert p.objectives[-1].is_box


class TestSoftAssertions:
    def test_assert_soft(self):
        p = sp.parse("""
(declare-const a Bool)(declare-const b Bool)
(assert-soft a :weight 2)
(assert-soft b :weight 3)
""")
        assert len(p.soft_assertions) == 2
        weights = [w.value for w in p.soft_weights]
        assert weights == [2, 3]

    def test_maxsat_objective(self):
        p = sp.parse("""
(declare-const a Bool)
(assert-soft a :weight 1 :id goal)
(maxsat true :id goal)
""")
        objs = p.objectives
        assert len(objs) == 1
        assert objs[0].is_maxsat
        assert len(p.soft_assertions) == 1

    def test_grouped_soft_assertions(self):
        p = sp.parse("""
(declare-const a Bool)(declare-const b Bool)
(assert-soft a :weight 1 :id g1)
(assert-soft b :weight 1 :id g2)
""")
        groups = p.grouped_soft_assertions
        assert set(groups.keys()) >= {"g1", "g2"}


class TestOmtScript:
    def test_objective_commands_recorded(self, sample_omt):
        p = sp.Parser()
        p.set_command_logging(True)
        p.parse_string(sample_omt)
        types = [c.type for c in p.script]
        assert sp.CmdType.MINIMIZE in types
        assert sp.CmdType.MAXIMIZE in types

    def test_omt_dump_keeps_assertions(self, sample_omt):
        # dump_smt2() covers declarations + assertions; objectives are
        # exposed via parser.objectives (not serialized by the C++ dumper).
        p = sp.parse(sample_omt)
        text = p.dump_smt2()
        assert "(assert" in text and "declare-fun" in text
        assert len(p.objectives) == 2
