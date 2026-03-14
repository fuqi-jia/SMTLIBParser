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

import os
import sys

if sys.platform == "win32":
    _package_dir = os.path.dirname(os.path.abspath(__file__))
    _bin_dir = os.path.join(_package_dir, "bin")
    if os.path.isdir(_bin_dir):
        os.add_dll_directory(_bin_dir)
        os.environ["PATH"] = _bin_dir + os.pathsep + os.environ.get("PATH", "")

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
