/* -*- Header -*-
 *
 * Natural Language Frontend — Plan → Unified IR.
 *
 * 1. Accept a Plan (from LLM or programmatically constructed)
 * 2. Validate Plan against UnifiedOpRegistry
 * 3. Emit Plan → Unified::Model
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef NL_FRONTEND_H
#define NL_FRONTEND_H

#include "somtparser/frontends/natural/nl_plan.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/frontend/parser.h"

#include <string>
#include <vector>

namespace SOMTParser::Frontend::Natural {

/**
 * Validates a Plan against the UnifiedOpRegistry.
 * Checks: all ops exist, arities match (where possible), types are known.
 */
class PlanValidator {
public:
    explicit PlanValidator(const Unified::UnifiedOpRegistry& registry)
        : registry_(registry) {}

    bool validate(const Plan& plan);
    const std::vector<std::string>& errors() const { return errors_; }

private:
    const Unified::UnifiedOpRegistry& registry_;
    std::vector<std::string> errors_;

    bool validateExpr(const nlohmann::json& expr);
    void addError(const std::string& msg);
};

/**
 * Emits a validated Plan into a Unified::Model.
 */
class PlanEmitter {
public:
    explicit PlanEmitter(const Unified::UnifiedOpRegistry& registry)
        : registry_(registry) {}

    Unified::UnifiedModel emit(const Plan& plan);

private:
    const Unified::UnifiedOpRegistry& registry_;

    Unified::UnifiedType parseType(const std::string& type_str) const;
    Unified::ExprPtr emitExpr(const nlohmann::json& expr) const;
    Unified::ExprPtr emitOp(const std::string& op_name,
                             const std::vector<nlohmann::json>& args) const;
};

/**
 * High-level Natural Language frontend.
 * In a real system, this would call an LLM to generate the Plan from NL text.
 * Here we provide the programmatic interface.
 */
class NaturalFrontend {
public:
    NaturalFrontend(Parser& parser, const Unified::UnifiedOpRegistry& registry);

    /** Parse a pre-constructed Plan JSON into Unified::Model. */
    Unified::UnifiedModel parsePlan(const Plan& plan);

    /** Parse Plan from JSON string. */
    Unified::UnifiedModel parsePlanJson(const std::string& json_str);

    /** High-level: "x is int, x > 0, minimize x" → Plan → Unified::Model.
     *  (Stub: returns empty model; real implementation needs LLM.)
     */
    Unified::UnifiedModel parseString(const std::string& source,
                                       const std::string& filename = "<nl>");

    PlanValidator& validator() { return validator_; }
    PlanEmitter& emitter() { return emitter_; }

private:
    Parser& parser_;
    PlanValidator validator_;
    PlanEmitter emitter_;
};

} // namespace SOMTParser::Frontend::Natural

#endif // NL_FRONTEND_H
