/* -*- Header -*-
 *
 * MiniZinc Frontend — OR-Tools CP-SAT Lowering Backend (Reserved)
 *
 * Phase 7 extension: maps MiniZinc AST to OR-Tools CP-SAT model.
 */

#ifndef MZN_LOWER_ORTOOLS_H
#define MZN_LOWER_ORTOOLS_H

#include "somtparser/minizinc/mzn_lower_backend.h"

// OR-Tools backend is optional and compiled only when the dependency is present.
// For now, this header defines the interface skeleton.

namespace SOMTParser::MiniZinc {

#ifdef SOMTPARSER_ENABLE_ORTOOLS

/**
 * @brief OR-Tools CP-SAT lowering backend (Phase 7).
 */
class OrToolsLoweringBackend : public LoweringBackend {
public:
    OrToolsLoweringBackend();
    ~OrToolsLoweringBackend() override;

    std::shared_ptr<Sort> lowerType(const TypeInst& ti) override;
    std::shared_ptr<DAGNode> lowerExpr(const ExprPtr& expr) override;
    std::shared_ptr<DAGNode> lowerVarDecl(
        const VarDeclItem& decl,
        std::vector<std::shared_ptr<DAGNode>>& out_assertions) override;
    std::shared_ptr<DAGNode> lowerConstraint(const ConstraintItem& ci) override;
    void lowerSolveItem(const SolveItem& si) override;
    void lowerOutputItem(const OutputItem& oi) override;
    std::vector<std::shared_ptr<DAGNode>> decomposeGlobal(
        const std::string& name,
        const std::vector<ExprPtr>& args) override;
    void finalize() override;

    bool hadErrors() const override;
    std::vector<std::string> getErrors() const override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

#else // SOMTPARSER_ENABLE_ORTOOLS

// Stub: when OR-Tools is not available, provide a placeholder class declaration
// so that downstream code can still reference the type name (but not instantiate).
class OrToolsLoweringBackend;

#endif // SOMTPARSER_ENABLE_ORTOOLS

} // namespace SOMTParser::MiniZinc

#endif // MZN_LOWER_ORTOOLS_H
