/* -*- C++ -*-
 *
 * MiniZinc Frontend — Compile-time Evaluator Implementation
 */

#include "somtparser/minizinc/mzn_evaluator.h"
#include "somtparser/minizinc/mzn_symbol_table.h"
#include "somtparser/minizinc/mzn_builtins.h"
#include <algorithm>
#include <iterator>
#include <cmath>

namespace SOMTParser::MiniZinc {

// ── Constructor ──────────────────────────────────────────────────
MznEvaluator::MznEvaluator(MznSymbolTable* sym_table)
    : sym_table(sym_table) {}

// ── Main entry point ─────────────────────────────────────────────
EvalResult MznEvaluator::evaluate(const ExprPtr& expr) {
    if (!expr) return EvalResult::err("Null expression");

    switch (expr->kind) {
        case Expr::Kind::BOOL_LIT:
            return EvalResult::ok(expr);
        case Expr::Kind::INT_LIT:
            return EvalResult::ok(expr);
        case Expr::Kind::FLOAT_LIT:
            return EvalResult::ok(expr);
        case Expr::Kind::STRING_LIT:
            return EvalResult::ok(expr);
        case Expr::Kind::ARRAY_LIT: {
            auto* arr = expr->as<ArrayLit>();
            if (!arr) return EvalResult::err("Invalid array literal");
            return evalArrayLit(*arr);
        }
        case Expr::Kind::SET_LIT: {
            auto* set = expr->as<SetLit>();
            if (!set) return EvalResult::err("Invalid set literal");
            return evalSetLit(*set);
        }
        case Expr::Kind::IDENT: {
            auto* id = expr->as<Ident>();
            if (!id) return EvalResult::err("Invalid identifier");
            auto it = fixed_values.find(id->name);
            if (it != fixed_values.end()) {
                return EvalResult::ok(it->second);
            }
            if (sym_table) {
                auto vd = sym_table->lookupVar(id->name);
                if (vd && vd->init) {
                    return evaluate(vd->init);
                }
            }
            return EvalResult::err("Identifier not fixed: " + id->name);
        }
        case Expr::Kind::UNARY_OP: {
            auto* un = expr->as<UnaryOp>();
            if (!un) return EvalResult::err("Invalid unary op");
            return evalUnaryOp(*un);
        }
        case Expr::Kind::BINARY_OP: {
            auto* bin = expr->as<BinaryOp>();
            if (!bin) return EvalResult::err("Invalid binary op");
            switch (bin->op) {
                case BinaryOp::Op::AND:
                case BinaryOp::Op::OR:
                case BinaryOp::Op::IMPLIES:
                case BinaryOp::Op::IFF:
                case BinaryOp::Op::XOR:
                    return evalBoolOp(*bin);
                case BinaryOp::Op::EQ:
                case BinaryOp::Op::NEQ:
                case BinaryOp::Op::LT:
                case BinaryOp::Op::LE:
                case BinaryOp::Op::GT:
                case BinaryOp::Op::GE:
                    return evalCompOp(*bin); // comparison of constants
                case BinaryOp::Op::ADD:
                case BinaryOp::Op::SUB:
                case BinaryOp::Op::MUL:
                case BinaryOp::Op::DIV:
                case BinaryOp::Op::DIV_INT:
                case BinaryOp::Op::MOD:
                case BinaryOp::Op::POW:
                    return evalArithOp(*bin);
                case BinaryOp::Op::UNION:
                case BinaryOp::Op::DIFF:
                case BinaryOp::Op::SYMDIFF:
                case BinaryOp::Op::INTERSECT:
                    return evalSetOp(*bin);
                default:
                    return EvalResult::err("Unsupported binary operator for evaluation");
            }
        }
        case Expr::Kind::CALL: {
            auto* c = expr->as<CallExpr>();
            if (!c) return EvalResult::err("Invalid call");
            return evalCall(*c);
        }
        case Expr::Kind::IF_THEN_ELSE: {
            auto* ite = expr->as<IfThenElse>();
            if (!ite) return EvalResult::err("Invalid if-then-else");
            return evalIfThenElse(*ite);
        }
        case Expr::Kind::ARRAY_ACCESS: {
            auto* acc = expr->as<ArrayAccess>();
            if (!acc) return EvalResult::err("Invalid array access");
            return evalArrayAccess(*acc);
        }
        default:
            return EvalResult::err("Expression kind not evaluable at compile time");
    }
}

// ── Helpers ──────────────────────────────────────────────────────
bool MznEvaluator::isParExpr(const ExprPtr& expr) const {
    // Simplified: check if expression contains only literals and fixed identifiers
    (void)expr;
    // Cannot call evaluate() from const method; use a different heuristic
    return false;
}

bool MznEvaluator::isFixed(const std::string& name) const {
    return fixed_values.find(name) != fixed_values.end();
}

void MznEvaluator::markFixed(const std::string& name, ExprPtr value) {
    fixed_values[name] = std::move(value);
}

// ── Sub-evaluators ───────────────────────────────────────────────
EvalResult MznEvaluator::evalBoolOp(const BinaryOp& op) {
    auto l = evaluate(op.left);
    auto r = evaluate(op.right);
    if (!l.success) return l;
    if (!r.success) return r;
    auto* lb = l.value->as<BoolLit>();
    auto* rb = r.value->as<BoolLit>();
    if (!lb || !rb) return EvalResult::err("BoolOp on non-bool values");
    bool res = false;
    switch (op.op) {
        case BinaryOp::Op::AND: res = lb->value && rb->value; break;
        case BinaryOp::Op::OR:  res = lb->value || rb->value; break;
        case BinaryOp::Op::IMPLIES: res = !lb->value || rb->value; break;
        case BinaryOp::Op::IFF: res = lb->value == rb->value; break;
        case BinaryOp::Op::XOR: res = lb->value != rb->value; break;
        default: return EvalResult::err("Unknown bool op");
    }
    auto expr = std::make_shared<Expr>(Expr::Kind::BOOL_LIT, SourceLoc{});
    expr->data = BoolLit{res};
    return EvalResult::ok(expr);
}

EvalResult MznEvaluator::evalArithOp(const BinaryOp& op) {
    auto l = evaluate(op.left);
    auto r = evaluate(op.right);
    if (!l.success) return l;
    if (!r.success) return r;

    auto* li = l.value->as<IntLit>();
    auto* ri = r.value->as<IntLit>();
    if (li && ri) {
        int64_t lv = li->value;
        int64_t rv = ri->value;
        int64_t res = 0;
        switch (op.op) {
            case BinaryOp::Op::ADD: res = lv + rv; break;
            case BinaryOp::Op::SUB: res = lv - rv; break;
            case BinaryOp::Op::MUL: res = lv * rv; break;
            case BinaryOp::Op::DIV_INT:
            case BinaryOp::Op::DIV:
                if (rv == 0) return EvalResult::err("Division by zero");
                res = lv / rv; break;
            case BinaryOp::Op::MOD:
                if (rv == 0) return EvalResult::err("Modulo by zero");
                res = lv % rv; break;
            case BinaryOp::Op::POW: res = static_cast<int64_t>(std::pow(lv, rv)); break;
            default: return EvalResult::err("Unknown arithmetic op");
        }
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{res};
        return EvalResult::ok(expr);
    }

    auto* lf = l.value->as<FloatLit>();
    auto* rf = r.value->as<FloatLit>();
    if ((li || lf) && (ri || rf)) {
        double lv = li ? static_cast<double>(li->value) : lf->value;
        double rv = ri ? static_cast<double>(ri->value) : rf->value;
        double res = 0.0;
        switch (op.op) {
            case BinaryOp::Op::ADD: res = lv + rv; break;
            case BinaryOp::Op::SUB: res = lv - rv; break;
            case BinaryOp::Op::MUL: res = lv * rv; break;
            case BinaryOp::Op::DIV:
                if (rv == 0.0) return EvalResult::err("Division by zero");
                res = lv / rv; break;
            case BinaryOp::Op::POW: res = std::pow(lv, rv); break;
            default: return EvalResult::err("Unknown float arithmetic op");
        }
        auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, SourceLoc{});
        expr->data = FloatLit{res};
        return EvalResult::ok(expr);
    }

