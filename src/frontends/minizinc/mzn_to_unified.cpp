/* -*- C++ -*-
 *
 * MznAstToUnifiedIR implementation
 */

#include "somtparser/frontends/minizinc/mzn_to_unified.h"

#include <iostream>

namespace SOMTParser::MiniZinc {

namespace U = SOMTParser::Unified;

using UExprPtr = U::ExprPtr;
using UOpRef = U::UnifiedOpRef;
using UModel = U::UnifiedModel;
using UVarDecl = U::UnifiedVarDecl;
using UType = U::UnifiedType;
using USort = U::UnifiedSort;
using ULit = U::UnifiedExpr::Literal;
using UIdent = U::UnifiedExpr::Ident;
using UOpNode = U::UnifiedExpr::OpNode;
using UArrayLit = U::UnifiedExpr::ArrayLit;
using USetLit = U::UnifiedExpr::SetLit;
using ULetExpr = U::UnifiedExpr::LetExpr;
using UIteExpr = U::UnifiedExpr::IteExpr;
using UQuantExpr = U::UnifiedExpr::QuantExpr;
using UConstraint = U::UnifiedConstraint;
using UObjective = U::UnifiedObjective;

// ── Helpers ────────────────────────────────────────────────────────

UOpRef MznAstToUnifiedIR::lookupOpByMznName(const std::string& name) const {
    return registry_.lookupByLangName("minizinc", name);
}

std::string MznAstToUnifiedIR::binaryOpToMznName(BinaryOp::Op op) const {
    switch (op) {
        case BinaryOp::Op::AND:   return "/\\";
        case BinaryOp::Op::OR:    return "\\/";
        case BinaryOp::Op::IMPLIES: return "->";
        case BinaryOp::Op::IFF:   return "<->";
        case BinaryOp::Op::XOR:   return "xor";
        case BinaryOp::Op::EQ:    return "=";
        case BinaryOp::Op::NEQ:   return "!=";
        case BinaryOp::Op::LT:    return "<";
        case BinaryOp::Op::LE:    return "<=";
        case BinaryOp::Op::GT:    return ">";
        case BinaryOp::Op::GE:    return ">=";
        case BinaryOp::Op::IN:    return "in";
        case BinaryOp::Op::UNION: return "union";
        case BinaryOp::Op::ADD:   return "+";
        case BinaryOp::Op::SUB:   return "-";
        case BinaryOp::Op::MUL:   return "*";
        case BinaryOp::Op::DIV:   return "/";
        case BinaryOp::Op::DIV_INT: return "div";
        case BinaryOp::Op::MOD:   return "mod";
        case BinaryOp::Op::POW:   return "pow";
        default: return "unknown";
    }
}

std::string MznAstToUnifiedIR::unaryOpToMznName(UnaryOp::Op op) const {
    switch (op) {
        case UnaryOp::Op::NOT:   return "not";
        case UnaryOp::Op::PLUS:  return "+";
        case UnaryOp::Op::MINUS: return "-";
        default: return "unknown";
    }
}

// ── Type converter ─────────────────────────────────────────────────

UType MznAstToUnifiedIR::convertType(const std::shared_ptr<TypeInst>& type) const {
    if (!type) return UType{};

    UType ut;
    ut.par_var = (type->par_var == TypeInst::ParVar::VAR) ? UType::ParVar::VAR : UType::ParVar::PAR;
    ut.is_optional = type->is_opt;

    switch (type->base) {
        case TypeInst::BaseKind::BOOL:   ut.sort = USort::mkBool(); break;
        case TypeInst::BaseKind::INT:    ut.sort = USort::mkInt(); break;
        case TypeInst::BaseKind::FLOAT:  ut.sort = USort::mkFloat(); break;
        case TypeInst::BaseKind::STRING: ut.sort = USort::mkString(); break;
        case TypeInst::BaseKind::ANY:
        case TypeInst::BaseKind::TOP:
        default:
            ut.sort = USort::mkAny();
            break;
    }

    if (type->is_set) {
        USort elem = USort::mkInt();
        if (type->elem_type) {
            auto eut = convertType(type->elem_type);
            elem = eut.sort;
        }
        ut.sort = USort::mkSet(elem);
    }

    for (auto& dim : type->array_dims) {
        ut.array_dims.push_back(convertExpr(dim));
    }

    if (type->domain_expr) {
        ut.domain = convertExpr(type->domain_expr);
    }

    return ut;
}

// ── Expression converters ──────────────────────────────────────────

UExprPtr MznAstToUnifiedIR::convertLiteral(const ExprPtr& expr) const {
    if (auto* lit = expr->as<BoolLit>()) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        node->data = ULit::mkBool(lit->value);
        return node;
    }
    if (auto* ilit = expr->as<IntLit>()) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        node->data = ULit::mkInt(ilit->value);
        return node;
    }
    if (auto* flit = expr->as<FloatLit>()) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        node->data = ULit::mkFloat(flit->value);
        return node;
    }
    if (auto* slit = expr->as<StringLit>()) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        node->data = ULit::mkString(slit->value);
        return node;
    }
    return nullptr;
}

