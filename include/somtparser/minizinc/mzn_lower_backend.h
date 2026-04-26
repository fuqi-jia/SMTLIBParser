/* -*- Header -*-
 *
 * MiniZinc Frontend — Lowering Backend Abstract Interface
 *
 * Pluggable backend for translating MiniZinc AST into target IR.
 */

#ifndef MZN_LOWER_BACKEND_H
#define MZN_LOWER_BACKEND_H

#include "somtparser/minizinc/mzn_ast.h"
#include "somtparser/ir/dag.h"
#include <memory>
#include <vector>
#include <string>

namespace SOMTParser {
    class Parser;
    struct ParserContext;
}

namespace SOMTParser::MiniZinc {

// Forward declarations
class MznSymbolTable;
class MznEvaluator;

/**
 * @brief Abstract interface for lowering a MiniZinc Model to a backend IR.
 *
 * Implementations: SmtLoweringBackend, OrToolsLoweringBackend (reserved).
 */
class LoweringBackend {
public:
    virtual ~LoweringBackend() = default;

    // ── Type lowering ──────────────────────────────────────────
    virtual std::shared_ptr<Sort> lowerType(const TypeInst& ti) = 0;

    // ── Expression lowering ────────────────────────────────────
    virtual std::shared_ptr<DAGNode> lowerExpr(const ExprPtr& expr) = 0;

    // ── Variable declaration lowering ──────────────────────────
    virtual std::shared_ptr<DAGNode> lowerVarDecl(
        const VarDeclItem& decl,
        std::vector<std::shared_ptr<DAGNode>>& out_assertions) = 0;

    // ── Constraint lowering ────────────────────────────────────
    virtual std::shared_ptr<DAGNode> lowerConstraint(const ConstraintItem& ci) = 0;

    // ── Solve item lowering ────────────────────────────────────
    virtual void lowerSolveItem(const SolveItem& si) = 0;

    // ── Output item lowering ───────────────────────────────────
    virtual void lowerOutputItem(const OutputItem& oi) = 0;

    // ── Global constraint decomposition ────────────────────────
    virtual std::vector<std::shared_ptr<DAGNode>> decomposeGlobal(
        const std::string& name,
        const std::vector<ExprPtr>& args) = 0;

    // ── Finalization ───────────────────────────────────────────
    virtual void finalize() = 0;

    // ── Error / unsupported ────────────────────────────────────
    virtual bool hadErrors() const { return false; }
    virtual std::vector<std::string> getErrors() const { return {}; }
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_LOWER_BACKEND_H