    return EvalResult::err("Arithmetic op on incompatible types");
}

EvalResult MznEvaluator::evalCompOp(const BinaryOp& op) {
    auto l = evaluate(op.left);
    auto r = evaluate(op.right);
    if (!l.success) return l;
    if (!r.success) return r;
    // Simplified: only support int/int and bool/bool comparison
    auto* li = l.value->as<IntLit>();
    auto* ri = r.value->as<IntLit>();
    bool res = false;
    if (li && ri) {
        switch (op.op) {
            case BinaryOp::Op::EQ: res = li->value == ri->value; break;
            case BinaryOp::Op::NEQ: res = li->value != ri->value; break;
            case BinaryOp::Op::LT: res = li->value < ri->value; break;
            case BinaryOp::Op::LE: res = li->value <= ri->value; break;
            case BinaryOp::Op::GT: res = li->value > ri->value; break;
            case BinaryOp::Op::GE: res = li->value >= ri->value; break;
            default: return EvalResult::err("Unknown comparison op");
        }
    } else {
        auto* lb = l.value->as<BoolLit>();
        auto* rb = r.value->as<BoolLit>();
        if (!lb || !rb) return EvalResult::err("Comparison on incompatible types");
        switch (op.op) {
            case BinaryOp::Op::EQ: res = lb->value == rb->value; break;
            case BinaryOp::Op::NEQ: res = lb->value != rb->value; break;
            default: return EvalResult::err("Comparison on bools not supported");
        }
    }
    auto expr = std::make_shared<Expr>(Expr::Kind::BOOL_LIT, SourceLoc{});
    expr->data = BoolLit{res};
    return EvalResult::ok(expr);
}

