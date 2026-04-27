/* -*- C++ -*-
 *
 * Plan Builder implementation — PlanValidator + PlanEmitter
 */

#include "somtparser/unified/plan_builder.h"

#include <iostream>

namespace SOMTParser::Unified {

namespace U = SOMTParser::Unified;

// ── PlanValidator ──────────────────────────────────────────────────

bool PlanValidator::validate(const Plan& plan) {
    errors_.clear();
    bool ok = true;

    for (size_t i = 0; i < plan.constraints.size(); ++i) {
        if (!validateExpr(plan.constraints[i].expr)) {
            ok = false;
            addError("Constraint " + std::to_string(i) + " failed validation");
        }
    }

    if (plan.objective && !validateExpr(plan.objective->expr)) {
        ok = false;
        addError("Objective failed validation");
    }

    return ok;
}

bool PlanValidator::validateExpr(const nlohmann::json& expr) {
    if (expr.is_null()) return true;

    if (expr.contains("lit")) return true;
    if (expr.contains("var")) return true;

    if (expr.contains("op")) {
        std::string op_name = expr.value("op", "");
        auto ref = registry_.lookupByUnifiedName(op_name);
        if (!ref.valid()) {
            // Try language lookup
            ref = registry_.lookupByLangName("minizinc", op_name);
        }
        if (!ref.valid()) {
            addError("Unknown op in plan: '" + op_name + "'");
            return false;
        }

        if (expr.contains("args")) {
            auto& args = expr["args"];
            const auto* def = registry_.getDef(ref);
            if (def && def->arity >= 0) {
                if (static_cast<int>(args.size()) != def->arity) {
                    addError("Arity mismatch for '" + op_name + "': expected " +
                             std::to_string(def->arity) + ", got " +
                             std::to_string(args.size()));
                    return false;
                }
            }
            for (auto& arg : args) {
                if (!validateExpr(arg)) return false;
            }
        }
        return true;
    }

    return true;
}

void PlanValidator::addError(const std::string& msg) {
    errors_.push_back(msg);
}

// ── PlanEmitter ────────────────────────────────────────────────────

U::UnifiedType PlanEmitter::parseType(const std::string& type_str) const {
    if (type_str == "bool") return U::UnifiedType(U::UnifiedSort::mkBool());
    if (type_str == "int")  return U::UnifiedType(U::UnifiedSort::mkInt());
    if (type_str == "float" || type_str == "real") return U::UnifiedType(U::UnifiedSort::mkReal());
    if (type_str == "string") return U::UnifiedType(U::UnifiedSort::mkString());
    // TODO: parse array(int), set(int), etc.
    return U::UnifiedType(U::UnifiedSort::mkAny());
}

U::ExprPtr PlanEmitter::emitExpr(const nlohmann::json& expr) const {
    if (expr.is_null()) return nullptr;

    // Handle nested "expr" wrapper for robustness
    if (expr.contains("expr") && expr.size() == 1) {
        return emitExpr(expr["expr"]);
    }

    if (expr.contains("lit")) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
        if (expr["lit"].is_boolean()) {
            node->data = U::UnifiedExpr::Literal::mkBool(expr["lit"]);
        } else if (expr["lit"].is_number_integer()) {
            node->data = U::UnifiedExpr::Literal::mkInt(expr["lit"]);
        } else if (expr["lit"].is_number_float()) {
            node->data = U::UnifiedExpr::Literal::mkFloat(expr["lit"]);
        } else if (expr["lit"].is_string()) {
            node->data = U::UnifiedExpr::Literal::mkString(expr["lit"]);
        }
        return node;
    }

    if (expr.contains("var")) {
        auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::IDENT, U::SourceLoc{});
        node->data = U::UnifiedExpr::Ident{expr["var"]};
        return node;
    }

    if (expr.contains("op")) {
        std::string op_name = expr.value("op", "");
        std::vector<nlohmann::json> args;
        if (expr.contains("args")) {
            args = expr["args"].get<std::vector<nlohmann::json>>();
        }
        return emitOp(op_name, args);
    }

    return nullptr;
}

U::ExprPtr PlanEmitter::emitOp(const std::string& op_name,
                                const std::vector<nlohmann::json>& args) const {
    auto ref = registry_.lookupByUnifiedName(op_name);
    if (!ref.valid()) {
        ref = registry_.lookupByLangName("minizinc", op_name);
    }
    if (!ref.valid()) {
        std::cerr << "[PlanEmitter] Warning: unknown op '" << op_name << "'\n";
        return nullptr;
    }

    std::vector<U::ExprPtr> uargs;
    uargs.reserve(args.size());
    for (auto& arg : args) {
        uargs.push_back(emitExpr(arg));
    }

    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = U::UnifiedExpr::OpNode{ref, std::move(uargs), {}};
    return node;
}

