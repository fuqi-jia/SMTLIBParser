/* -*- C++ -*-
 *
 * UnifiedRewriter implementation
 */

#include "somtparser/unified/unified_rewriter.h"

namespace SOMTParser::Unified {

// ── RewriteContext helpers ─────────────────────────────────────────

ExprPtr RewriteContext::rebuildLike(const UnifiedExpr::OpNode& old,
                                     const std::vector<ExprPtr>& newArgs) {
    // Check if any arg changed
    bool same = true;
    for (size_t i = 0; i < old.args.size() && same; ++i) {
        if (i < newArgs.size() && old.args[i].get() != newArgs[i].get())
            same = false;
    }
    if (same && old.args.size() == newArgs.size()) {
        // Return a node that shares the same op but we can't easily return old
        // because old is not an ExprPtr. Caller should handle identity.
        // For simplicity, always rebuild.
    }
    auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::OP, SourceLoc{});
    node->data = UnifiedExpr::OpNode{old.op, newArgs, old.generators};
    return node;
}

ExprPtr RewriteContext::rebuildLike(const UnifiedExpr::QuantExpr& old,
                                     const std::vector<std::pair<std::string, ExprPtr>>& newGens,
                                     ExprPtr newBody) {
    auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::FORALL, SourceLoc{});
    node->data = UnifiedExpr::QuantExpr{newGens, newBody};
    return node;
}

ExprPtr RewriteContext::rebuildLike(const UnifiedExpr::IteExpr& old,
                                     ExprPtr newCond, ExprPtr newThen, ExprPtr newElse) {
    auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ITE, SourceLoc{});
    node->data = UnifiedExpr::IteExpr{newCond, newThen, newElse};
    return node;
}

ExprPtr RewriteContext::rebuildLike(const UnifiedExpr::LetExpr& old,
                                     std::vector<UnifiedVarDecl> newLocals, ExprPtr newBody) {
    auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::LET, SourceLoc{});
    node->data = UnifiedExpr::LetExpr{std::move(newLocals), newBody};
    return node;
}

// ── UnifiedRewriter ────────────────────────────────────────────────

ExprPtr UnifiedRewriter::rewriteOnce(ExprPtr root) {
    if (!root) return nullptr;
    memo_.clear();
    return rewriteImpl(root);
}

ExprPtr UnifiedRewriter::rewrite(ExprPtr root, bool enable_fixpoint, unsigned max_rounds) {
    if (!root || !enable_fixpoint) return rewriteOnce(root);

    ExprPtr current = root;
    for (unsigned round = 0; round < max_rounds; ++round) {
        memo_.clear();
        ExprPtr next = rewriteImpl(current);
        if (next.get() == current.get()) return current;
        current = next;
    }
    return current;
}

