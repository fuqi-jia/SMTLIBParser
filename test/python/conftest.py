"""pytest configuration and fixtures for SOMTParser tests."""

import pytest
from pathlib import Path


@pytest.fixture
def instances_dir():
    """Return path to test instances directory."""
    # Navigate from test/python/ to test/instances/
    return Path(__file__).parent.parent / "instances"


@pytest.fixture
def smt2_files(instances_dir):
    """Return all .smt2 test files."""
    if not instances_dir.exists():
        pytest.skip(f"Test instances directory not found: {instances_dir}")
    files = list(instances_dir.glob("*.smt2"))
    if not files:
        pytest.skip("No .smt2 files found in test instances")
    return files


@pytest.fixture
def sample_qf_lia():
    """Return a simple QF_LIA SMT-LIB2 string."""
    return """
(set-logic QF_LIA)
(declare-const x Int)
(declare-const y Int)
(assert (> x 0))
(assert (< y 10))
(assert (= (+ x y) 5))
(check-sat)
"""


@pytest.fixture
def sample_qf_bv():
    """Return a simple QF_BV SMT-LIB2 string."""
    return """
(set-logic QF_BV)
(declare-const a (_ BitVec 32))
(declare-const b (_ BitVec 32))
(assert (= (bvadd a b) #x00000010))
(check-sat)
"""


@pytest.fixture
def sample_omt():
    """Return a simple OMT string with objectives."""
    return """
(set-logic QF_LIA)
(declare-const x Int)
(declare-const y Int)
(assert (>= x 0))
(assert (>= y 0))
(assert (<= (+ x y) 100))
(minimize x)
(maximize y)
(check-sat)
"""