/** Helper: create a binary op expression node. */
static U::ExprPtr mkBinOp(const U::UnifiedOpRegistry& reg,
                          const std::string& op_name,
                          U::ExprPtr lhs, U::ExprPtr rhs) {
    auto ref = reg.lookupByUnifiedName(op_name);
    if (!ref.valid()) return nullptr;
    auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    node->data = U::UnifiedExpr::OpNode{ref, {std::move(lhs), std::move(rhs)}, {}};
    return node;
}

/** Parse domain string and emit constraints.
 *  Supports: "1..10" (range), "{1,2,3}" (enumeration)
 */
static std::vector<U::ExprPtr> emitDomainConstraints(
        const U::UnifiedOpRegistry& reg,
        const std::string& var_name,
        const std::string& domain) {
    std::vector<U::ExprPtr> result;
    auto var = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::IDENT, U::SourceLoc{});
    var->data = U::UnifiedExpr::Ident{var_name};

    // Range: "1..10" or "0..1"
    size_t dots = domain.find("..");
    if (dots != std::string::npos) {
        std::string lo_str = domain.substr(0, dots);
        std::string hi_str = domain.substr(dots + 2);
        // Trim whitespace
        auto trim = [](std::string& s) {
            size_t a = s.find_first_not_of(" \t");
            size_t b = s.find_last_not_of(" \t");
            if (a != std::string::npos) s = s.substr(a, b - a + 1);
        };
        trim(lo_str); trim(hi_str);
        try {
            int lo = std::stoi(lo_str);
            int hi = std::stoi(hi_str);
            auto lo_lit = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
            lo_lit->data = U::UnifiedExpr::Literal{U::UnifiedExpr::Literal::LitKind::INT, static_cast<int64_t>(lo)};
            auto hi_lit = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
            hi_lit->data = U::UnifiedExpr::Literal{U::UnifiedExpr::Literal::LitKind::INT, static_cast<int64_t>(hi)};
            auto ge = mkBinOp(reg, "ge", var, lo_lit);
            auto le = mkBinOp(reg, "le", var, hi_lit);
            if (ge) result.push_back(ge);
            if (le) result.push_back(le);
        } catch (...) {
            // Ignore malformed domain
        }
        return result;
    }

    // Enumeration: "{1,2,3}"
    if (!domain.empty() && domain.front() == '{' && domain.back() == '}') {
        std::string inner = domain.substr(1, domain.size() - 2);
        std::vector<U::ExprPtr> eqs;
        size_t pos = 0;
        while (pos < inner.size()) {
            size_t comma = inner.find(',', pos);
            std::string val_str = (comma == std::string::npos) ? inner.substr(pos) : inner.substr(pos, comma - pos);
            size_t a = val_str.find_first_not_of(" \t");
            size_t b = val_str.find_last_not_of(" \t");
            if (a != std::string::npos) val_str = val_str.substr(a, b - a + 1);
            try {
                int val = std::stoi(val_str);
                auto lit = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});
                lit->data = U::UnifiedExpr::Literal{U::UnifiedExpr::Literal::LitKind::INT, static_cast<int64_t>(val)};
                auto eq = mkBinOp(reg, "eq", var, lit);
                if (eq) eqs.push_back(eq);
            } catch (...) {}
            if (comma == std::string::npos) break;
            pos = comma + 1;
        }
        if (eqs.size() == 1) {
            result.push_back(eqs[0]);
        } else if (eqs.size() > 1) {
            auto or_ref = reg.lookupByUnifiedName("bool_or");
            if (or_ref.valid()) {
                auto node = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
                node->data = U::UnifiedExpr::OpNode{or_ref, std::move(eqs), {}};
                result.push_back(node);
            }
        }
        return result;
    }

    return result;
}

U::UnifiedModel PlanEmitter::emit(const Plan& plan) {
    U::UnifiedModel model;

    for (auto& sym : plan.symbols) {
        U::UnifiedVarDecl decl;
        decl.name = sym.name;
        decl.type = parseType(sym.unified_type);
        decl.type.par_var = sym.is_var ? U::UnifiedType::ParVar::VAR : U::UnifiedType::ParVar::PAR;
        if (sym.domain && !sym.domain->empty()) {
            auto dom_constraints = emitDomainConstraints(registry_, sym.name, *sym.domain);
            for (auto& c : dom_constraints) {
                if (c) model.constraints.emplace_back(c);
            }
        }
        if (sym.is_var) {
            model.vars.push_back(std::move(decl));
        } else {
            model.parameters.push_back(std::move(decl));
        }
    }

    for (auto& c : plan.constraints) {
        auto expr = emitExpr(c.expr);
        if (expr) model.constraints.emplace_back(expr);
    }

    if (plan.objective) {
        U::UnifiedObjective::Mode mode = U::UnifiedObjective::Mode::SATISFY;
        if (plan.objective->mode == "minimize") mode = U::UnifiedObjective::Mode::MINIMIZE;
        else if (plan.objective->mode == "maximize") mode = U::UnifiedObjective::Mode::MAXIMIZE;
        auto expr = emitExpr(plan.objective->expr);
        model.objectives.emplace_back(mode, expr);
    }

    return model;
}

} // namespace SOMTParser::Unified