ExprPtr UnifiedRewriter::rewriteImpl(ExprPtr root) {
    if (!root) return nullptr;

    auto it = memo_.find(root.get());
    if (it != memo_.end()) return it->second;

    ExprPtr result = root; // default: identity

    switch (root->kind) {
        case UnifiedExpr::Kind::LITERAL: {
            if (on_literal_) {
                auto r = on_literal_(*root->asLiteral());
                if (r) result = r;
            }
            break;
        }

        case UnifiedExpr::Kind::IDENT: {
            if (on_ident_) {
                auto r = on_ident_(*root->asIdent());
                if (r) result = r;
            }
            break;
        }

        case UnifiedExpr::Kind::OP: {
            auto* op = root->asOp();
            if (!op) break;

            std::vector<ExprPtr> newArgs;
            newArgs.reserve(op->args.size());
            for (auto& arg : op->args) {
                newArgs.push_back(rewriteImpl(arg));
            }

            bool changed = false;
            for (size_t i = 0; i < op->args.size(); ++i) {
                if (op->args[i].get() != newArgs[i].get()) changed = true;
            }

            // Try specific handler
            std::string op_name;
            // We'd need registry access to get unified_name from op.id
            // For now, handlers are keyed by something the user provides.
            // This is a limitation; we improve in Phase 5.
            bool handled = false;
            if (!op_name.empty() && op_handlers_.count(op_name)) {
                auto r = op_handlers_[op_name](*op, newArgs);
                if (r) { result = r; handled = true; }
            }
            if (!handled && any_op_handler_) {
                auto r = any_op_handler_(*op, newArgs);
                if (r) { result = r; handled = true; }
            }
            if (!handled && changed) {
                result = RewriteContext::rebuildLike(*op, newArgs);
            }
            break;
        }

        case UnifiedExpr::Kind::ARRAY_LIT: {
            auto* arr = root->asArray();
            if (!arr) break;
            std::vector<ExprPtr> newElems;
            newElems.reserve(arr->elems.size());
            bool changed = false;
            for (auto& e : arr->elems) {
                auto ne = rewriteImpl(e);
                if (ne.get() != e.get()) changed = true;
                newElems.push_back(ne);
            }
            if (on_array_) {
                auto r = on_array_(*arr, newElems);
                if (r) result = r;
            } else if (changed) {
                auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::ARRAY_LIT, SourceLoc{});
                node->data = UnifiedExpr::ArrayLit{std::move(newElems)};
                result = node;
            }
            break;
        }

        case UnifiedExpr::Kind::SET_LIT: {
            auto* s = root->asSet();
            if (!s) break;
            std::vector<ExprPtr> newElems;
            newElems.reserve(s->elems.size());
            bool changed = false;
            for (auto& e : s->elems) {
                auto ne = rewriteImpl(e);
                if (ne.get() != e.get()) changed = true;
                newElems.push_back(ne);
            }
            if (on_set_) {
                auto r = on_set_(*s, newElems);
                if (r) result = r;
            } else if (changed) {
                auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::SET_LIT, SourceLoc{});
                node->data = UnifiedExpr::SetLit{std::move(newElems)};
                result = node;
            }
            break;
        }

        case UnifiedExpr::Kind::TUPLE_LIT: {
            auto* t = root->as<UnifiedExpr::TupleLit>();
            if (!t) break;
            std::vector<ExprPtr> newElems;
            newElems.reserve(t->elems.size());
            bool changed = false;
            for (auto& e : t->elems) {
                auto ne = rewriteImpl(e);
                if (ne.get() != e.get()) changed = true;
                newElems.push_back(ne);
            }
            if (changed) {
                auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::TUPLE_LIT, SourceLoc{});
                node->data = UnifiedExpr::TupleLit{std::move(newElems)};
                result = node;
            }
            break;
        }

        case UnifiedExpr::Kind::RECORD_LIT: {
            auto* r = root->as<UnifiedExpr::RecordLit>();
            if (!r) break;
            bool changed = false;
            std::vector<std::pair<std::string, ExprPtr>> newFields;
            newFields.reserve(r->fields.size());
            for (auto& [name, val] : r->fields) {
                auto nv = rewriteImpl(val);
                if (nv.get() != val.get()) changed = true;
                newFields.emplace_back(name, nv);
            }
            if (changed) {
                auto node = std::make_shared<UnifiedExpr>(UnifiedExpr::Kind::RECORD_LIT, SourceLoc{});
                node->data = UnifiedExpr::RecordLit{std::move(newFields)};
                result = node;
            }
            break;
        }

        case UnifiedExpr::Kind::LET: {
            auto* let = root->asLet();
            if (!let) break;
            std::vector<UnifiedVarDecl> newLocals;
            newLocals.reserve(let->locals.size());
            bool changed = false;
            for (auto& local : let->locals) {
                UnifiedVarDecl nl = local;
                if (local.init) {
                    nl.init = rewriteImpl(local.init);
                    if (nl.init.get() != local.init.get()) changed = true;
                }
                newLocals.push_back(std::move(nl));
            }
            auto newBody = rewriteImpl(let->body);
            if (newBody.get() != let->body.get()) changed = true;

            if (on_let_) {
                auto r = on_let_(*let, std::move(newLocals), newBody);
                if (r) result = r;
            } else if (changed) {
                result = RewriteContext::rebuildLike(*let, std::move(newLocals), newBody);
            }
            break;
        }

        case UnifiedExpr::Kind::ITE: {
            auto* ite = root->asIte();
            if (!ite) break;
            auto newCond = rewriteImpl(ite->cond);
            auto newThen = rewriteImpl(ite->then_expr);
            auto newElse = rewriteImpl(ite->else_expr);
            bool changed = (newCond.get() != ite->cond.get() ||
                           newThen.get() != ite->then_expr.get() ||
                           newElse.get() != ite->else_expr.get());
            if (on_ite_) {
                auto r = on_ite_(*ite, newCond, newThen, newElse);
                if (r) result = r;
            } else if (changed) {
                result = RewriteContext::rebuildLike(*ite, newCond, newThen, newElse);
            }
            break;
        }

        case UnifiedExpr::Kind::FORALL:
        case UnifiedExpr::Kind::EXISTS: {
            auto* q = root->asQuant();
            if (!q) break;
            std::vector<std::pair<std::string, ExprPtr>> newGens;
            newGens.reserve(q->generators.size());
            bool changed = false;
            for (auto& [var, set_expr] : q->generators) {
                auto ns = rewriteImpl(set_expr);
                if (ns.get() != set_expr.get()) changed = true;
                newGens.emplace_back(var, ns);
            }
            auto newBody = rewriteImpl(q->body);
            if (newBody.get() != q->body.get()) changed = true;

            if (on_quant_) {
                auto r = on_quant_(*q, newGens, newBody);
                if (r) result = r;
            } else if (changed) {
                auto kind = root->kind;
                auto node = std::make_shared<UnifiedExpr>(kind, SourceLoc{});
                node->data = UnifiedExpr::QuantExpr{std::move(newGens), newBody};
                result = node;
            }
            break;
        }
    }

    memo_[root.get()] = result;
    return result;
}

