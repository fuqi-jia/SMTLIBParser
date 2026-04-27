/* -*- Header -*-
 *
 * Natural Language Frontend — thin wrapper around Nl2Plan + Unified::PlanEmitter.
 *
 * Provides a convenient single-call interface for NL → UnifiedModel.
 * For direct Plan → SMT-LIB2, use UnifiedPipeline in the unified layer.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef NL_FRONTEND_H
#define NL_FRONTEND_H

#include "somtparser/unified/plan.h"
#include "somtparser/unified/plan_builder.h"
#include "somtparser/frontends/natural/nl2plan.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/frontend/parser.h"

#include <string>
#include <vector>

namespace SOMTParser::Frontend::Natural {

/**
 * Thin convenience wrapper: NL text → Unified::Model.
 *
 * Internally delegates to Nl2Plan (LLM) + Unified::PlanEmitter.
 * For full end-to-end NL → SMT-LIB2, use Unified::UnifiedPipeline.
 */
class NaturalFrontend {
public:
    NaturalFrontend(Parser& parser, const Unified::UnifiedOpRegistry& registry);

    /** Parse a pre-constructed Plan into Unified::Model. */
    Unified::UnifiedModel parsePlan(const Unified::Plan& plan);

    /** Parse Plan from JSON string. */
    Unified::UnifiedModel parsePlanJson(const std::string& json_str);

    /** High-level: "x is int, x > 0, minimize x" → Plan → Unified::Model. */
    Unified::UnifiedModel parseString(const std::string& source,
                                       const std::string& filename = "<nl>");

    Unified::PlanValidator& validator() { return validator_; }
    Unified::PlanEmitter& emitter() { return emitter_; }
    Nl2Plan& nl2plan() { return nl2plan_; }

    /** Access the last successfully parsed Plan. */
    const Unified::Plan& lastPlan() const { return last_plan_; }

private:
    Parser& parser_;
    Unified::PlanValidator validator_;
    Unified::PlanEmitter emitter_;
    Nl2Plan nl2plan_;
    Unified::Plan last_plan_;
};

} // namespace SOMTParser::Frontend::Natural

#endif // NL_FRONTEND_H
