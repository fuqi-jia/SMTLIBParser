/* -*- C++ -*-
 *
 * UnifiedPipeline implementation
 */

#include "somtparser/unified/unified_pipeline.h"
#include "somtparser/unified/plan_builder.h"
#include "somtparser/lowering/lower_to_smt.h"

namespace SOMTParser::Unified {

UnifiedPipeline::UnifiedPipeline(const UnifiedOpRegistry& registry)
    : registry_(registry) {}

std::string UnifiedPipeline::planToSmt2(const Plan& plan) {
    errors_.clear();

    PlanEmitter emitter(registry_);
    auto model = emitter.emit(plan);
    return modelToSmt2(model, plan.logic_hint);
}

std::string UnifiedPipeline::modelToSmt2(const UnifiedModel& model,
                                          const std::string& logic_hint) {
    if (!logic_hint.empty()) {
        auto logic = mapLogicHint(logic_hint);
        parser_.getOptions()->setLogic(logic);
    }

    Lowering::LowerToSmt lowerer(parser_, registry_);
    auto assertion = lowerer.lowerModel(model);
    if (assertion) {
        (parser_.assert)(assertion);
    }
    return parser_.dumpSMT2();
}

std::string UnifiedPipeline::mapLogicHint(const std::string& hint) {
    if (hint == "SAT" || hint == "QF_UF") return "QF_UF";
    if (hint == "CP" || hint == "QF_LIA") return "QF_LIA";
    if (hint == "QF_LRA") return "QF_LRA";
    if (hint == "QF_NIA") return "QF_NIA";
    if (hint == "QF_BV") return "ALL";  // BV lowering uses Int for now; use ALL
    if (hint == "QF_S") return "ALL";   // String lowering incomplete; use ALL
    if (hint == "LIA") return "LIA";
    if (hint == "LRA") return "LRA";
    if (hint == "ALL") return "ALL";
    // Default fallback
    return "ALL";
}

void UnifiedPipeline::addError(const std::string& msg) {
    errors_.push_back(msg);
}

} // namespace SOMTParser::Unified
