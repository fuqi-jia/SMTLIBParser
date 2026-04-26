/* -*- Header -*-
 *
 * MiniZinc Frontend — SMT/OMT Lowering Backend
 *
 * Translates MiniZinc AST into SOMTParser SMT/OMT DAG (IR).
 */

#ifndef MZN_LOWER_SMT_H
#define MZN_LOWER_SMT_H

#include "somtparser/frontends/minizinc/mzn_lower_backend.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace SOMTParser {
    class Parser;
}

namespace SOMTParser::MiniZinc {

// Forward declarations
class MznSymbolTable;
class MznEvaluator;
class GlobalConstraintDecomposer;

/**
 * @brief SMT/OMT lowering backend implementation.
 *
 * Maps MiniZinc types to SMT sorts, expressions to DAG nodes,
 * and global constraints to their SMT decompositions.
 */
class SmtLoweringBackend : public LoweringBackend {
public:
    SmtLoweringBackend(Parser& parser,
                       MznSymbolTable& sym_table,
                       MznEvaluator& evaluator);

    // ── LoweringBackend interface ──────────────────────────────
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

    bool hadErrors() const override { return !errors_.empty(); }
    std::vector<std::string> getErrors() const override { return errors_; }

    // ── Internal accessors ─────────────────────────────────────
    Parser& getParser() const { return parser_; }
    std::shared_ptr<DAGNode> lookupVar(const std::string& name) const;
    std::shared_ptr<Sort> lookupEnumSort(const std::string& name) const;

private:
    Parser& parser_;
    MznSymbolTable& sym_table_;
    MznEvaluator& evaluator_;
    std::unique_ptr<GlobalConstraintDecomposer> globals_;

    // Variable / enum maps
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> var_map_;
    std::unordered_map<std::string, std::shared_ptr<Sort>> enum_sort_map_;
    std::vector<std::string> errors_;

    // ── Expression lowering helpers ────────────────────────────
    std::shared_ptr<DAGNode> lowerBoolOp(const BinaryOp& op);
    std::shared_ptr<DAGNode> lowerArithOp(const BinaryOp& op);
    std::shared_ptr<DAGNode> lowerCompOp(const BinaryOp& op);
    std::shared_ptr<DAGNode> lowerSetOp(const BinaryOp& op);
    std::shared_ptr<DAGNode> lowerUnaryOp(const UnaryOp& op);
    std::shared_ptr<DAGNode> lowerCall(const CallExpr& call);
    std::shared_ptr<DAGNode> lowerArrayAccess(const ArrayAccess& acc);
    std::shared_ptr<DAGNode> lowerIfThenElse(const IfThenElse& ite);
    std::shared_ptr<DAGNode> lowerLet(const LetExpr& let);
    std::shared_ptr<DAGNode> lowerArrayLit(const ArrayLit& arr);
    std::shared_ptr<DAGNode> lowerSetLit(const SetLit& set);
    std::shared_ptr<DAGNode> lowerAnnotated(const Annotated& ann);

    // ── Array / Set / Enum lowering ────────────────────────────
    std::shared_ptr<DAGNode> lowerArrayTypeExpr(
        const TypeInst& arr_type,
        const ExprPtr& init);
    std::shared_ptr<DAGNode> lowerSetVarExpr(
        const TypeInst& set_type,
        const std::string& name);
    std::shared_ptr<DAGNode> lowerOptTypeExpr(
        const TypeInst& opt_type,
        const std::string& name);
    std::shared_ptr<DAGNode> lowerEnumTypeExpr(
        const TypeInst& enum_type,
        const std::string& name);

    // ── Domain assertions ──────────────────────────────────────
    std::vector<std::shared_ptr<DAGNode>> lowerDomain(
        const std::string& var_name,
        const ExprPtr& domain_expr);

    void addError(const std::string& msg);
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_LOWER_SMT_H
