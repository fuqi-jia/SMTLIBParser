/* -*- C++ -*-
 *
 * MiniZinc Frontend — SMT/OMT Lowering Backend Implementation
 */

#include "somtparser/frontends/minizinc/mzn_lower_smt.h"
#include "somtparser/frontends/minizinc/mzn_symbol_table.h"
#include "somtparser/frontends/minizinc/mzn_evaluator.h"
#include "somtparser/frontends/minizinc/mzn_globals.h"
#include "somtparser/frontends/minizinc/mzn_builtins.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/frontend/parser_context.h"
#include "somtparser/ir/sort.h"

namespace SOMTParser::MiniZinc {

// ── Constructor ──────────────────────────────────────────────────
SmtLoweringBackend::SmtLoweringBackend(Parser& parser,
                                       MznSymbolTable& sym_table,
                                       MznEvaluator& evaluator)
    : parser_(parser), sym_table_(sym_table), evaluator_(evaluator),
      globals_(std::make_unique<GlobalConstraintDecomposer>(*this)) {}

// ── Type lowering ────────────────────────────────────────────────
std::shared_ptr<Sort> SmtLoweringBackend::lowerType(const TypeInst& ti) {
    switch (ti.base) {
        case TypeInst::BaseKind::BOOL:
            return SortManager::getBool();
        case TypeInst::BaseKind::INT:
            return SortManager::getInt();
        case TypeInst::BaseKind::FLOAT:
            return SortManager::getReal();
        case TypeInst::BaseKind::STRING:
            return SortManager::getStr();
        case TypeInst::BaseKind::UNKNOWN: {
            // Array type
            if (ti.elem_type) {
                auto elem = lowerType(*ti.elem_type);
                auto idx = SortManager::getInt(); // MiniZinc arrays are 1-based int-indexed
                return parser_.getSortManager()->createArraySort(idx, elem);
            }
            return SortManager::getUnknown();
        }
        default:
            return SortManager::getUnknown();
    }
}

// ── Expression lowering ──────────────────────────────────────────
std::shared_ptr<DAGNode> SmtLoweringBackend::lowerExpr(const ExprPtr& expr) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case Expr::Kind::BOOL_LIT: {
            auto* lit = expr->as<BoolLit>();
            return lit->value ? parser_.mkTrue() : parser_.mkFalse();
        }
        case Expr::Kind::INT_LIT: {
            auto* lit = expr->as<IntLit>();
            return parser_.mkConstInt(lit->value);
        }
        case Expr::Kind::FLOAT_LIT: {
            auto* lit = expr->as<FloatLit>();
            return parser_.mkConstReal(lit->value);
        }
        case Expr::Kind::STRING_LIT: {
            auto* lit = expr->as<StringLit>();
            return parser_.mkConstStr(lit->value);
        }
        case Expr::Kind::IDENT: {
            auto* id = expr->as<Ident>();
            auto it = var_map_.find(id->name);
            if (it != var_map_.end()) return it->second;
            // Try evaluator for par values
            auto ev = evaluator_.evaluate(expr);
            if (ev.success) return lowerExpr(ev.value);
            addError("Undefined variable in lowering: " + id->name);
            return nullptr;
        }
        case Expr::Kind::UNARY_OP:
            return lowerUnaryOp(*expr->as<UnaryOp>());
        case Expr::Kind::BINARY_OP:
            return lowerBoolOp(*expr->as<BinaryOp>());
        case Expr::Kind::CALL:
            return lowerCall(*expr->as<CallExpr>());
        case Expr::Kind::ARRAY_ACCESS:
            return lowerArrayAccess(*expr->as<ArrayAccess>());
        case Expr::Kind::IF_THEN_ELSE:
            return lowerIfThenElse(*expr->as<IfThenElse>());
        case Expr::Kind::LET:
            return lowerLet(*expr->as<LetExpr>());
        case Expr::Kind::ARRAY_LIT:
            return lowerArrayLit(*expr->as<ArrayLit>());
        case Expr::Kind::SET_LIT:
            return lowerSetLit(*expr->as<SetLit>());
        case Expr::Kind::ANNOTATED:
            return lowerAnnotated(*expr->as<Annotated>());
        default:
            addError("Unsupported expression kind in SMT lowering");
            return nullptr;
    }
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerBoolOp(const BinaryOp& op) {
    auto l = lowerExpr(op.left);
    auto r = lowerExpr(op.right);
    if (!l || !r) return nullptr;

    switch (op.op) {
        case BinaryOp::Op::AND:  return parser_.mkAnd(l, r);
        case BinaryOp::Op::OR:   return parser_.mkOr(l, r);
        case BinaryOp::Op::IMPLIES: return parser_.mkImplies(l, r);
        case BinaryOp::Op::IMPLIED_BY: return parser_.mkImplies(r, l);
        case BinaryOp::Op::IFF:  return parser_.mkEq(l, r);
        case BinaryOp::Op::XOR:  return parser_.mkXor(l, r);
        case BinaryOp::Op::EQ:   return parser_.mkEq(l, r);
        case BinaryOp::Op::NEQ:  return parser_.mkNot(parser_.mkEq(l, r));
        case BinaryOp::Op::LT:   return parser_.mkLt(l, r);
        case BinaryOp::Op::LE:   return parser_.mkLe(l, r);
        case BinaryOp::Op::GT:   return parser_.mkGt(l, r);
        case BinaryOp::Op::GE:   return parser_.mkGe(l, r);
        case BinaryOp::Op::ADD:  return parser_.mkAdd(l, r);
        case BinaryOp::Op::SUB:  return parser_.mkSub(l, r);
        case BinaryOp::Op::MUL:  return parser_.mkMul(l, r);
        case BinaryOp::Op::DIV:  return parser_.mkDivReal(l, r);
        case BinaryOp::Op::DIV_INT: return parser_.mkDivInt(l, r);
        case BinaryOp::Op::MOD:  return parser_.mkMod(l, r);
        case BinaryOp::Op::POW:  return parser_.mkPow(l, r);
        case BinaryOp::Op::UNION:
        case BinaryOp::Op::DIFF:
        case BinaryOp::Op::SYMDIFF:
        case BinaryOp::Op::INTERSECT:
        case BinaryOp::Op::IN:
        case BinaryOp::Op::SUBSET:
        case BinaryOp::Op::SUPERSET:
            return lowerSetOp(op);
        case BinaryOp::Op::RANGE:
        case BinaryOp::Op::RANGE_HALF_OPEN_L:
        case BinaryOp::Op::RANGE_HALF_OPEN_R:
        case BinaryOp::Op::RANGE_OPEN:
            // Range creates a set; for SMT we represent sets differently
            addError("Range expression lowering not yet implemented");
            return nullptr;
        case BinaryOp::Op::CONCAT:
            addError("Concat expression lowering not yet implemented");
            return nullptr;
        default:
            addError("Unknown binary operator in lowering");
            return nullptr;
    }
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerArithOp(const BinaryOp& op) {
    // Merged into lowerBoolOp since arithmetic uses the same dispatch
    return lowerBoolOp(op);
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerCompOp(const BinaryOp& op) {
    return lowerBoolOp(op);
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerSetOp(const BinaryOp& op) {
    (void)op;
    addError("Set operation lowering not yet implemented");
    return nullptr;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerUnaryOp(const UnaryOp& op) {
    auto operand = lowerExpr(op.operand);
    if (!operand) return nullptr;
    switch (op.op) {
        case UnaryOp::Op::NOT: return parser_.mkNot(operand);
        case UnaryOp::Op::PLUS: return operand;
        case UnaryOp::Op::MINUS: return parser_.mkNeg(operand);
        default:
            addError("Unknown unary operator in lowering");
            return nullptr;
    }
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerCall(const CallExpr& call) {
    // Global constraints
    if (MznBuiltins::get().isGlobalConstraint(call.name)) {
        auto decomposed = decomposeGlobal(call.name, call.args);
        if (decomposed.empty()) {
            return parser_.mkTrue();
        }
        if (decomposed.size() == 1) return decomposed[0];
        return parser_.mkAnd(decomposed);
    }

    // Aggregation: forall / exists (comprehension-style)
    if (call.name == "forall" || call.name == "exists") {
        // TODO: generator expansion
        addError("Comprehension-style forall/exists lowering not yet implemented");
        return nullptr;
    }

    // Built-in functions
    if (call.args.empty()) {
        addError("Call with no arguments: " + call.name);
        return nullptr;
    }

    if (call.name == "abs") {
        auto arg = lowerExpr(call.args[0]);
        return arg ? parser_.mkAbs(arg) : nullptr;
    }
    if (call.name == "sum") {
        std::vector<std::shared_ptr<DAGNode>> terms;
        for (auto& a : call.args) {
            auto t = lowerExpr(a);
            if (t) terms.push_back(t);
        }
        if (terms.empty()) return parser_.mkConstInt(0);
        if (terms.size() == 1) return terms[0];
        return parser_.mkAdd(terms);
    }
    if (call.name == "min") {
        auto a = lowerExpr(call.args[0]);
        auto b = lowerExpr(call.args[1]);
        return (a && b) ? parser_.mkMin(std::vector<NodePtr>{a, b}) : nullptr;
    }
    if (call.name == "max") {
        auto a = lowerExpr(call.args[0]);
        auto b = lowerExpr(call.args[1]);
        return (a && b) ? parser_.mkMax(std::vector<NodePtr>{a, b}) : nullptr;
    }
    if (call.name == "bool2int") {
        auto a = lowerExpr(call.args[0]);
        if (!a) return nullptr;
        return parser_.mkIte(a, parser_.mkConstInt(1), parser_.mkConstInt(0));
    }
    if (call.name == "int2float") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkToReal(a) : nullptr;
    }
    if (call.name == "ceil") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkCeil(a) : nullptr;
    }
    if (call.name == "floor") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkFloor(a) : nullptr;
    }
    if (call.name == "round") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkRound(a) : nullptr;
    }

    // Transcendentals
    if (call.name == "sqrt") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkSqrt(a) : nullptr;
    }
    if (call.name == "exp") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkOper(SortManager::getReal(), NODE_KIND::NT_EXP, a) : nullptr;
    }
    if (call.name == "ln") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkOper(SortManager::getReal(), NODE_KIND::NT_LN, a) : nullptr;
    }
    if (call.name == "sin") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkOper(SortManager::getReal(), NODE_KIND::NT_SIN, a) : nullptr;
    }
    if (call.name == "cos") {
        auto a = lowerExpr(call.args[0]);
        return a ? parser_.mkOper(SortManager::getReal(), NODE_KIND::NT_COS, a) : nullptr;
    }

    addError("Unsupported function call in lowering: " + call.name);
    return nullptr;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerArrayAccess(const ArrayAccess& acc) {
    auto arr = lowerExpr(acc.array);
    if (!arr) return nullptr;
    if (acc.indices.size() == 1) {
        auto idx = lowerExpr(acc.indices[0]);
        if (!idx) return nullptr;
        // MiniZinc arrays are 1-based; adjust to 0-based for SMT
        auto one = parser_.mkConstInt(1);
        auto adjusted = parser_.mkSub(idx, one);
        return parser_.mkSelect(arr, adjusted);
    }
    // Multi-dimensional: flatten
    addError("Multi-dimensional array access lowering not yet implemented");
    return nullptr;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerIfThenElse(const IfThenElse& ite) {
    auto else_br = lowerExpr(ite.else_branch);
    for (auto it = ite.branches.rbegin(); it != ite.branches.rend(); ++it) {
        auto cond = lowerExpr(it->first);
        auto then_br = lowerExpr(it->second);
        if (!cond || !then_br || !else_br) return nullptr;
        else_br = parser_.mkIte(cond, then_br, else_br);
    }
    return else_br;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerLet(const LetExpr& let) {
    // TODO: implement let binding lowering with NT_LET
    (void)let;
    addError("Let expression lowering not yet implemented");
    return nullptr;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerArrayLit(const ArrayLit& arr) {
    if (arr.elements.empty()) {
        addError("Empty array literal lowering not supported");
        return nullptr;
    }
    auto first = lowerExpr(arr.elements[0]);
    if (!first) return nullptr;
    auto sort = first->getSort();
    auto arr_sort = parser_.getSortManager()->createArraySort(SortManager::getInt(), sort);
    auto result = parser_.mkConstArray(arr_sort, first);
    for (size_t i = 1; i < arr.elements.size(); ++i) {
        auto elem = lowerExpr(arr.elements[i]);
        if (!elem) return nullptr;
        auto idx = parser_.mkConstInt(static_cast<int>(i));
        result = parser_.mkStore(result, idx, elem);
    }
    return result;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerSetLit(const SetLit& set) {
    (void)set;
    addError("Set literal lowering not yet implemented");
    return nullptr;
}

std::shared_ptr<DAGNode> SmtLoweringBackend::lowerAnnotated(const Annotated& ann) {
    // Annotations are ignored in SMT lowering (stored elsewhere)
    return lowerExpr(ann.expr);
}

// ── Variable declaration lowering ────────────────────────────────
std::shared_ptr<DAGNode> SmtLoweringBackend::lowerVarDecl(
    const VarDeclItem& decl,
    std::vector<std::shared_ptr<DAGNode>>& out_assertions) {

    auto sort = lowerType(*decl.type);
    auto var = parser_.mkVar(sort, decl.name);
    var_map_[decl.name] = var;

    // Domain assertions
    if (decl.type->domain_expr) {
        auto dom = lowerDomain(decl.name, decl.type->domain_expr);
        out_assertions.insert(out_assertions.end(), dom.begin(), dom.end());
    }

    // Initializer
    if (decl.init) {
        auto init = lowerExpr(decl.init);
        if (init) {
            out_assertions.push_back(parser_.mkEq(var, init));
        }
    }

    return var;
}

std::vector<std::shared_ptr<DAGNode>> SmtLoweringBackend::lowerDomain(
    const std::string& var_name,
    const ExprPtr& domain_expr) {
    std::vector<std::shared_ptr<DAGNode>> result;
    auto var = lookupVar(var_name);
    if (!var) return result;

    auto* range = domain_expr->as<BinaryOp>();
    if (range && range->op == BinaryOp::Op::RANGE) {
        auto lo = lowerExpr(range->left);
        auto hi = lowerExpr(range->right);
        if (lo && hi) {
            result.push_back(parser_.mkGe(var, lo));
            result.push_back(parser_.mkLe(var, hi));
        }
        return result;
    }

    auto* set = domain_expr->as<SetLit>();
    if (set) {
        // var in {v1, v2, ...}  ->  var=v1 \/ var=v2 \/ ...
        std::vector<std::shared_ptr<DAGNode>> disjuncts;
        for (auto& e : set->elements) {
            auto val = lowerExpr(e);
            if (val) disjuncts.push_back(parser_.mkEq(var, val));
        }
        if (!disjuncts.empty()) {
            result.push_back(parser_.mkOr(disjuncts));
        }
        return result;
    }

    return result;
}

// ── Constraint lowering ──────────────────────────────────────────
std::shared_ptr<DAGNode> SmtLoweringBackend::lowerConstraint(const ConstraintItem& ci) {
    return lowerExpr(ci.expr);
}

// ── Solve item lowering ──────────────────────────────────────────
void SmtLoweringBackend::lowerSolveItem(const SolveItem& si) {
    if (si.mode == SolveItem::Mode::SATISFY) {
        // Nothing to register for satisfy
        return;
    }
    if (!si.objective) return;

    auto obj_term = lowerExpr(si.objective);
    if (!obj_term) {
        addError("Failed to lower objective expression");
        return;
    }

    OPT_KIND opt_kind = (si.mode == SolveItem::Mode::MINIMIZE)
                        ? OPT_KIND::OPT_MINIMIZE : OPT_KIND::OPT_MAXIMIZE;
    auto obj_mgr = parser_.getObjectiveManager();
    if (obj_mgr) {
        obj_mgr->addSingleObjective(opt_kind, obj_term, COMP_KIND::COMP_LE,
                                    parser_.mkConstInt(0),
                                    parser_.mkConstInt(0), "");
    }
}

// ── Output item lowering ─────────────────────────────────────────
void SmtLoweringBackend::lowerOutputItem(const OutputItem& oi) {
    (void)oi;
    // Output items are collected but do not affect SMT assertions
    // TODO: store in ParserContext::output_items
}

// ── Global constraint decomposition ──────────────────────────────
std::vector<std::shared_ptr<DAGNode>> SmtLoweringBackend::decomposeGlobal(
    const std::string& name,
    const std::vector<ExprPtr>& args) {
    return globals_->decompose(name, args);
}

// ── Finalization ─────────────────────────────────────────────────
void SmtLoweringBackend::finalize() {
    // All assertions and objectives have been added to ctx_ during lowering
}

// ── Internal accessors ───────────────────────────────────────────
std::shared_ptr<DAGNode> SmtLoweringBackend::lookupVar(const std::string& name) const {
    auto it = var_map_.find(name);
    return (it != var_map_.end()) ? it->second : nullptr;
}

std::shared_ptr<Sort> SmtLoweringBackend::lookupEnumSort(const std::string& name) const {
    auto it = enum_sort_map_.find(name);
    return (it != enum_sort_map_.end()) ? it->second : nullptr;
}

void SmtLoweringBackend::addError(const std::string& msg) {
    errors_.push_back(msg);
}

} // namespace SOMTParser::MiniZinc
