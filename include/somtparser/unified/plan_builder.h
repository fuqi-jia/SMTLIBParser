/* -*- Header -*-
 *
 * Plan Builder — Validates a Plan and emits it into Unified::Model.
 *
 * 1. PlanValidator: checks ops exist, arities match, types are known.
 * 2. PlanEmitter:   converts a validated Plan into Unified::Model.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef SOMTPARSER_UNIFIED_PLAN_BUILDER_H
#define SOMTPARSER_UNIFIED_PLAN_BUILDER_H

#include "somtparser/unified/plan.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"

#include <string>
#include <vector>

namespace SOMTParser::Unified {

/**
 * Validates a Plan against the UnifiedOpRegistry.
 * Checks: all ops exist, arities match (where possible), types are known.
 */
class PlanValidator {
public:
    explicit PlanValidator(const UnifiedOpRegistry& registry)
        : registry_(registry) {}

    bool validate(const Plan& plan);
    const std::vector<std::string>& errors() const { return errors_; }

private:
    const UnifiedOpRegistry& registry_;
    std::vector<std::string> errors_;

    bool validateExpr(const nlohmann::json& expr);
    void addError(const std::string& msg);
};

/**
 * Emits a validated Plan into a Unified::Model.
 */
class PlanEmitter {
public:
    explicit PlanEmitter(const UnifiedOpRegistry& registry)
        : registry_(registry) {}

    UnifiedModel emit(const Plan& plan);

private:
    const UnifiedOpRegistry& registry_;

    UnifiedType parseType(const std::string& type_str) const;
    ExprPtr emitExpr(const nlohmann::json& expr) const;
    ExprPtr emitOp(const std::string& op_name,
                    const std::vector<nlohmann::json>& args) const;
};

} // namespace SOMTParser::Unified

#endif // SOMTPARSER_UNIFIED_PLAN_BUILDER_H