EvalResult MznEvaluator::evalSetOp(const BinaryOp& op) {
    // Set operations on literal sets (evaluate element-wise)
    auto l = evaluate(op.left);
    auto r = evaluate(op.right);
    if (!l.success) return l;
    if (!r.success) return r;
    auto* ls = l.value->as<SetLit>();
    auto* rs = r.value->as<SetLit>();
    if (!ls || !rs) return EvalResult::err("SetOp on non-set values");

    // For simplicity, we represent sets as sorted vectors of int literals
    std::vector<int64_t> a, b, res;
    for (auto& e : ls->elements) {
        auto* lit = e->as<IntLit>();
        if (!lit) return EvalResult::err("Set elements must be int literals");
        a.push_back(lit->value);
    }
    for (auto& e : rs->elements) {
        auto* lit = e->as<IntLit>();
        if (!lit) return EvalResult::err("Set elements must be int literals");
        b.push_back(lit->value);
    }
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());

    switch (op.op) {
        case BinaryOp::Op::UNION: {
            std::set_union(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(res));
            break;
        }
        case BinaryOp::Op::INTERSECT: {
            std::set_intersection(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(res));
            break;
        }
        case BinaryOp::Op::DIFF: {
            std::set_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(res));
            break;
        }
        case BinaryOp::Op::SYMDIFF: {
            std::set_symmetric_difference(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(res));
            break;
        }
        default:
            return EvalResult::err("Unknown set operator");
    }

    SetLit result_set;
    for (auto v : res) {
        auto lit = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        lit->data = IntLit{v};
        result_set.elements.push_back(lit);
    }
    auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, SourceLoc{});
    expr->data = std::move(result_set);
    return EvalResult::ok(expr);
}

EvalResult MznEvaluator::evalUnaryOp(const UnaryOp& op) {
    auto operand = evaluate(op.operand);
    if (!operand.success) return operand;
    switch (op.op) {
        case UnaryOp::Op::NOT: {
            auto* b = operand.value->as<BoolLit>();
            if (!b) return EvalResult::err("not on non-bool");
            auto expr = std::make_shared<Expr>(Expr::Kind::BOOL_LIT, SourceLoc{});
            expr->data = BoolLit{!b->value};
            return EvalResult::ok(expr);
        }
        case UnaryOp::Op::PLUS: {
            return operand; // identity
        }
        case UnaryOp::Op::MINUS: {
            auto* i = operand.value->as<IntLit>();
            if (i) {
                auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
                expr->data = IntLit{-i->value};
                return EvalResult::ok(expr);
            }
            auto* f = operand.value->as<FloatLit>();
            if (f) {
                auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, SourceLoc{});
                expr->data = FloatLit{-f->value};
                return EvalResult::ok(expr);
            }
            return EvalResult::err("negation on non-numeric");
        }
    }
    return EvalResult::err("Unknown unary op");
}

