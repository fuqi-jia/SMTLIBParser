/* -*- Header -*-
 *
 * NL Plan — Structured plan schema for Natural Language → Unified IR.
 *
 * The Plan is a JSON-serializable intermediate representation that uses
 * Unified op names (e.g., "int_add", "all_different", "forall").
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef NL_PLAN_H
#define NL_PLAN_H

#include "somtparser/unified/unified_ir.h"

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace SOMTParser::Frontend::Natural {

// ── Plan symbol (variable / parameter) ─────────────────────────────

struct PlanSymbol {
    std::string name;
    std::string unified_type;   // e.g., "int", "bool", "array(int)"
    std::optional<std::string> domain; // e.g., "1..10", "{1,3,5}", "true,false"
    bool is_var = true;

    static PlanSymbol mkInt(const std::string& name, const std::string& domain = "") {
        PlanSymbol s; s.name = name; s.unified_type = "int"; s.domain = domain; return s;
    }
    static PlanSymbol mkBool(const std::string& name) {
        PlanSymbol s; s.name = name; s.unified_type = "bool"; return s;
    }
};

inline void to_json(nlohmann::json& j, const PlanSymbol& s) {
    j = nlohmann::json{{"name", s.name}, {"unified_type", s.unified_type}, {"is_var", s.is_var}};
    if (s.domain) j["domain"] = *s.domain;
}
inline void from_json(const nlohmann::json& j, PlanSymbol& s) {
    j.at("name").get_to(s.name);
    j.at("unified_type").get_to(s.unified_type);
    j.at("is_var").get_to(s.is_var);
    if (j.contains("domain")) s.domain = j.at("domain").get<std::string>();
}

// ── Plan constraint (structured expression tree) ───────────────────

struct PlanConstraint {
    // The constraint is a JSON-like expression tree using Unified op names.
    // Example:
    //   {"op": "lt", "args": [
    //     {"op": "int_add", "args": [{"var": "x"}, {"lit": 1}]},
    //     {"lit": 10}
    //   ]}
    nlohmann::json expr;

    PlanConstraint() = default;
    explicit PlanConstraint(nlohmann::json e) : expr(std::move(e)) {}

    // Convenience builders
    static PlanConstraint mkOp(const std::string& op_name,
                                std::vector<nlohmann::json> args);
    static PlanConstraint mkVar(const std::string& name);
    static PlanConstraint mkLit(int64_t v);
    static PlanConstraint mkLit(bool v);
};

inline void to_json(nlohmann::json& j, const PlanConstraint& c) { j = c.expr; }
inline void from_json(const nlohmann::json& j, PlanConstraint& c) { c.expr = j; }

// ── Plan objective ─────────────────────────────────────────────────

struct PlanObjective {
    std::string mode;  // "satisfy", "minimize", "maximize"
    nlohmann::json expr;
};

inline void to_json(nlohmann::json& j, const PlanObjective& o) { j = nlohmann::json{{"mode", o.mode}, {"expr", o.expr}}; }
inline void from_json(const nlohmann::json& j, PlanObjective& o) { j.at("mode").get_to(o.mode); j.at("expr").get_to(o.expr); }

// ── Plan ───────────────────────────────────────────────────────────

struct Plan {
    std::string version = "1";
    std::string logic_hint;              // "QF_LIA", "CP", "SAT", ...
    std::vector<PlanSymbol> symbols;
    std::vector<PlanConstraint> constraints;
    std::optional<PlanObjective> objective;

    static Plan fromJson(const nlohmann::json& j);
    nlohmann::json toJson() const;
};

// Custom serialization for Plan (std::optional needs special handling)
inline void to_json(nlohmann::json& j, const Plan& p) {
    j = nlohmann::json{{"version", p.version}, {"logic_hint", p.logic_hint},
                       {"symbols", p.symbols}, {"constraints", p.constraints}};
    if (p.objective) j["objective"] = *p.objective;
}

inline void from_json(const nlohmann::json& j, Plan& p) {
    j.at("version").get_to(p.version);
    if (j.contains("logic_hint")) j.at("logic_hint").get_to(p.logic_hint);
    if (j.contains("symbols")) j.at("symbols").get_to(p.symbols);
    if (j.contains("constraints")) j.at("constraints").get_to(p.constraints);
    if (j.contains("objective")) {
        p.objective = j.at("objective").get<PlanObjective>();
    }
}

} // namespace SOMTParser::Frontend::Natural

#endif // NL_PLAN_H
