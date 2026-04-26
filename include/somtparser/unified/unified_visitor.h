/* -*- Header -*-
 *
 * UnifiedVisitor — Tree traversal over Unified IR.
 *
 * Pre-order walk; each node visited once. Override visit() to process.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef UNIFIED_VISITOR_H
#define UNIFIED_VISITOR_H

#include "somtparser/unified/unified_ir.h"

#include <unordered_set>

namespace SOMTParser::Unified {

class UnifiedVisitor {
public:
    virtual ~UnifiedVisitor() = default;

    /** Override to process each expression node. Default no-op. */
    virtual void visit(UnifiedExpr& expr) { (void)expr; }
    virtual void visit(const UnifiedExpr& expr) { (void)expr; }

    /** Walk the entire tree rooted at expr. Calls visit() once per node (pre-order). */
    void walk(ExprPtr root);
    void walk(const ExprPtr& root) const;

    /** Walk all expressions in a model. */
    void walkModel(UnifiedModel& model);
    void walkModel(const UnifiedModel& model) const;

protected:
    /** Recursively visit children of expr (called by default walk implementation). */
    void visitChildren(UnifiedExpr& expr);
    void visitChildren(const UnifiedExpr& expr) const;

private:
    std::unordered_set<const UnifiedExpr*> visited_;
};

} // namespace SOMTParser::Unified

#endif // UNIFIED_VISITOR_H
