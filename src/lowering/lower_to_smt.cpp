/* -*- C++ -*-
 *
 * LowerToSmt implementation — Registry-driven SMT lowering.
 */

#include "somtparser/lowering/lower_to_smt.h"

#include <iostream>

namespace SOMTParser::Lowering {

using ExprPtr = Unified::ExprPtr;

// ── Constructor ────────────────────────────────────────────────────

LowerToSmt::LowerToSmt(Parser& parser, const Unified::UnifiedOpRegistry& registry)
    : parser_(parser), registry_(registry) {}

// ── Error handling ─────────────────────────────────────────────────

void LowerToSmt::addError(const std::string& msg) {
    errors_.push_back(msg);
    std::cerr << "[LowerToSmt] " << msg << "\n";
}

// ── Sort lowering ──────────────────────────────────────────────────

std::shared_ptr<Sort> LowerToSmt::lowerSort(const Unified::UnifiedSort& sort) {
    switch (sort.kind) {
        case Unified::UnifiedSort::Kind::BOOL:  return SortManager::getBool();
        case Unified::UnifiedSort::Kind::INT:   return SortManager::getInt();
        case Unified::UnifiedSort::Kind::REAL:  return SortManager::getReal();
        case Unified::UnifiedSort::Kind::FLOAT: return SortManager::getReal();
        case Unified::UnifiedSort::Kind::STRING: return SortManager::getStr();
        case Unified::UnifiedSort::Kind::ARRAY: {
            auto idx = SortManager::getInt();
            std::shared_ptr<Sort> elem = SortManager::getUnknown();
            if (!sort.params.empty()) elem = lowerSort(sort.params[0]);
            return parser_.getSortManager()->createArraySort(idx, elem);
        }
        default:
            return SortManager::getUnknown();
    }
}

// ── Variable declaration lowering ──────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerVarDecl(const Unified::UnifiedVarDecl& decl) {
    auto sort = lowerSort(decl.type.sort);
    auto var = parser_.mkVar(sort, decl.name);
    var_map_[decl.name] = var;
    if (decl.init) {
        auto init = lowerExpr(decl.init);
        if (init) {
            auto eq = parser_.mkEq(var, init);
            return eq;
        }
    }
    return nullptr;
}

// ── Native op lowering ─────────────────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerNative(const Unified::UnifiedOpDef& def,
                                                   const std::vector<std::shared_ptr<DAGNode>>& args) {
    const std::string& name = def.smt_lowering.native_smt_name;

    // Handle variadic / multi-ary ops
    if (name == "+") {
        if (args.empty()) return parser_.mkConstInt(0);
        if (args.size() == 1) return args[0];
        return parser_.mkAdd(args);
    }
    if (name == "-") {
        if (args.size() == 1) return parser_.mkNeg(args[0]);
        if (args.size() == 2) return parser_.mkSub(args[0], args[1]);
        addError("'-' with >2 args not supported");
        return nullptr;
    }
    if (name == "*") {
        if (args.empty()) return parser_.mkConstInt(1);
        if (args.size() == 1) return args[0];
        // Parser may not have variadic mkMul; chain binary
        auto result = args[0];
        for (size_t i = 1; i < args.size(); ++i) result = parser_.mkMul(result, args[i]);
        return result;
    }
    if (name == "div") {
        if (args.size() == 2) return parser_.mkDivInt(args[0], args[1]);
        addError("'div' requires 2 args");
        return nullptr;
    }
    if (name == "/") {
        if (args.size() == 2) return parser_.mkDivReal(args[0], args[1]);
        addError("'/' requires 2 args");
        return nullptr;
    }
    if (name == "mod") {
        if (args.size() == 2) return parser_.mkMod(args[0], args[1]);
        addError("'mod' requires 2 args");
        return nullptr;
    }
    if (name == "^") {
        if (args.size() == 2) return parser_.mkPow(args[0], args[1]);
        addError("'^' requires 2 args");
        return nullptr;
    }

    // Boolean
    if (name == "and") {
        if (args.empty()) return parser_.mkTrue();
        if (args.size() == 1) return args[0];
        return parser_.mkAnd(args);
    }
    if (name == "or") {
        if (args.empty()) return parser_.mkFalse();
        if (args.size() == 1) return args[0];
        return parser_.mkOr(args);
    }
    if (name == "not") {
        if (args.size() == 1) return parser_.mkNot(args[0]);
        addError("'not' requires 1 arg");
        return nullptr;
    }
    if (name == "=>") {
        if (args.size() == 2) return parser_.mkImplies(args[0], args[1]);
        addError("'=>' requires 2 args");
        return nullptr;
    }
    if (name == "xor") {
        if (args.size() == 2) return parser_.mkXor(args[0], args[1]);
        addError("'xor' requires 2 args");
        return nullptr;
    }

    // Relations
    if (name == "=") {
        if (args.size() == 2) return parser_.mkEq(args[0], args[1]);
        if (args.size() > 2) return parser_.mkEq(args);
        addError("'=' requires >= 2 args");
        return nullptr;
    }
    if (name == "distinct") {
        if (args.size() >= 2) {
            // SMT-LIB distinct; fallback: pairwise !=
            std::vector<std::shared_ptr<DAGNode>> neqs;
            for (size_t i = 0; i < args.size(); ++i) {
                for (size_t j = i + 1; j < args.size(); ++j) {
                    neqs.push_back(parser_.mkNot(parser_.mkEq(args[i], args[j])));
                }
            }
            if (neqs.empty()) return parser_.mkTrue();
            if (neqs.size() == 1) return neqs[0];
            return parser_.mkAnd(neqs);
        }
        addError("'distinct' requires >= 2 args");
        return nullptr;
    }
    if (name == "<") {
        if (args.size() == 2) return parser_.mkLt(args[0], args[1]);
        addError("'<' requires 2 args");
        return nullptr;
    }
    if (name == "<=") {
        if (args.size() == 2) return parser_.mkLe(args[0], args[1]);
        addError("'<=' requires 2 args");
        return nullptr;
    }
    if (name == ">") {
        if (args.size() == 2) return parser_.mkGt(args[0], args[1]);
        addError("'>' requires 2 args");
        return nullptr;
    }
    if (name == ">=") {
        if (args.size() == 2) return parser_.mkGe(args[0], args[1]);
        addError("'>=' requires 2 args");
        return nullptr;
    }

    // Array
    if (name == "select") {
        if (args.size() == 2) return parser_.mkSelect(args[0], args[1]);
        addError("'select' requires 2 args");
        return nullptr;
    }

    // Quantifiers (handled separately in lowerExpr)
    if (name == "forall" || name == "exists") {
        addError("Quantifiers should be handled in lowerExpr, not lowerNative");
        return nullptr;
    }

    addError("Unknown native SMT name: '" + name + "' for op " + def.unified_name);
    return nullptr;
}

