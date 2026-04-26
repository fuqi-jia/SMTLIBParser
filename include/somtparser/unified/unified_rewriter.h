/* -*- Header -*-
 *
 * UnifiedRewriter — Bottom-up rewrite with memo + fixpoint over Unified IR.
 *
 * Handlers inspect a single node (children already rewritten).
 * Multi-step effects achieved via fixpoint iteration.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef UNIFIED_REWRITER_H
#define UNIFIED_REWRITER_H

#include "somtparser/unified/unified_ir.h"

#include <functional>
#include <unordered_map>

namespace SOMTParser::Unified {

/**
 * Context passed to rewrite handlers. Provides rebuild helpers.
 */
class RewriteContext {
public:
    /** If newArgs equals old's args (by pointer), return old. Otherwise rebuild. */
    static ExprPtr rebuildLike(const UnifiedExpr::OpNode& old,
                                const std::vector<ExprPtr>& newArgs);

    /** Rebuild a quantifier with new generators and body. */
    static ExprPtr rebuildLike(const UnifiedExpr::QuantExpr& old,
                                const std::vector<std::pair<std::string, ExprPtr>>& newGens,
                                ExprPtr newBody);

    /** Rebuild an ITE with new sub-expressions. */
    static ExprPtr rebuildLike(const UnifiedExpr::IteExpr& old,
                                ExprPtr newCond, ExprPtr newThen, ExprPtr newElse);

    /** Rebuild a let with new locals and body. */
    static ExprPtr rebuildLike(const UnifiedExpr::LetExpr& old,
                                std::vector<UnifiedVarDecl> newLocals, ExprPtr newBody);
};

/**
 * Rewriter: bottom-up rewrite with memo; rewrite(root) runs fixpoint by default.
 */
class UnifiedRewriter {
public:
    static constexpr unsigned kDefaultMaxFixpointRounds = 64u;

    UnifiedRewriter() = default;

    /** One bottom-up pass with memo. */
    ExprPtr rewriteOnce(ExprPtr root);

    /** Rewrite until root stabilizes or max rounds reached. */
    ExprPtr rewrite(ExprPtr root, bool enable_fixpoint = true,
                    unsigned max_rounds = kDefaultMaxFixpointRounds);

    /** Register a handler for a specific unified op name. */
    void onOp(const std::string& unified_name,
              std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)> handler);

    /** Register a catch-all handler for any op. Called if no specific handler matches. */
    void onAnyOp(std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)> handler);

    /** Register handlers for specific expression kinds. */
    void onLiteral(std::function<ExprPtr(const UnifiedExpr::Literal&)> handler);
    void onIdent(std::function<ExprPtr(const UnifiedExpr::Ident&)> handler);
    void onArrayLit(std::function<ExprPtr(const UnifiedExpr::ArrayLit&, const std::vector<ExprPtr>&)> handler);
    void onSetLit(std::function<ExprPtr(const UnifiedExpr::SetLit&, const std::vector<ExprPtr>&)> handler);
    void onLet(std::function<ExprPtr(const UnifiedExpr::LetExpr&, std::vector<UnifiedVarDecl>, ExprPtr)> handler);
    void onIte(std::function<ExprPtr(const UnifiedExpr::IteExpr&, ExprPtr, ExprPtr, ExprPtr)> handler);
    void onQuant(std::function<ExprPtr(const UnifiedExpr::QuantExpr&, const std::vector<std::pair<std::string, ExprPtr>>&, ExprPtr)> handler);

    void clearMemo() { memo_.clear(); }

private:
    ExprPtr rewriteImpl(ExprPtr root);

    // Op handlers: unified_name -> handler
    std::unordered_map<std::string,
        std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)>> op_handlers_;
    std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)> any_op_handler_;

    // Kind handlers
    std::function<ExprPtr(const UnifiedExpr::Literal&)> on_literal_;
    std::function<ExprPtr(const UnifiedExpr::Ident&)> on_ident_;
    std::function<ExprPtr(const UnifiedExpr::ArrayLit&, const std::vector<ExprPtr>&)> on_array_;
    std::function<ExprPtr(const UnifiedExpr::SetLit&, const std::vector<ExprPtr>&)> on_set_;
    std::function<ExprPtr(const UnifiedExpr::LetExpr&, std::vector<UnifiedVarDecl>, ExprPtr)> on_let_;
    std::function<ExprPtr(const UnifiedExpr::IteExpr&, ExprPtr, ExprPtr, ExprPtr)> on_ite_;
    std::function<ExprPtr(const UnifiedExpr::QuantExpr&, const std::vector<std::pair<std::string, ExprPtr>>&, ExprPtr)> on_quant_;

    std::unordered_map<UnifiedExpr*, ExprPtr> memo_;
};

/** Install minimal default rules: identity (no-op). */
void installDefaultRewriteRules(UnifiedRewriter& r);

} // namespace SOMTParser::Unified

#endif // UNIFIED_REWRITER_H
