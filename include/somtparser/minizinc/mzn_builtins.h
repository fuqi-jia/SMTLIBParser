/* -*- Header -*-
 *
 * MiniZinc Frontend — Built-in Function / Operator Registry
 *
 * Comprehensive registry of all MiniZinc 2.8+ built-in operators,
 * functions, aggregations, conversions, global constraints, and annotations.
 */

#ifndef MZN_BUILTINS_H
#define MZN_BUILTINS_H

#include "somtparser/minizinc/mzn_ast.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace SOMTParser::MiniZinc {

// ── Built-in function signature ──────────────────────────────────
struct BuiltinSig {
    std::string name;
    std::vector<std::shared_ptr<TypeInst>> param_types;
    std::shared_ptr<TypeInst> ret_type;
    bool is_variadic = false;     // e.g., sum(array[int])
    bool is_poly = false;         // polymorphic (works on any comparable)
};

// ── Operator metadata ────────────────────────────────────────────
struct OpInfo {
    enum class Arity { PREFIX, INFIX, POSTFIX };
    Arity arity;
    int precedence = 0;
    bool right_associative = false;
    BinaryOp::Op bin_op;       // valid for infix
    UnaryOp::Op un_op;         // valid for prefix
};

/**
 * @brief Singleton registry of all MiniZinc built-ins.
 */
class MznBuiltins {
public:
    static MznBuiltins& get();

    // ── Operator queries ───────────────────────────────────────
    bool isOperator(const std::string& name) const;
    const OpInfo* getOperatorInfo(const std::string& name) const;

    // ── Function queries ───────────────────────────────────────
    bool isBuiltinFunction(const std::string& name) const;
    std::vector<BuiltinSig> getSignatures(const std::string& name) const;

    // ── Global constraint queries ──────────────────────────────
    bool isGlobalConstraint(const std::string& name) const;
    bool isGlobalSoftConstraint(const std::string& name) const;
    bool isRedundantConstraint(const std::string& name) const;

    // ── Annotation queries ─────────────────────────────────────
    bool isAnnotation(const std::string& name) const;

    // ── Category checks ────────────────────────────────────────
    bool isAggregation(const std::string& name) const;   // sum, forall, exists, etc.
    bool isConversion(const std::string& name) const;    // bool2int, int2float, etc.
    bool isTranscendental(const std::string& name) const;// sin, cos, exp, ln, etc.

    // ── Predefined identifier sets ─────────────────────────────
    const std::unordered_set<std::string>& getAllKeywords() const;
    const std::unordered_set<std::string>& getAllGlobals() const;

private:
    MznBuiltins();
    void initOperators();
    void initFunctions();
    void initGlobals();
    void initAnnotations();

    std::unordered_map<std::string, OpInfo> operators;
    std::unordered_map<std::string, std::vector<BuiltinSig>> functions;
    std::unordered_set<std::string> globals;
    std::unordered_set<std::string> soft_globals;
    std::unordered_set<std::string> redundant_globals;
    std::unordered_set<std::string> annotations;
    std::unordered_set<std::string> aggregations;
    std::unordered_set<std::string> conversions;
    std::unordered_set<std::string> transcendentals;
    std::unordered_set<std::string> keywords;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_BUILTINS_H