// ── Decomposition lowering (stub) ──────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerDecompose(const Unified::UnifiedOpDef& def,
                                                     const std::vector<std::shared_ptr<DAGNode>>& args) {
    (void)args;
    addError("DECOMPOSE strategy not yet implemented for op: " + def.unified_name +
             " (template: " + def.smt_lowering.decomposition_template + ")");
    return parser_.mkTrue(); // fallback: no-op
}

// ── OpNode lowering ────────────────────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerOpNode(const Unified::UnifiedExpr::OpNode& op) {
    const auto* def = registry_.getDef(op.op);
    if (!def) {
        addError("Unknown op ref in lowering");
        return nullptr;
    }

    std::vector<std::shared_ptr<DAGNode>> dag_args;
    dag_args.reserve(op.args.size());
    for (auto& arg : op.args) {
        dag_args.push_back(lowerExpr(arg));
    }

    switch (def->smt_lowering.strategy) {
        case Unified::SmtLoweringDef::Strategy::NATIVE:
            return lowerNative(*def, dag_args);
        case Unified::SmtLoweringDef::Strategy::DECOMPOSE:
            return lowerDecompose(*def, dag_args);
        case Unified::SmtLoweringDef::Strategy::AXIOMATIZE:
            addError("AXIOMATIZE strategy not yet implemented for: " + def->unified_name);
            return parser_.mkTrue();
        case Unified::SmtLoweringDef::Strategy::EXTERNAL:
            addError("EXTERNAL strategy not yet implemented for: " + def->unified_name);
            return parser_.mkTrue();
        case Unified::SmtLoweringDef::Strategy::UNSUPPORTED:
        default:
            addError("Unsupported SMT lowering for: " + def->unified_name);
            return parser_.mkTrue();
    }
}

