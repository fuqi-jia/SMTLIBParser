/* -*- C++ -*-
 *
 * UnifiedVisitor implementation
 */

#include "somtparser/unified/unified_visitor.h"

namespace SOMTParser::Unified {

void UnifiedVisitor::visitChildren(UnifiedExpr& expr) {
    switch (expr.kind) {
        case UnifiedExpr::Kind::LITERAL:
        case UnifiedExpr::Kind::IDENT:
            break;

        case UnifiedExpr::Kind::OP: {
            if (auto* op = expr.asOp()) {
                for (auto& arg : op->args) {
                    if (arg) walk(arg);
                }
            }
            break;
        }

        case UnifiedExpr::Kind::ARRAY_LIT: {
            if (auto* arr = expr.asArray()) {
                for (auto& elem : arr->elems) {
                    if (elem) walk(elem);
                }
            }
            break;
        }

        case UnifiedExpr::Kind::SET_LIT: {
            if (auto* s = expr.asSet()) {
                for (auto& elem : s->elems) {
                    if (elem) walk(elem);
                }
            }
            break;
        }

        case UnifiedExpr::Kind::TUPLE_LIT: {
            if (auto* t = expr.as<UnifiedExpr::TupleLit>()) {
                for (auto& elem : t->elems) {
                    if (elem) walk(elem);
                }
            }
            break;
        }

        case UnifiedExpr::Kind::RECORD_LIT: {
            if (auto* r = expr.as<UnifiedExpr::RecordLit>()) {
                for (auto& [_, val] : r->fields) {
                    if (val) walk(val);
                }
            }
            break;
        }

        case UnifiedExpr::Kind::LET: {
            if (auto* let = expr.asLet()) {
                for (auto& local : let->locals) {
                    if (local.init) walk(local.init);
                    for (auto& ann : local.anns) {
                        if (ann) walk(ann);
                    }
                }
                if (let->body) walk(let->body);
            }
            break;
        }

        case UnifiedExpr::Kind::ITE: {
            if (auto* ite = expr.asIte()) {
                if (ite->cond) walk(ite->cond);
                if (ite->then_expr) walk(ite->then_expr);
                if (ite->else_expr) walk(ite->else_expr);
            }
            break;
        }

        case UnifiedExpr::Kind::FORALL:
        case UnifiedExpr::Kind::EXISTS: {
            if (auto* q = expr.asQuant()) {
                for (auto& [_, set_expr] : q->generators) {
                    if (set_expr) walk(set_expr);
                }
                if (q->body) walk(q->body);
            }
            break;
        }
    }
}

void UnifiedVisitor::visitChildren(const UnifiedExpr& expr) const {
    // const version delegates to mutable via const_cast trick
    // Since this only reads, it's safe
    auto* self = const_cast<UnifiedVisitor*>(this);
    self->visitChildren(const_cast<UnifiedExpr&>(expr));
}

void UnifiedVisitor::walk(ExprPtr root) {
    if (!root) return;
    if (!visited_.insert(root.get()).second) return; // already visited
    visit(*root);
    visitChildren(*root);
}

void UnifiedVisitor::walk(const ExprPtr& root) const {
    if (!root) return;
    const_cast<UnifiedVisitor*>(this)->visit(const_cast<UnifiedExpr&>(*root));
    // const walk: visit children without tracking visited set
    // (pre-order read-only traversal)
    switch (root->kind) {
        case UnifiedExpr::Kind::LITERAL:
        case UnifiedExpr::Kind::IDENT:
            break;
        case UnifiedExpr::Kind::OP: {
            if (auto* op = root->asOp()) {
                for (const auto& arg : op->args) walk(arg);
            }
            break;
        }
        case UnifiedExpr::Kind::ARRAY_LIT: {
            if (auto* arr = root->asArray()) {
                for (const auto& e : arr->elems) walk(e);
            }
            break;
        }
        case UnifiedExpr::Kind::SET_LIT: {
            if (auto* s = root->asSet()) {
                for (const auto& e : s->elems) walk(e);
            }
            break;
        }
        case UnifiedExpr::Kind::TUPLE_LIT: {
            if (auto* t = root->as<UnifiedExpr::TupleLit>()) {
                for (const auto& e : t->elems) walk(e);
            }
            break;
        }
        case UnifiedExpr::Kind::RECORD_LIT: {
            if (auto* r = root->as<UnifiedExpr::RecordLit>()) {
                for (const auto& [_, val] : r->fields) walk(val);
            }
            break;
        }
        case UnifiedExpr::Kind::LET: {
            if (auto* let = root->asLet()) {
                for (const auto& local : let->locals) {
                    if (local.init) walk(local.init);
                    for (const auto& ann : local.anns) if (ann) walk(ann);
                }
                if (let->body) walk(let->body);
            }
            break;
        }
        case UnifiedExpr::Kind::ITE: {
            if (auto* ite = root->asIte()) {
                if (ite->cond) walk(ite->cond);
                if (ite->then_expr) walk(ite->then_expr);
                if (ite->else_expr) walk(ite->else_expr);
            }
            break;
        }
        case UnifiedExpr::Kind::FORALL:
        case UnifiedExpr::Kind::EXISTS: {
            if (auto* q = root->asQuant()) {
                for (const auto& [_, set_expr] : q->generators) {
                    if (set_expr) walk(set_expr);
                }
                if (q->body) walk(q->body);
            }
            break;
        }
    }
}

void UnifiedVisitor::walkModel(UnifiedModel& model) {
    for (auto& var : model.vars) {
        if (var.init) walk(var.init);
        for (auto& ann : var.anns) if (ann) walk(ann);
    }
    for (auto& c : model.constraints) {
        if (c.expr) walk(c.expr);
        for (auto& ann : c.anns) if (ann) walk(ann);
    }
    for (auto& o : model.objectives) {
        if (o.expr) walk(o.expr);
        for (auto& ann : o.anns) if (ann) walk(ann);
    }
    for (auto& out : model.outputs) {
        if (out) walk(out);
    }
}

void UnifiedVisitor::walkModel(const UnifiedModel& model) const {
    for (const auto& var : model.vars) {
        if (var.init) walk(var.init);
        for (const auto& ann : var.anns) if (ann) walk(ann);
    }
    for (const auto& c : model.constraints) {
        if (c.expr) walk(c.expr);
        for (const auto& ann : c.anns) if (ann) walk(ann);
    }
    for (const auto& o : model.objectives) {
        if (o.expr) walk(o.expr);
        for (const auto& ann : o.anns) if (ann) walk(ann);
    }
    for (const auto& out : model.outputs) {
        if (out) walk(out);
    }
}

} // namespace SOMTParser::Unified
