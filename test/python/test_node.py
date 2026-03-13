"""Tests for Node class functionality."""

import pytest


def test_node_kind():
    """Test Node.kind property."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (and (> x 0) (< x 10)))")
    node = p.assertions[0]
    
    # The top-level assertion should be an AND
    assert node.kind == "and" or node.is_and


def test_node_sort():
    """Test Node.sort property."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    node = p.assertions[0]
    
    # The assertion (> x 0) should have Bool sort
    sort = node.sort
    assert sort.is_bool


def test_node_children():
    """Test Node children access."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (and (> x 0) (< x 10)))")
    node = p.assertions[0]
    
    # AND node should have 2 children
    assert node.num_children == 2 or len(node) == 2


def test_node_getitem():
    """Test Node.__getitem__ (indexing)."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (and (> x 0) (< x 10)))")
    node = p.assertions[0]
    
    if len(node) >= 2:
        child0 = node[0]
        child1 = node[1]
        assert child0 is not None
        assert child1 is not None
        
        # Test negative indexing
        assert node[-1] is not None


def test_node_getitem_out_of_range():
    """Test Node.__getitem__ raises IndexError for invalid index."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    node = p.assertions[0]
    
    with pytest.raises(IndexError):
        _ = node[100]


def test_node_iteration():
    """Test Node.__iter__ (iteration over children)."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (and (> x 0) (< x 10)))")
    node = p.assertions[0]
    
    children = list(node)
    assert len(children) == node.num_children


def test_node_len():
    """Test Node.__len__."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (and (> x 0) (< x 10)))")
    node = p.assertions[0]
    
    assert len(node) == node.num_children


def test_node_repr():
    """Test Node.__repr__."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    node = p.assertions[0]
    
    repr_str = repr(node)
    assert isinstance(repr_str, str)
    assert len(repr_str) > 0


def test_node_to_smt2():
    """Test Node.to_smt2 method."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    node = p.assertions[0]
    
    smt2_str = node.to_smt2()
    assert isinstance(smt2_str, str)
    # Should contain the comparison operator
    assert ">" in smt2_str or "x" in smt2_str


def test_node_type_checks():
    """Test Node type check properties."""
    import somtparser as sp
    
    p = sp.parse("""
        (set-logic QF_LIA)
        (declare-const x Int)
        (assert (and (> x 0) (not (= x 5))))
    """)
    
    node = p.assertions[0]
    
    # Top level should be AND
    if node.is_and:
        assert node.kind == "and"
        
    # Check that type checks are booleans
    assert isinstance(node.is_const, bool)
    assert isinstance(node.is_var, bool)
    assert isinstance(node.is_leaf, bool)


def test_node_variable():
    """Test that variables are correctly identified."""
    import somtparser as sp
    
    p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    
    variables = p.variables
    assert len(variables) >= 1
    
    x_var = variables[0]
    assert x_var.is_var or x_var.is_const
    assert x_var.name == "x"
