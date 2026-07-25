"""Parsing: strings, files, errors, incremental interface, script logging."""

import pytest

import somtparser as sp


class TestBasicParsing:
    def test_module_surface(self):
        for name in ("Parser", "Node", "Sort", "Model", "Script", "Command",
                     "Objective", "MetaObjective", "OptKind", "CmdType",
                     "ResultType", "ParseError", "parse", "parse_file"):
            assert hasattr(sp, name), name

    def test_parse_simple(self):
        p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
        assert len(p.assertions) == 1
        assert p.assertions[0].kind == ">"
        assert p.logic == "QF_LIA"

    def test_parse_string_chaining(self, sample_qf_lia):
        p = sp.Parser()
        result = p.parse_string(sample_qf_lia)
        assert result is p
        assert len(p.assertions) == 3

    def test_parse_file(self, instances_dir, tmp_path):
        f = tmp_path / "t.smt2"
        f.write_text("(set-logic QF_LIA)(declare-const a Int)(assert (= a 1))")
        p = sp.parse_file(str(f))
        assert len(p.assertions) == 1

    def test_parse_file_missing_raises(self):
        with pytest.raises(sp.ParseError):
            sp.parse_file("/nonexistent/path/file.smt2")

    def test_parse_error_raises(self):
        with pytest.raises(sp.ParseError):
            sp.parse("(assert (> undeclared_var 0))")

    def test_parse_error_is_exception_not_crash(self):
        for bad in ["(assert", "(declare-const x)", "(assert (+ 1 2))",
                    "(pop 1)", "(assert (and 1 2))"]:
            p = sp.Parser()
            try:
                p.parse_string(bad)
            except (sp.ParseError, ValueError):
                pass  # any Python exception is acceptable; crashing is not

    def test_parse_all_instances(self, smt2_files):
        errors = []
        for f in smt2_files:
            if "error" in f.name.lower():
                continue
            try:
                p = sp.parse_file(str(f))
                _ = p.assertions
            except Exception as e:  # noqa: BLE001
                errors.append(f"{f.name}: {e}")
        assert not errors, "Failed to parse:\n" + "\n".join(errors)

    def test_error_instances_are_fixed_regressions(self, instances_dir):
        # The error_*.smt2 files are regression fixtures for bugs that have
        # been FIXED: they must parse cleanly now.
        error_files = sorted(instances_dir.glob("error_*.smt2"))
        assert error_files, "no error_*.smt2 fixtures found"
        for f in error_files:
            p = sp.parse_file(str(f))
            assert len(p.assertions) > 0

    def test_assert_string(self):
        p = sp.Parser()
        p.parse_string("(declare-const x Int)")
        p.assert_("(> x 1)")
        assert len(p.assertions) == 1

    def test_assert_node(self):
        p = sp.Parser()
        x = p.var_int("x")
        p.assert_(p.gt(x, p.const_int(0)))
        assert len(p.assertions) == 1

    def test_expr(self):
        p = sp.Parser()
        p.parse_string("(declare-const x Int)")
        e = p.expr("(+ x 1)")
        assert e.kind == "+"
        assert e.num_children == 2

    def test_expr_unknown_symbol_raises(self):
        p = sp.Parser()
        with pytest.raises((sp.ParseError, ValueError)):
            p.expr("(+ nosuchvar 1)")


class TestContents:
    def test_variables(self, sample_qf_lia):
        p = sp.parse(sample_qf_lia)
        names = {v.name for v in p.variables}
        assert names == {"x", "y"}
        assert all(v.is_var for v in p.variables)

    def test_declared_variables(self, sample_qf_lia):
        p = sp.parse(sample_qf_lia)
        assert {v.name for v in p.declared_variables} == {"x", "y"}

    def test_get_variable(self, sample_qf_lia):
        p = sp.parse(sample_qf_lia)
        x = p.get_variable("x")
        assert x.name == "x"
        assert x.sort.is_int
        assert p.is_declared_variable("x")
        assert not p.is_declared_variable("zzz")
        with pytest.raises(KeyError):
            p.get_variable("zzz")

    def test_functions(self):
        p = sp.parse("""
            (declare-fun f (Int Int) Int)
            (define-fun g ((a Int)) Int (+ a 1))
        """)
        assert p.is_declared_function("f")
        assert p.is_declared_function("g")

    def test_result_type_check_sat(self):
        # check_sat() folds trivially-true/false assertion sets; anything
        # symbolic stays UNKNOWN. (Responses like a bare "sat" line are not
        # part of the SMT-LIB input language and are rejected by the parser.)
        p = sp.parse("(set-logic QF_LIA)(assert true)(check-sat)")
        assert p.check_sat() == sp.ResultType.SAT
        q = sp.parse("(declare-const x Int)(assert (> x 0))")
        assert q.check_sat() == sp.ResultType.UNKNOWN
        with pytest.raises(sp.ParseError):
            sp.parse("(set-logic QF_LIA)(check-sat)\nsat")

    def test_node_count_grows(self):
        p = sp.Parser()
        before = p.node_count
        p.parse_string("(declare-const x Int)(assert (> (+ x 1) 0))")
        assert p.node_count > before