EvalResult MznEvaluator::evalCall(const CallExpr& call) {
    std::vector<ExprPtr> arg_vals;
    for (auto& a : call.args) {
        auto r = evaluate(a);
        if (!r.success) return r;
        arg_vals.push_back(r.value);
    }
    return evalBuiltin(call.name, arg_vals);
}

EvalResult MznEvaluator::evalBuiltin(const std::string& name,
                                     const std::vector<ExprPtr>& args) {
    if (name == "abs") {
        if (args.size() != 1) return EvalResult::err("abs expects 1 argument");
        auto* i = args[0]->as<IntLit>();
        if (i) {
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{std::abs(i->value)};
            return EvalResult::ok(expr);
        }
        auto* f = args[0]->as<FloatLit>();
        if (f) {
            auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, SourceLoc{});
            expr->data = FloatLit{std::abs(f->value)};
            return EvalResult::ok(expr);
        }
        return EvalResult::err("abs on non-numeric");
    }
    if (name == "sum") {
        if (args.size() != 1) return EvalResult::err("sum expects 1 argument");
        auto* arr = args[0]->as<ArrayLit>();
        if (!arr) return EvalResult::err("sum expects array");
        int64_t total = 0;
        for (auto& e : arr->elements) {
            auto* lit = e->as<IntLit>();
            if (!lit) return EvalResult::err("sum array must contain int literals");
            total += lit->value;
        }
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{total};
        return EvalResult::ok(expr);
    }
    if (name == "product") {
        if (args.size() != 1) return EvalResult::err("product expects 1 argument");
        auto* arr = args[0]->as<ArrayLit>();
        if (!arr) return EvalResult::err("product expects array");
        int64_t total = 1;
        for (auto& e : arr->elements) {
            auto* lit = e->as<IntLit>();
            if (!lit) return EvalResult::err("product array must contain int literals");
            total *= lit->value;
        }
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{total};
        return EvalResult::ok(expr);
    }
    if (name == "min") {
        if (args.size() == 1) {
            auto* arr = args[0]->as<ArrayLit>();
            if (!arr || arr->elements.empty()) return EvalResult::err("min expects non-empty array");
            int64_t best = INT64_MAX;
            for (auto& e : arr->elements) {
                auto* lit = e->as<IntLit>();
                if (!lit) return EvalResult::err("min array must contain int literals");
                best = std::min(best, lit->value);
            }
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{best};
            return EvalResult::ok(expr);
        } else if (args.size() == 2) {
            auto* a = args[0]->as<IntLit>();
            auto* b = args[1]->as<IntLit>();
            if (!a || !b) return EvalResult::err("min expects int args");
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{std::min(a->value, b->value)};
            return EvalResult::ok(expr);
        }
        return EvalResult::err("min expects 1 or 2 arguments");
    }
    if (name == "max") {
        if (args.size() == 1) {
            auto* arr = args[0]->as<ArrayLit>();
            if (!arr || arr->elements.empty()) return EvalResult::err("max expects non-empty array");
            int64_t best = INT64_MIN;
            for (auto& e : arr->elements) {
                auto* lit = e->as<IntLit>();
                if (!lit) return EvalResult::err("max array must contain int literals");
                best = std::max(best, lit->value);
            }
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{best};
            return EvalResult::ok(expr);
        } else if (args.size() == 2) {
            auto* a = args[0]->as<IntLit>();
            auto* b = args[1]->as<IntLit>();
            if (!a || !b) return EvalResult::err("max expects int args");
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{std::max(a->value, b->value)};
            return EvalResult::ok(expr);
        }
        return EvalResult::err("max expects 1 or 2 arguments");
    }
    if (name == "bool2int") {
        if (args.size() != 1) return EvalResult::err("bool2int expects 1 argument");
        auto* b = args[0]->as<BoolLit>();
        if (!b) return EvalResult::err("bool2int expects bool");
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{b->value ? 1 : 0};
        return EvalResult::ok(expr);
    }
    if (name == "int2float") {
        if (args.size() != 1) return EvalResult::err("int2float expects 1 argument");
        auto* i = args[0]->as<IntLit>();
        if (!i) return EvalResult::err("int2float expects int");
        auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, SourceLoc{});
        expr->data = FloatLit{static_cast<double>(i->value)};
        return EvalResult::ok(expr);
    }
    if (name == "card" || name == "length") {
        if (args.size() != 1) return EvalResult::err(name + " expects 1 argument");
        auto* set = args[0]->as<SetLit>();
        if (set) {
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{static_cast<int64_t>(set->elements.size())};
            return EvalResult::ok(expr);
        }
        auto* arr = args[0]->as<ArrayLit>();
        if (arr) {
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
            expr->data = IntLit{static_cast<int64_t>(arr->elements.size())};
            return EvalResult::ok(expr);
        }
        return EvalResult::err(name + " expects set or array");
    }
    if (name == "ceil") {
        if (args.size() != 1) return EvalResult::err("ceil expects 1 argument");
        auto* f = args[0]->as<FloatLit>();
        if (!f) return EvalResult::err("ceil expects float");
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{static_cast<int64_t>(std::ceil(f->value))};
        return EvalResult::ok(expr);
    }
    if (name == "floor") {
        if (args.size() != 1) return EvalResult::err("floor expects 1 argument");
        auto* f = args[0]->as<FloatLit>();
        if (!f) return EvalResult::err("floor expects float");
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{static_cast<int64_t>(std::floor(f->value))};
        return EvalResult::ok(expr);
    }
    if (name == "round") {
        if (args.size() != 1) return EvalResult::err("round expects 1 argument");
        auto* f = args[0]->as<FloatLit>();
        if (!f) return EvalResult::err("round expects float");
        auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
        expr->data = IntLit{static_cast<int64_t>(std::round(f->value))};
        return EvalResult::ok(expr);
    }

    return EvalResult::err("Unsupported builtin function: " + name);
}

