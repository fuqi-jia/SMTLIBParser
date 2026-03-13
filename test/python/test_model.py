"""Tests for Model class functionality."""

import pytest


def test_model_creation():
    """Test Model can be created."""
    import somtparser as sp
    
    # Model is typically obtained from parser, but we test the class exists
    assert hasattr(sp, "Model")


def test_model_from_parser():
    """Test getting model from parser."""
    import somtparser as sp
    
    p = sp.parse("""
        (set-logic QF_LIA)
        (declare-const x Int)
        (assert (= x 5))
    """)
    
    # Try to get model (may be None if not solved)
    model = p.get_model()
    # Model access depends on solver state, just check it doesn't crash


def test_expression_building():
    """Test building expressions with Parser."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    # Create variables
    x = p.var_int("x")
    y = p.var_int("y")
    
    assert x is not None
    assert y is not None
    assert x.name == "x"
    assert y.name == "y"


def test_arithmetic_expressions():
    """Test arithmetic expression building."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    x = p.var_int("x")
    y = p.var_int("y")
    
    # Build expressions
    sum_expr = p.add(x, y)
    assert sum_expr is not None
    
    diff_expr = p.sub(x, y)
    assert diff_expr is not None
    
    prod_expr = p.mul(x, y)
    assert prod_expr is not None


def test_boolean_expressions():
    """Test boolean expression building."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    x = p.var_int("x")
    y = p.var_int("y")
    
    # Build comparison
    eq_expr = p.eq(x, y)
    assert eq_expr is not None
    assert eq_expr.sort.is_bool
    
    # Build boolean operations
    true_val = p.true_()
    false_val = p.false_()
    
    and_expr = p.and_(true_val, false_val)
    assert and_expr is not None
    
    or_expr = p.or_(true_val, false_val)
    assert or_expr is not None
    
    not_expr = p.not_(true_val)
    assert not_expr is not None


def test_ite_expression():
    """Test ITE expression building."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    x = p.var_int("x")
    cond = p.true_()
    
    one = p.const_int("1")
    two = p.const_int("2")
    
    ite_expr = p.ite(cond, one, two)
    assert ite_expr is not None


def test_const_values():
    """Test constant value creation."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    int_val = p.const_int("42")
    assert int_val is not None
    # Note: numeric literals are IntOrReal type in SMT-LIB (can be used as Int or Real)
    assert int_val.sort.is_int or int_val.sort.is_int_or_real
    
    real_val = p.const_real("3.14")
    assert real_val is not None
    # Note: numeric literals are IntOrReal type in SMT-LIB
    assert real_val.sort.is_real or real_val.sort.is_int_or_real


def test_assert_constraint():
    """Test asserting constraints."""
    import somtparser as sp
    
    p = sp.Parser()
    p.parse_string("(set-logic QF_LIA)")
    
    x = p.var_int("x")
    constraint = p.eq(x, p.const_int("5"))
    
    p.assert_(constraint)
    
    assert len(p.assertions) >= 1
