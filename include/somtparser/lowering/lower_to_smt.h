/* -*- Header -*-
 *
 * LowerToSmt — Registry-driven lowering from Unified IR to DAGNode.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef LOWER_TO_SMT_H
#define LOWER_TO_SMT_H

#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"

#include <string>
#include <vector>

namespace SOMTParser::Lowering {

/**
 * Lower a Unified IR model to SMT DAGNodes using the op registry.
 *
 * Usage:
 *   LowerToSmt lowerer(parser, registry);
 *   lowerer.lowerModel(unified_model);
 *   auto dag = lowerer.getAssertion(); // combined assertion
 */
class LowerToSmt {
public:
    LowerToSmt(Parser& parser, const Unified::UnifiedOpRegistry& registry);

    /** Lower an entire UnifiedModel. Returns the top-level assertion (AND of all constraints). */
    std::shared_ptr<DAGNode> lowerModel(const Unified::UnifiedModel& model);

    /** Lower a single UnifiedExpr to DAGNode. */
    std::shared_ptr<DAGNode> lowerExpr(const Unified::ExprPtr& expr);

    /** Access the variable map (unified name -> DAGNode). */
    const std::unordered_map<std::string, std::shared_ptr<DAGNode>>& varMap() const { return var_map_; }

    /** Get any lowering errors. */
    const std::vector<std::string>& errors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }

private:
    Parser& parser_;
    const Unified::UnifiedOpRegistry& registry_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> var_map_;
    std::vector<std::string> errors_;

    std::shared_ptr<DAGNode> lowerOpNode(const Unified::UnifiedExpr::OpNode& op);
    std::shared_ptr<DAGNode> lowerNative(const Unified::UnifiedOpDef& def,
                                          const std::vector<std::shared_ptr<DAGNode>>& args);
    std::shared_ptr<DAGNode> lowerDecompose(const Unified::UnifiedOpDef& def,
                                             const std::vector<std::shared_ptr<DAGNode>>& args);

    std::shared_ptr<Sort> lowerSort(const Unified::UnifiedSort& sort);
    std::shared_ptr<DAGNode> lowerVarDecl(const Unified::UnifiedVarDecl& decl);

    /** Convert a Bool-typed DAGNode to Int via (ite x 1 0). No-op if already Int. */
    std::shared_ptr<DAGNode> coerceToInt(std::shared_ptr<DAGNode> node);

    void addError(const std::string& msg);
};

} // namespace SOMTParser::Lowering

#endif // LOWER_TO_SMT_H