EvalResult MznEvaluator::evalArrayLit(const ArrayLit& arr) {
    // Array literals are already constant if all elements are constant
    auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, SourceLoc{});
    expr->data = ArrayLit{arr.elements};
    return EvalResult::ok(expr);
}

EvalResult MznEvaluator::evalSetLit(const SetLit& set) {
    auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, SourceLoc{});
    expr->data = SetLit{set.elements};
    return EvalResult::ok(expr);
}

EvalResult MznEvaluator::evalIfThenElse(const IfThenElse& ite) {
    for (auto& branch : ite.branches) {
        auto cond = evaluate(branch.first);
        if (!cond.success) return cond;
        auto* b = cond.value->as<BoolLit>();
        if (!b) return EvalResult::err("if condition must be bool");
        if (b->value) {
            return evaluate(branch.second);
        }
    }
    return evaluate(ite.else_branch);
}

EvalResult MznEvaluator::evalArrayAccess(const ArrayAccess& acc) {
    auto arr = evaluate(acc.array);
    if (!arr.success) return arr;
    auto* lit = arr.value->as<ArrayLit>();
    if (!lit) return EvalResult::err("Array access on non-array");
    if (acc.indices.size() != 1) return EvalResult::err("Multi-dimensional array access not supported in evaluator");
    auto idx = evaluate(acc.indices[0]);
    if (!idx.success) return idx;
    auto* i = idx.value->as<IntLit>();
    if (!i) return EvalResult::err("Array index must be int");
    size_t pos = static_cast<size_t>(i->value - 1); // MiniZinc 1-based
    if (pos >= lit->elements.size()) return EvalResult::err("Array index out of bounds");
    return EvalResult::ok(lit->elements[pos]);
}

} // namespace SOMTParser::MiniZinc
