/* -*- C++ -*-
 *
 * Natural Language Frontend — thin wrapper implementation
 */

#include "somtparser/frontends/natural/nl_frontend.h"

namespace SOMTParser::Frontend::Natural {

namespace U = SOMTParser::Unified;

NaturalFrontend::NaturalFrontend(Parser& parser,
                                  const U::UnifiedOpRegistry& registry)
    : parser_(parser), validator_(registry), emitter_(registry),
      nl2plan_(registry) {}

U::UnifiedModel NaturalFrontend::parsePlan(const U::Plan& plan) {
    if (!validator_.validate(plan)) {
        throw std::runtime_error("Plan validation failed");
    }
    return emitter_.emit(plan);
}

U::UnifiedModel NaturalFrontend::parsePlanJson(const std::string& json_str) {
    auto j = nlohmann::json::parse(json_str);
    auto plan = U::Plan::fromJson(j);
    return parsePlan(plan);
}

U::UnifiedModel NaturalFrontend::parseString(const std::string& source,
                                              const std::string& /*filename*/) {
    // NL → Plan → Unified::Model pipeline
    last_plan_ = nl2plan_.convert(source);
    return emitter_.emit(last_plan_);
}

} // namespace SOMTParser::Frontend::Natural