class TestIncremental:
    def test_push_pop(self):
        p = sp.Parser()
        p.parse_string("(declare-const b Int)(assert (> b 0))")
        assert len(p.assertions) == 1
        p.push()
        p.parse_string("(assert (< b 5))")
        assert len(p.assertions) == 2
        p.pop()
        assert len(p.assertions) == 1

    def test_push_pop_n(self):
        p = sp.Parser()
        p.parse_string("(declare-const b Int)")
        p.push(2)
        p.parse_string("(assert (> b 0))")
        p.pop(2)
        assert len(p.assertions) == 0

    def test_pop_without_push_raises(self):
        p = sp.Parser()
        with pytest.raises(ValueError):
            p.pop()

    def test_reset_assertions(self):
        p = sp.Parser()
        p.parse_string("(declare-const c Int)(assert (> c 0))")
        p.reset_assertions()
        assert len(p.assertions) == 0

    def test_reset(self):
        p = sp.Parser()
        p.parse_string("(declare-const c Int)(assert (> c 0))")
        p.reset()
        assert len(p.assertions) == 0
        assert not p.is_declared_variable("c")

    def test_parsed_push_pop_commands(self):
        p = sp.parse("""
            (declare-const v Int)
            (assert (> v 0))
            (push 1)
            (assert (< v -5))
            (pop 1)
        """)
        assert len(p.assertions) == 1


class TestScriptLogging:
    def test_script_records_commands(self):
        p = sp.Parser()
        p.set_command_logging(True)
        p.parse_string(
            "(set-logic QF_LIA)(declare-const a Int)(assert (> a 1))"
            "(push 1)(check-sat)(get-model)(pop 1)(echo \"hi\")")
        types = [c.type for c in p.script]
        assert sp.CmdType.SET_LOGIC in types
        assert sp.CmdType.DECLARE_CONST in types
        assert sp.CmdType.ASSERT in types
        assert sp.CmdType.PUSH in types
        assert sp.CmdType.CHECK_SAT in types
        assert sp.CmdType.GET_MODEL in types
        assert sp.CmdType.POP in types
        assert sp.CmdType.ECHO in types

    def test_script_sequence_protocol(self):
        p = sp.Parser()
        p.set_command_logging(True)
        p.parse_string("(set-logic QF_LIA)(declare-const a Int)")
        assert len(p.script) == 2
        assert p.script[0].type == sp.CmdType.SET_LOGIC
        assert p.script[-1].type == sp.CmdType.DECLARE_CONST
        with pytest.raises(IndexError):
            _ = p.script[99]

    def test_command_payloads(self):
        p = sp.Parser()
        p.set_command_logging(True)
        p.parse_string(
            "(set-logic QF_LIA)(declare-const a Int)(assert (> a 1))"
            "(get-value (a (+ a 1)))(echo \"msg\")")
        cmds = list(p.script)
        assert_cmd = [c for c in cmds if c.type == sp.CmdType.ASSERT][0]
        assert assert_cmd.is_assert
        assert assert_cmd.expr is not None and assert_cmd.expr.kind == ">"
        decl = [c for c in cmds if c.type == sp.CmdType.DECLARE_CONST][0]
        assert decl.name == "a"
        assert decl.sort is not None and decl.sort.is_int
        getv = [c for c in cmds if c.type == sp.CmdType.GET_VALUE][0]
        assert len(getv.value_terms) == 2
        echo = [c for c in cmds if c.type == sp.CmdType.ECHO][0]
        assert "msg" in echo.keyword

    def test_script_disabled_by_default(self):
        p = sp.Parser()
        p.parse_string("(set-logic QF_LIA)")
        assert len(p.script) == 0


class TestDump:
    def test_dump_smt2_roundtrip(self, sample_qf_lia):
        p = sp.parse(sample_qf_lia)
        text = p.dump_smt2()
        assert "declare-" in text and "assert" in text
        p2 = sp.parse(text)
        assert len(p2.assertions) == len(p.assertions)

    def test_dump_smt2_to_file(self, tmp_path, sample_qf_lia):
        p = sp.parse(sample_qf_lia)
        out = tmp_path / "dump.smt2"
        p.dump_smt2(str(out))
        assert out.exists()
        p2 = sp.parse_file(str(out))
        assert len(p2.assertions) == len(p.assertions)

    def test_to_string(self):
        p = sp.Parser()
        x = p.var_int("x")
        s = p.to_string(p.gt(x, p.const_int(0)))
        assert "x" in s

    def test_options_smt2(self):
        p = sp.Parser()
        p.set_option("produce-models", True)
        assert isinstance(p.options_smt2(), str)


class TestOptions:
    def test_strict_smtlib(self):
        p = sp.Parser()
        assert p.get_strict_smtlib() is False
        p.set_strict_smtlib(True)
        assert p.get_strict_smtlib() is True

    def test_evaluate_use_floating(self):
        p = sp.Parser()
        p.set_evaluate_use_floating(False)
        assert p.get_evaluate_use_floating() is False
        p.set_evaluate_use_floating(True)
        assert p.get_evaluate_use_floating() is True

    def test_set_option_types(self):
        p = sp.Parser()
        p.set_option("produce-models", True)
        p.set_option("random-seed", 42)
        p.set_option("some-name", "value")
