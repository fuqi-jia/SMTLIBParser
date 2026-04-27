/* -*- C++ -*-
 *
 * Plan implementation
 */

#include "somtparser/unified/plan.h"

namespace SOMTParser::Unified {

Plan Plan::fromJson(const nlohmann::json& j) {
    return j.get<Plan>();
}

nlohmann::json Plan::toJson() const {
    nlohmann::json j = *this;
    return j;
}

// ── PlanConstraint builders ────────────────────────────────────────

PlanConstraint PlanConstraint::mkOp(const std::string& op_name,
                                     std::vector<nlohmann::json> args) {
    nlohmann::json j;
    j["op"] = op_name;
    j["args"] = std::move(args);
    return PlanConstraint(j);
}

PlanConstraint PlanConstraint::mkVar(const std::string& name) {
    nlohmann::json j;
    j["var"] = name;
    return PlanConstraint(j);
}

PlanConstraint PlanConstraint::mkLit(int64_t v) {
    nlohmann::json j;
    j["lit"] = v;
    return PlanConstraint(j);
}

PlanConstraint PlanConstraint::mkLit(bool v) {
    nlohmann::json j;
    j["lit"] = v;
    return PlanConstraint(j);
}

} // namespace SOMTParser::Unified
