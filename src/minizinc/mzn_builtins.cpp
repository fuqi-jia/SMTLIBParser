/* -*- C++ -*-
 *
 * MiniZinc Frontend — Built-in Registry Implementation
 */

#include "somtparser/minizinc/mzn_builtins.h"
#include <algorithm>

namespace SOMTParser::MiniZinc {

// ── Singleton accessor ───────────────────────────────────────────
MznBuiltins& MznBuiltins::get() {
    static MznBuiltins instance;
    return instance;
}

// ── Constructor ──────────────────────────────────────────────────
MznBuiltins::MznBuiltins() {
    initOperators();
    initFunctions();
    initGlobals();
    initAnnotations();
}

// ── Operator initialization ──────────────────────────────────────
void MznBuiltins::initOperators() {
    // Boolean infix (precedence matches mzn_parser.cpp)
    operators["/\\"]  = {OpInfo::Arity::INFIX, 50, false, BinaryOp::Op::AND,  UnaryOp::Op::NOT};
    operators["\\/"]  = {OpInfo::Arity::INFIX, 30, false, BinaryOp::Op::OR,   UnaryOp::Op::NOT};
    operators["->"]   = {OpInfo::Arity::INFIX, 20, true,  BinaryOp::Op::IMPLIES, UnaryOp::Op::NOT};
    operators["<-"]   = {OpInfo::Arity::INFIX, 20, false, BinaryOp::Op::IMPLIED_BY, UnaryOp::Op::NOT};
    operators["<->"]  = {OpInfo::Arity::INFIX, 10, false, BinaryOp::Op::IFF,  UnaryOp::Op::NOT};
    operators["xor"]  = {OpInfo::Arity::INFIX, 40, false, BinaryOp::Op::XOR,  UnaryOp::Op::NOT};

    // Comparison
    operators["="]    = {OpInfo::Arity::INFIX, 60, false, BinaryOp::Op::EQ,   UnaryOp::Op::NOT};
    operators["!="]   = {OpInfo::Arity::INFIX, 60, false, BinaryOp::Op::NEQ,  UnaryOp::Op::NOT};
    operators["<"]    = {OpInfo::Arity::INFIX, 70, false, BinaryOp::Op::LT,   UnaryOp::Op::NOT};
    operators["<="]   = {OpInfo::Arity::INFIX, 70, false, BinaryOp::Op::LE,   UnaryOp::Op::NOT};
    operators[">"]    = {OpInfo::Arity::INFIX, 70, false, BinaryOp::Op::GT,   UnaryOp::Op::NOT};
    operators[">="]   = {OpInfo::Arity::INFIX, 70, false, BinaryOp::Op::GE,   UnaryOp::Op::NOT};

    // Set
    operators["in"]       = {OpInfo::Arity::INFIX, 80, false, BinaryOp::Op::IN,       UnaryOp::Op::NOT};
    operators["subset"]   = {OpInfo::Arity::INFIX, 80, false, BinaryOp::Op::SUBSET,   UnaryOp::Op::NOT};
    operators["superset"]= {OpInfo::Arity::INFIX, 80, false, BinaryOp::Op::SUPERSET, UnaryOp::Op::NOT};
    operators["union"]    = {OpInfo::Arity::INFIX, 90, false, BinaryOp::Op::UNION,    UnaryOp::Op::NOT};
    operators["diff"]     = {OpInfo::Arity::INFIX, 90, false, BinaryOp::Op::DIFF,     UnaryOp::Op::NOT};
    operators["symdiff"]  = {OpInfo::Arity::INFIX, 90, false, BinaryOp::Op::SYMDIFF,  UnaryOp::Op::NOT};
    operators["intersect"]={OpInfo::Arity::INFIX,100, false, BinaryOp::Op::INTERSECT,UnaryOp::Op::NOT};

    // Range
    operators[".."]   = {OpInfo::Arity::INFIX, 105, false, BinaryOp::Op::RANGE, UnaryOp::Op::NOT};
    operators["..<"]  = {OpInfo::Arity::INFIX, 105, false, BinaryOp::Op::RANGE_HALF_OPEN_L, UnaryOp::Op::NOT};
    operators["<.."]  = {OpInfo::Arity::INFIX, 105, false, BinaryOp::Op::RANGE_HALF_OPEN_R, UnaryOp::Op::NOT};
    operators["<..<"]= {OpInfo::Arity::INFIX, 105, false, BinaryOp::Op::RANGE_OPEN, UnaryOp::Op::NOT};

    // Array / string concat
    operators["++"]   = {OpInfo::Arity::INFIX, 115, false, BinaryOp::Op::CONCAT, UnaryOp::Op::NOT};

    // Arithmetic
    operators["+"]    = {OpInfo::Arity::INFIX, 120, false, BinaryOp::Op::ADD, UnaryOp::Op::PLUS};
    operators["-"]    = {OpInfo::Arity::INFIX, 120, false, BinaryOp::Op::SUB, UnaryOp::Op::MINUS};
    operators["*"]    = {OpInfo::Arity::INFIX, 130, false, BinaryOp::Op::MUL, UnaryOp::Op::NOT};
    operators["/"]    = {OpInfo::Arity::INFIX, 130, false, BinaryOp::Op::DIV, UnaryOp::Op::NOT};
    operators["div"]  = {OpInfo::Arity::INFIX, 130, false, BinaryOp::Op::DIV_INT, UnaryOp::Op::NOT};
    operators["mod"]  = {OpInfo::Arity::INFIX, 130, false, BinaryOp::Op::MOD, UnaryOp::Op::NOT};
    operators["^"]    = {OpInfo::Arity::INFIX, 140, true,  BinaryOp::Op::POW, UnaryOp::Op::NOT};

    // Prefix unary
    operators["not"]  = {OpInfo::Arity::PREFIX, 110, false, BinaryOp::Op::EQ, UnaryOp::Op::NOT};

    // Collect all keyword-like operator names
    for (auto& kv : operators) {
        keywords.insert(kv.first);
    }
}

// ── Function initialization ──────────────────────────────────────
void MznBuiltins::initFunctions() {
    aggregations = {
        "forall", "exists", "clause", "iffall", "xorall",
        "sum", "product", "min", "max", "arg_min", "arg_max",
        "count", "length", "card"
    };
    conversions = {
        "bool2int", "bool2float", "int2float",
        "ceil", "floor", "round", "set2array", "array2set"
    };
    transcendentals = {
        "sqrt", "pow", "exp", "ln", "log", "log10", "log2",
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2",
        "sinh", "cosh", "tanh", "asinh", "acosh", "atanh"
    };

    for (auto& a : aggregations) keywords.insert(a);
    for (auto& c : conversions) keywords.insert(c);
    for (auto& t : transcendentals) keywords.insert(t);
}

// ── Global constraint initialization ─────────────────────────────
void MznBuiltins::initGlobals() {
    globals = {
        "all_different", "all_equal", "all_different_except_0",
        "count", "count_eq", "count_geq", "count_gt", "count_le", "count_lt",
        "global_cardinality", "global_cardinality_closed",
        "global_cardinality_low_up",
        "cumulative", "disjunctive", "alternative", "span", "unary",
        "element", "increasing", "decreasing",
        "strictly_increasing", "strictly_decreasing",
        "sort", "arg_sort", "arg_min", "arg_max",
        "circuit", "subcircuit", "path",
        "table", "regular", "mdd",
        "lex_less", "lex_lesseq", "lex_greater", "lex_greatereq", "lex2",
        "nvalue", "diffn", "diffn_k", "geost", "geost_bb", "geost_smallest_bb",
        "at_least", "at_most", "exactly", "among",
        "inverse", "inverse_set", "link_set_to_booleans", "int_set_channel",
        "bin_packing", "bin_packing_capa", "bin_packing_load",
        "network_flow", "network_flow_cost",
        "bounded_path", "steiner_tree", "weighted_spanning_tree",
        "cumulative_task", "schedule",
        "soft_all_different", "soft_global_cardinality", "soft_regular",
        "value_precede", "value_precede_chain",
        "member", "range", "roots",
        "partition_set", "redundant_constraint",
        "symmetry_breaking_constraint", "implied_constraint"
    };
    soft_globals = {
        "soft_all_different", "soft_global_cardinality", "soft_regular"
    };
    redundant_globals = {
        "redundant_constraint", "symmetry_breaking_constraint"
    };
}

// ── Annotation initialization ────────────────────────────────────
void MznBuiltins::initAnnotations() {
    annotations = {
        "int_search", "bool_search", "set_search",
        "seq_search", "priority_search",
        "int_default_search", "bool_default_search", "set_default_search",
        "restart_constant", "restart_linear", "restart_geometric",
        "relax_and_reconstruct"
    };
}

// ── Queries ──────────────────────────────────────────────────────
bool MznBuiltins::isOperator(const std::string& name) const {
    return operators.find(name) != operators.end();
}

const OpInfo* MznBuiltins::getOperatorInfo(const std::string& name) const {
    auto it = operators.find(name);
    return (it != operators.end()) ? &it->second : nullptr;
}

bool MznBuiltins::isBuiltinFunction(const std::string& name) const {
    return aggregations.find(name) != aggregations.end()
        || conversions.find(name) != conversions.end()
        || transcendentals.find(name) != transcendentals.end()
        || isGlobalConstraint(name)
        || isAnnotation(name);
}

std::vector<BuiltinSig> MznBuiltins::getSignatures(const std::string& name) const {
    auto it = functions.find(name);
    return (it != functions.end()) ? it->second : std::vector<BuiltinSig>{};
}

bool MznBuiltins::isGlobalConstraint(const std::string& name) const {
    return globals.find(name) != globals.end();
}

bool MznBuiltins::isGlobalSoftConstraint(const std::string& name) const {
    return soft_globals.find(name) != soft_globals.end();
}

bool MznBuiltins::isRedundantConstraint(const std::string& name) const {
    return redundant_globals.find(name) != redundant_globals.end();
}

bool MznBuiltins::isAnnotation(const std::string& name) const {
    return annotations.find(name) != annotations.end();
}

bool MznBuiltins::isAggregation(const std::string& name) const {
    return aggregations.find(name) != aggregations.end();
}

bool MznBuiltins::isConversion(const std::string& name) const {
    return conversions.find(name) != conversions.end();
}

bool MznBuiltins::isTranscendental(const std::string& name) const {
    return transcendentals.find(name) != transcendentals.end();
}

const std::unordered_set<std::string>& MznBuiltins::getAllKeywords() const {
    return keywords;
}

const std::unordered_set<std::string>& MznBuiltins::getAllGlobals() const {
    return globals;
}

} // namespace SOMTParser::MiniZinc