UExprPtr MznAstToUnifiedIR::convertIdent(const ExprPtr& expr) const {
    auto* id = expr->as<Ident>();
    if (!id) return nullptr;
    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::IDENT, U::SourceLoc{});
    node->data = UIdent{id->name};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertUnaryOp(const ExprPtr& expr) const {
    auto* uop = expr->as<UnaryOp>();
    if (!uop) return nullptr;

    std::string mzn_name = unaryOpToMznName(uop->op);
    auto ref = lookupOpByMznName(mzn_name);
    if (!ref.valid()) {
        std::cerr << "[MznToUnified] Warning: unary op '" << mzn_name << "' not found in registry\n";
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = UOpNode{ref, {convertExpr(uop->operand)}, {}};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertBinaryOp(const ExprPtr& expr) const {
    auto* bop = expr->as<BinaryOp>();
    if (!bop) return nullptr;

    std::string mzn_name = binaryOpToMznName(bop->op);
    auto ref = lookupOpByMznName(mzn_name);
    if (!ref.valid()) {
        std::cerr << "[MznToUnified] Warning: binary op '" << mzn_name << "' not found in registry\n";
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = UOpNode{ref, {convertExpr(bop->left), convertExpr(bop->right)}, {}};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertCall(const ExprPtr& expr) const {
    auto* call = expr->as<CallExpr>();
    if (!call) return nullptr;

    if (call->name == "forall" || call->name == "exists") {
        auto kind = (call->name == "forall") ? U::UnifiedExpr::Kind::FORALL : U::UnifiedExpr::Kind::EXISTS;
        auto node = std::make_shared<U::UnifiedExpr>(kind, U::SourceLoc{});

        std::vector<std::pair<std::string, UExprPtr>> generators;

        // Try structured generators first
        for (auto& gen : call->generators) {
            UExprPtr set_expr = nullptr;
            if (gen.set_expr) set_expr = convertExpr(gen.set_expr);
            for (auto& var : gen.vars) {
                generators.emplace_back(var, set_expr);
            }
        }

        UExprPtr body = nullptr;
        size_t body_idx = 0;

        if (call->is_comprehension_call && !call->args.empty()) {
            // Comprehension-style: last arg is body, preceding args are generators
            body_idx = call->args.size() - 1;
            body = convertExpr(call->args[body_idx]);
            // Extract generators from preceding args (each should be "var in set")
            for (size_t i = 0; i < body_idx; ++i) {
                auto* bin = call->args[i]->as<BinaryOp>();
                if (bin && bin->op == BinaryOp::Op::IN) {
                    auto* id = bin->left->as<Ident>();
                    if (id) {
                        generators.emplace_back(id->name, convertExpr(bin->right));
                    }
                }
            }
        } else if (!call->args.empty()) {
            body = convertExpr(call->args[0]);
        }

        node->data = UQuantExpr{std::move(generators), body};
        return node;
    }

    auto ref = lookupOpByMznName(call->name);
    if (!ref.valid()) {
        std::cerr << "[MznToUnified] Warning: call '" << call->name << "' not found in registry\n";
    }

    std::vector<UExprPtr> args;
    args.reserve(call->args.size());
    for (auto& arg : call->args) {
        args.push_back(convertExpr(arg));
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = UOpNode{ref, std::move(args), {}};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertArrayAccess(const ExprPtr& expr) const {
    auto* aa = expr->as<ArrayAccess>();
    if (!aa) return nullptr;

    auto ref = lookupOpByMznName("[]");
    if (!ref.valid()) {
        std::cerr << "[MznToUnified] Warning: array access '[]' not found in registry\n";
    }

    std::vector<UExprPtr> args;
    args.push_back(convertExpr(aa->array));
    for (auto& idx : aa->indices) {
        args.push_back(convertExpr(idx));
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = UOpNode{ref, std::move(args), {}};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertArrayLit(const ExprPtr& expr) const {
    auto* arr = expr->as<ArrayLit>();
    if (!arr) return nullptr;
    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::ARRAY_LIT, U::SourceLoc{});
    std::vector<UExprPtr> elems;
    elems.reserve(arr->elements.size());
    for (auto& e : arr->elements) {
        elems.push_back(convertExpr(e));
    }
    node->data = UArrayLit{std::move(elems)};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertSetLit(const ExprPtr& expr) const {
    auto* s = expr->as<SetLit>();
    if (!s) return nullptr;
    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::SET_LIT, U::SourceLoc{});
    std::vector<UExprPtr> elems;
    elems.reserve(s->elements.size());
    for (auto& e : s->elements) {
        elems.push_back(convertExpr(e));
    }
    node->data = USetLit{std::move(elems)};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertIfThenElse(const ExprPtr& expr) const {
    auto* ite = expr->as<IfThenElse>();
    if (!ite || ite->branches.empty()) return nullptr;

    UExprPtr cond = convertExpr(ite->branches[0].first);
    UExprPtr then_expr = convertExpr(ite->branches[0].second);
    UExprPtr else_expr = nullptr;
    if (ite->else_branch) {
        else_expr = convertExpr(ite->else_branch);
    } else {
        auto false_lit = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        false_lit->data = ULit::mkBool(false);
        else_expr = false_lit;
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::ITE, U::SourceLoc{});
    node->data = UIteExpr{cond, then_expr, else_expr};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertLet(const ExprPtr& expr) const {
    auto* let = expr->as<LetExpr>();
    if (!let) return nullptr;

    std::vector<UVarDecl> locals;
    for (auto& item : let->items) {
        if (auto* vd = dynamic_cast<VarDeclItem*>(item.get())) {
            UVarDecl uvd;
            uvd.name = vd->name;
            uvd.type = convertType(vd->type);
            if (vd->init) uvd.init = convertExpr(vd->init);
            for (auto& ann : vd->anns) {
                uvd.anns.push_back(convertExpr(ann));
            }
            locals.push_back(std::move(uvd));
        }
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LET, U::SourceLoc{});
    node->data = ULetExpr{std::move(locals), convertExpr(let->body)};
    return node;
}

UExprPtr MznAstToUnifiedIR::convertAnnotated(const ExprPtr& expr) const {
    auto* ann = expr->as<Annotated>();
    if (!ann) return nullptr;
    return convertExpr(ann->expr);
}

// ── Main expression dispatcher ─────────────────────────────────────

UExprPtr MznAstToUnifiedIR::convertExpr(const ExprPtr& expr) const {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case Expr::Kind::BOOL_LIT:
        case Expr::Kind::INT_LIT:
        case Expr::Kind::FLOAT_LIT:
        case Expr::Kind::STRING_LIT:
            return convertLiteral(expr);

        case Expr::Kind::IDENT:
            return convertIdent(expr);

        case Expr::Kind::UNARY_OP:
            return convertUnaryOp(expr);

        case Expr::Kind::BINARY_OP:
            return convertBinaryOp(expr);

        case Expr::Kind::CALL:
            return convertCall(expr);

        case Expr::Kind::ARRAY_ACCESS:
            return convertArrayAccess(expr);

        case Expr::Kind::ARRAY_LIT:
            return convertArrayLit(expr);

        case Expr::Kind::SET_LIT:
            return convertSetLit(expr);

        case Expr::Kind::IF_THEN_ELSE:
            return convertIfThenElse(expr);

        case Expr::Kind::LET:
            return convertLet(expr);

        case Expr::Kind::ANNOTATED:
            return convertAnnotated(expr);

        default:
            std::cerr << "[MznToUnified] Warning: unhandled expr kind "
                      << static_cast<int>(expr->kind) << "\n";
            return nullptr;
    }
}

// ── Model converter ────────────────────────────────────────────────

UModel MznAstToUnifiedIR::convert(const Model& mzn_model) const {
    UModel unified;

    for (auto& item : mzn_model.items) {
        if (!item) continue;

        switch (item->kind) {
            case Item::Kind::VAR_DECL: {
                auto* vd = dynamic_cast<VarDeclItem*>(item.get());
                if (!vd) break;
                UVarDecl uvd;
                uvd.name = vd->name;
                uvd.type = convertType(vd->type);
                if (vd->init) uvd.init = convertExpr(vd->init);
                for (auto& ann : vd->anns) {
                    uvd.anns.push_back(convertExpr(ann));
                }
                if (uvd.type.isPar()) {
                    unified.parameters.push_back(std::move(uvd));
                } else {
                    unified.vars.push_back(std::move(uvd));
                }
                break;
            }

            case Item::Kind::CONSTRAINT: {
                auto* ci = dynamic_cast<ConstraintItem*>(item.get());
                if (!ci || !ci->expr) break;
                UConstraint uc(convertExpr(ci->expr));
                for (auto& ann : ci->anns) {
                    uc.anns.push_back(convertExpr(ann));
                }
                unified.constraints.push_back(std::move(uc));
                break;
            }

            case Item::Kind::SOLVE: {
                auto* si = dynamic_cast<SolveItem*>(item.get());
                if (!si) break;
                UObjective::Mode mode;
                switch (si->mode) {
                    case SolveItem::Mode::SATISFY: mode = UObjective::Mode::SATISFY; break;
                    case SolveItem::Mode::MINIMIZE: mode = UObjective::Mode::MINIMIZE; break;
                    case SolveItem::Mode::MAXIMIZE: mode = UObjective::Mode::MAXIMIZE; break;
                }
                UExprPtr obj_expr = nullptr;
                if (si->objective) obj_expr = convertExpr(si->objective);
                unified.objectives.emplace_back(mode, obj_expr);
                break;
            }

            case Item::Kind::OUTPUT: {
                auto* oi = dynamic_cast<OutputItem*>(item.get());
                if (!oi || !oi->expr) break;
                unified.outputs.push_back(convertExpr(oi->expr));
                break;
            }

            case Item::Kind::ASSIGN: {
                auto* ai = dynamic_cast<AssignItem*>(item.get());
                if (!ai) break;
                auto ref = lookupOpByMznName("=");
                auto lhs = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::IDENT, U::SourceLoc{});
                lhs->data = UIdent{ai->name};
                auto rhs = convertExpr(ai->expr);
                auto eq = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
                eq->data = UOpNode{ref, {lhs, rhs}, {}};
                unified.constraints.emplace_back(eq);
                break;
            }

            default:
                break;
        }
    }

    return unified;
}

} // namespace SOMTParser::MiniZinc
