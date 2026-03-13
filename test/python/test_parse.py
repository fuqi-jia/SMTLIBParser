"""Tests for parsing functionality."""

import pytest


def test_import():
    """Test that the module can be imported."""
    import somtparser as sp
    assert hasattr(sp, "Parser")
    assert hasattr(sp, "parse")
    assert hasattr(sp, "parse_file")
    assert hasattr(sp, "Node")
    assert hasattr(sp, "Sort")
    assert hasattr(sp, "Model")


def test_parse_simple():
    """Test parsing a simple SMT-LIB2 string."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    assert p is not None
    assert len(p.assertions) == 1


def test_parse_string_method(sample_qf_lia):
    """Test Parser.parse_string method."""
    import somtparser as sp
    
    p = sp.Parser()
    result = p.parse_string(sample_qf_lia)
    # Should return self for chaining
    assert result is p
    assert len(p.assertions) == 3


def test_parse_file(instances_dir):
    """Test parsing a file."""
    import somtparser as sp
    
    # Find a simple test file
    test_file = instances_dir / "symbols.smt2"
    if not test_file.exists():
        # Try another file
        files = list(instances_dir.glob("*.smt2"))
        if files:
            test_file = files[0]
        else:
            pytest.skip("No test files available")
    
    p = sp.parse_file(str(test_file))
    assert p is not None


def test_parse_all_instances(smt2_files):
    """Regression test: parse all existing test instances without error."""
    import somtparser as sp
    
    errors = []
    for f in smt2_files:
        try:
            # Skip known error test files
            if "error" in f.name.lower():
                continue
            p = sp.parse_file(str(f))
            # Access assertions to verify parsing worked
            _ = p.assertions
        except Exception as e:
            errors.append(f"{f.name}: {e}")
    
    if errors:
        pytest.fail(f"Failed to parse {len(errors)} files:\n" + "\n".join(errors[:5]))


def test_parser_chaining():
    """Test method chaining."""
    import somtparser as sp
    
    p = (sp.Parser()
         .parse_string("(set-logic QF_LIA)")
         .parse_string("(declare-const x Int)")
         .parse_string("(assert (> x 0))"))
    
    assert len(p.assertions) == 1


def test_assertions_property(sample_qf_lia):
    """Test assertions property."""
    import somtparser as sp
    
    p = sp.parse(sample_qf_lia)
    assertions = p.assertions
    assert isinstance(assertions, list)
    assert len(assertions) == 3
    for a in assertions:
        assert isinstance(a, sp.Node)


def test_variables_property(sample_qf_lia):
    """Test variables property."""
    import somtparser as sp
    
    p = sp.parse(sample_qf_lia)
    variables = p.variables
    assert isinstance(variables, list)
    # Should have x and y
    assert len(variables) >= 2
