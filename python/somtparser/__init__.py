"""SOMTParser: A Python interface for SMT/OMT parsing.

This module provides Python bindings for the SOMTParser C++ library,
enabling parsing of SMT-LIB2 and OMT formats, expression manipulation,
model parsing, and formula evaluation.

Example:
    >>> import somtparser as sp
    >>> p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")
    >>> print(len(p.assertions))
    1
"""

from ._somtparser import (
    Parser,
    Node,
    Sort,
    Model,
    Objective,
    OptKind,
    parse,
    parse_file,
)

__version__ = "0.1.0"
__all__ = [
    "Parser",
    "Node",
    "Sort",
    "Model",
    "Objective",
    "OptKind",
    "parse",
    "parse_file",
]
