/* -*- Header -*-
 *
 * MiniZinc Frontend — Compile-time Evaluator
 *
 * Evaluates `par` expressions and .dzn data to constant values.
 */

#ifndef MZN_EVALUATOR_H
#define MZN_EVALUATOR_H

#include "somtparser/frontends/minizinc/mzn_ast.h"
#include <memory>
#include <vector>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

// Forward declaration
class MznSymbolTable;

/**
 * @brief Result of evaluating a MiniZinc expression.
 *
 * For successful evaluation, holds a constant Expr.
 * For failure, records an error message.
 */
struct EvalResult {
    bool success = false;
    ExprPtr value; // constant expression (IntLit, BoolLit, FloatLit, etc.)
    std::string error;

    static EvalResult ok(ExprPtr v) {
        EvalResult r;
        r.success = true;
        r.value = std::move(v);
        return r;
    }
    static EvalResult err(std::string msg) {
        EvalResult r;
        r.error = std::move(msg);
        return r;
    }
};

/**
 * @brief Compile-time evaluator for MiniZinc par expressions.
 *
 * Operates on a fully parsed AST.  Requires that all referenced identifiers
 * are par (not var) and are already defined in the symbol table.
 */
class MznEvaluator {
public:
    explicit MznEvaluator(MznSymbolTable* sym_table = nullptr);

    // ── Main entry point ───────────────────────────────────────
    EvalResult evaluate(const ExprPtr& expr);

    // ── Helpers ────────────────────────────────────────────────
    bool isParExpr(const ExprPtr& expr) const;
    bool isFixed(const std::string& name) const;
    void markFixed(const std::string& name, ExprPtr value);

    // ── Built-in par function evaluation ───────────────────────
    EvalResult evalBuiltin(const std::string& name,
                           const std::vector<ExprPtr>& args);

private:
    MznSymbolTable* sym_table;
    std::unordered_map<std::string, ExprPtr> fixed_values;

    EvalResult evalBoolOp(const BinaryOp& op);
    EvalResult evalArithOp(const BinaryOp& op);
    EvalResult evalCompOp(const BinaryOp& op);
    EvalResult evalSetOp(const BinaryOp& op);
    EvalResult evalUnaryOp(const UnaryOp& op);
    EvalResult evalCall(const CallExpr& call);
    EvalResult evalArrayLit(const ArrayLit& arr);
    EvalResult evalSetLit(const SetLit& set);
    EvalResult evalIfThenElse(const IfThenElse& ite);
    EvalResult evalArrayAccess(const ArrayAccess& acc);
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_EVALUATOR_H
