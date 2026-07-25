"""pytest configuration and fixtures for SOMTParser Python binding tests."""

from pathlib import Path

import pytest


@pytest.fixture
def instances_dir():
    """Path to the shared C++ test instances directory."""
    return Path(__file__).parent.parent / "instances"


@pytest.fixture
def smt2_files(instances_dir):
    """All .smt2 test files."""
    if not instances_dir.exists():
        pytest.skip(f"Test instances directory not found: {instances_dir}")
    files = sorted(instances_dir.glob("*.smt2"))
    if not files:
        pytest.skip("No .smt2 files found in test instances")
    return files


@pytest.fixture
def sample_qf_lia():
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
    return """
(set-logic QF_BV)
(declare-const a (_ BitVec 32))
(declare-const b (_ BitVec 32))
(assert (= (bvadd a b) #x00000010))
(check-sat)
"""


@pytest.fixture
def sample_omt():
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