// ── Handler registration ───────────────────────────────────────────

void UnifiedRewriter::onOp(const std::string& unified_name,
                            std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)> handler) {
    op_handlers_[unified_name] = std::move(handler);
}

void UnifiedRewriter::onAnyOp(std::function<ExprPtr(const UnifiedExpr::OpNode&, const std::vector<ExprPtr>&)> handler) {
    any_op_handler_ = std::move(handler);
}

void UnifiedRewriter::onLiteral(std::function<ExprPtr(const UnifiedExpr::Literal&)> handler) {
    on_literal_ = std::move(handler);
}

void UnifiedRewriter::onIdent(std::function<ExprPtr(const UnifiedExpr::Ident&)> handler) {
    on_ident_ = std::move(handler);
}

void UnifiedRewriter::onArrayLit(std::function<ExprPtr(const UnifiedExpr::ArrayLit&, const std::vector<ExprPtr>&)> handler) {
    on_array_ = std::move(handler);
}

void UnifiedRewriter::onSetLit(std::function<ExprPtr(const UnifiedExpr::SetLit&, const std::vector<ExprPtr>&)> handler) {
    on_set_ = std::move(handler);
}

void UnifiedRewriter::onLet(std::function<ExprPtr(const UnifiedExpr::LetExpr&, std::vector<UnifiedVarDecl>, ExprPtr)> handler) {
    on_let_ = std::move(handler);
}

void UnifiedRewriter::onIte(std::function<ExprPtr(const UnifiedExpr::IteExpr&, ExprPtr, ExprPtr, ExprPtr)> handler) {
    on_ite_ = std::move(handler);
}

void UnifiedRewriter::onQuant(std::function<ExprPtr(const UnifiedExpr::QuantExpr&, const std::vector<std::pair<std::string, ExprPtr>>&, ExprPtr)> handler) {
    on_quant_ = std::move(handler);
}

// ── Default rules ──────────────────────────────────────────────────

void installDefaultRewriteRules(UnifiedRewriter& r) {
    (void)r;
    // Default: identity (no-op). Users register handlers as needed.
}

} // namespace SOMTParser::Unified