// ── Expression lowering ────────────────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerExpr(const ExprPtr& expr) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case Unified::UnifiedExpr::Kind::LITERAL: {
            auto* lit = expr->asLiteral();
            if (!lit) return nullptr;
            switch (lit->lit_kind) {
                case Unified::UnifiedExpr::Literal::LitKind::BOOL:
                    return std::get<bool>(lit->value) ? parser_.mkTrue() : parser_.mkFalse();
                case Unified::UnifiedExpr::Literal::LitKind::INT:
                    return parser_.mkConstInt(static_cast<int>(std::get<int64_t>(lit->value)));
                case Unified::UnifiedExpr::Literal::LitKind::FLOAT:
                    return parser_.mkConstReal(std::get<double>(lit->value));
                case Unified::UnifiedExpr::Literal::LitKind::STRING:
                    return parser_.mkConstStr(std::get<std::string>(lit->value));
            }
            return nullptr;
        }

        case Unified::UnifiedExpr::Kind::IDENT: {
            auto* id = expr->asIdent();
            if (!id) return nullptr;
            auto it = var_map_.find(id->name);
            if (it != var_map_.end()) return it->second;
            addError("Undefined variable in lowering: " + id->name);
            return nullptr;
        }

        case Unified::UnifiedExpr::Kind::OP: {
            auto* op = expr->asOp();
            if (!op) return nullptr;
            return lowerOpNode(*op);
        }

        case Unified::UnifiedExpr::Kind::ARRAY_LIT: {
            auto* arr = expr->asArray();
            if (!arr) return nullptr;
            std::vector<std::shared_ptr<DAGNode>> elems;
            for (auto& e : arr->elems) {
                auto de = lowerExpr(e);
                if (de) elems.push_back(de);
            }
            if (elems.empty()) return parser_.mkConstInt(0);
            // mkConstArray takes (sort, default_value), not a list
            // For array literals we need a different approach; return first elem for now
            return elems[0];
        }

        case Unified::UnifiedExpr::Kind::SET_LIT: {
            auto* s = expr->asSet();
            if (!s) return nullptr;
            addError("Set literal lowering not yet implemented");
            return nullptr;
        }

        case Unified::UnifiedExpr::Kind::TUPLE_LIT:
        case Unified::UnifiedExpr::Kind::RECORD_LIT: {
            addError("Tuple/Record literal lowering not yet implemented");
            return nullptr;
        }

        case Unified::UnifiedExpr::Kind::LET: {
            auto* let = expr->asLet();
            if (!let) return nullptr;
            for (auto& local : let->locals) {
                auto sort = lowerSort(local.type.sort);
                auto var = parser_.mkVar(sort, local.name);
                var_map_[local.name] = var;
            }
            return lowerExpr(let->body);
        }

        case Unified::UnifiedExpr::Kind::ITE: {
            auto* ite = expr->asIte();
            if (!ite) return nullptr;
            auto cond = lowerExpr(ite->cond);
            auto then_br = lowerExpr(ite->then_expr);
            auto else_br = lowerExpr(ite->else_expr);
            if (!cond || !then_br || !else_br) return nullptr;
            return parser_.mkIte(cond, then_br, else_br);
        }

        case Unified::UnifiedExpr::Kind::FORALL: {
            auto* q = expr->asQuant();
            if (!q) return nullptr;
            std::vector<std::shared_ptr<DAGNode>> params;
            for (auto& [var, set_expr] : q->generators) {
                auto sort = SortManager::getInt();
                auto v = parser_.mkVar(sort, var);
                var_map_[var] = v;
                params.push_back(v);
            }
            auto body = lowerExpr(q->body);
            if (!body) body = parser_.mkTrue();
            params.push_back(body);
            return parser_.mkForall(params);
        }

        case Unified::UnifiedExpr::Kind::EXISTS: {
            auto* q = expr->asQuant();
            if (!q) return nullptr;
            std::vector<std::shared_ptr<DAGNode>> params;
            for (auto& [var, set_expr] : q->generators) {
                auto sort = SortManager::getInt();
                auto v = parser_.mkVar(sort, var);
                var_map_[var] = v;
                params.push_back(v);
            }
            auto body = lowerExpr(q->body);
            if (!body) body = parser_.mkTrue();
            params.push_back(body);
            return parser_.mkExists(params);
        }
    }

    return nullptr;
}

// ── Model lowering ─────────────────────────────────────────────────

std::shared_ptr<DAGNode> LowerToSmt::lowerModel(const Unified::UnifiedModel& model) {
    std::vector<std::shared_ptr<DAGNode>> assertions;

    // Lower variable declarations
    for (auto& decl : model.vars) {
        auto sort = lowerSort(decl.type.sort);
        auto var = parser_.mkVar(sort, decl.name);
        var_map_[decl.name] = var;
        if (decl.init) {
            auto init = lowerExpr(decl.init);
            if (init) assertions.push_back(parser_.mkEq(var, init));
        }
    }

    // Lower parameter declarations (treat as variables with init)
    for (auto& decl : model.parameters) {
        auto sort = lowerSort(decl.type.sort);
        auto var = parser_.mkVar(sort, decl.name);
        var_map_[decl.name] = var;
        if (decl.init) {
            auto init = lowerExpr(decl.init);
            if (init) assertions.push_back(parser_.mkEq(var, init));
        }
    }

    // Lower constraints
    for (auto& c : model.constraints) {
        if (c.expr) {
            auto dag = lowerExpr(c.expr);
            if (dag) assertions.push_back(dag);
        }
    }

    if (assertions.empty()) return parser_.mkTrue();
    if (assertions.size() == 1) return assertions[0];
    return parser_.mkAnd(assertions);
}

} // namespace SOMTParser::Lowering
