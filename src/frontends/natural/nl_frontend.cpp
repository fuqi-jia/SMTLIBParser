/* -*- C++ -*-
 *
 * Natural Language Frontend implementation
 */

#include "somtparser/frontends/natural/nl_frontend.h"

#include <iostream>

namespace SOMTParser::Frontend::Natural {

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
        std::cerr << "[NLEmitter] Warning: unknown op '" << op_name << "'\n";
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

U::UnifiedModel PlanEmitter::emit(const Plan& plan) {
    U::UnifiedModel model;

    for (auto& sym : plan.symbols) {
        U::UnifiedVarDecl decl;
        decl.name = sym.name;
        decl.type = parseType(sym.unified_type);
        decl.type.par_var = sym.is_var ? U::UnifiedType::ParVar::VAR : U::UnifiedType::ParVar::PAR;
        if (sym.domain && !sym.domain->empty()) {
            // Parse domain into a literal or range expression
            // For now, just store as a string annotation
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

// ── NaturalFrontend ────────────────────────────────────────────────

NaturalFrontend::NaturalFrontend(Parser& parser,
                                  const U::UnifiedOpRegistry& registry)
    : parser_(parser), validator_(registry), emitter_(registry) {}

U::UnifiedModel NaturalFrontend::parsePlan(const Plan& plan) {
    if (!validator_.validate(plan)) {
        throw std::runtime_error("Plan validation failed");
    }
    return emitter_.emit(plan);
}

U::UnifiedModel NaturalFrontend::parsePlanJson(const std::string& json_str) {
    auto j = nlohmann::json::parse(json_str);
    auto plan = Plan::fromJson(j);
    return parsePlan(plan);
}

U::UnifiedModel NaturalFrontend::parseString(const std::string& source,
                                              const std::string& /*filename*/) {
    // Stub: in a real system, this would call an LLM to generate a Plan
    // from the natural language source. For now, we support only
    // programmatically constructed Plans.
    (void)source;
    return U::UnifiedModel{};
}

} // namespace SOMTParser::Frontend::Natural
