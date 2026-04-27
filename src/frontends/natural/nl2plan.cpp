/* -*- C++ -*-
 *
 * NL → Plan pipeline implementation.
 */

#include "somtparser/frontends/natural/nl2plan.h"
#include "somtparser/frontends/natural/nl_frontend.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace SOMTParser::Frontend::Natural {

using SOMTParser::Unified::Plan;
using SOMTParser::Unified::PlanValidator;

// ── Constructor ────────────────────────────────────────────────────

Nl2Plan::Nl2Plan(const Unified::UnifiedOpRegistry& registry, Nl2PlanConfig config)
    : registry_(registry), config_(std::move(config)) {
    if (!config_.backend) {
        config_.backend = createDefaultBackend();
    }
}

// ── Main conversion ────────────────────────────────────────────────

Plan Nl2Plan::convert(const std::string& nl_text) {
    errors_.clear();

    std::string system_prompt = buildSystemPrompt();
    std::string user_prompt = buildUserPrompt(nl_text);

    if (config_.verbose) {
        std::cerr << "[Nl2Plan] Calling " << config_.backend->name() << "...\n";
    }

    // First attempt
    auto response = config_.backend->complete(system_prompt, user_prompt);
    last_raw_response_ = response.raw_response;

    if (!response.success) {
        addError("LLM call failed: " + response.error_message);
        throw std::runtime_error("LLM call failed: " + response.error_message);
    }

    // Extract JSON
    auto maybe_json = extractJsonBlock(response.text);
    if (!maybe_json) {
        addError("Could not extract JSON from LLM response");
        if (config_.verbose) {
            std::cerr << "[Nl2Plan] Raw text:\n" << response.text << "\n";
        }
        throw std::runtime_error("Could not extract JSON from LLM response");
    }

    // Parse Plan
    Plan plan;
    try {
        auto j = nlohmann::json::parse(*maybe_json);
        plan = Plan::fromJson(j);
    } catch (const std::exception& e) {
        addError("JSON parse error: " + std::string(e.what()));
        throw std::runtime_error("JSON parse error: " + std::string(e.what()));
    }

    // Validate
    PlanValidator validator(registry_);
    if (!validator.validate(plan) && config_.use_retry) {
        // Retry with error feedback
        for (int retry = 0; retry < config_.max_retries; ++retry) {
            if (config_.verbose) {
                std::cerr << "[Nl2Plan] Validation failed, retry " << (retry + 1)
                          << "/" << config_.max_retries << "...\n";
            }

            std::string retry_prompt = buildRetryPrompt(
                nl_text, maybe_json.value(), validator.errors());

            auto retry_response = config_.backend->complete(system_prompt, retry_prompt);
            last_raw_response_ = retry_response.raw_response;

            if (!retry_response.success) {
                addError("Retry LLM call failed: " + retry_response.error_message);
                break;
            }

            auto retry_json = extractJsonBlock(retry_response.text);
            if (!retry_json) {
                addError("Could not extract JSON on retry");
                break;
            }

            try {
                auto j = nlohmann::json::parse(*retry_json);
                plan = Plan::fromJson(j);
            } catch (const std::exception& e) {
                addError("Retry JSON parse error: " + std::string(e.what()));
                break;
            }

            if (validator.validate(plan)) {
                if (config_.verbose) {
                    std::cerr << "[Nl2Plan] Retry succeeded!\n";
                }
                return plan;
            }
        }

        addError("Plan validation failed after " +
                 std::to_string(config_.max_retries) + " retries");
        for (const auto& err : validator.errors()) {
            addError("  - " + err);
        }
        throw std::runtime_error("Plan validation failed after retries");
    }

    return plan;
}

// ── Prompt building ────────────────────────────────────────────────

std::string Nl2Plan::buildSystemPrompt() const {
    const auto& lib = loadPromptLibrary();
    std::ostringstream oss;

    if (lib.loaded) {
        // Use loaded prompt library
        oss << lib.system_message << "\n\n";

        if (config_.use_op_catalog) {
            oss << "Available operators (use these exact unified names):\n";
            oss << buildOpCatalog();
            oss << "\n";
        }

        if (config_.use_few_shot) {
            oss << buildFewShotExamplesFromLibrary();
            oss << "\n";
        }

        oss << "Output Rules:\n";
        int rule_num = 1;
        for (const auto& rule : lib.output_rules) {
            oss << rule_num << ". " << rule << "\n";
            ++rule_num;
        }

        if (!lib.schema_description.is_null()) {
            oss << "\nSchema Reference:\n";
            oss << lib.schema_description.dump(2) << "\n";
        }
    } else {
        // Fallback to hardcoded prompt
        oss << "You are a constraint modeling assistant. "
            << "Your task is to convert a natural language problem description "
            << "into a structured JSON Plan that represents variables, constraints, "
            << "and objectives.\n\n";

        if (config_.use_op_catalog) {
            oss << "Available operators (use these exact unified names):\n";
            oss << buildOpCatalog();
            oss << "\n";
        }

        if (config_.use_few_shot) {
            oss << buildFewShotExamplesHardcoded();
            oss << "\n";
        }

        oss << "Rules:\n"
            << "1. Output ONLY a valid JSON object matching the Plan schema.\n"
            << "2. Do not include explanations outside the JSON.\n"
            << "3. Use unified op names exactly as listed above.\n"
            << "4. All variables used in constraints must be declared in 'symbols'.\n"
            << "5. 'logic_hint' should be one of: QF_LIA, QF_LRA, QF_BV, CP, SAT.\n"
            << "6. If the problem has no objective, omit the 'objective' field.\n";
    }

    return oss.str();
}

std::string Nl2Plan::buildUserPrompt(const std::string& nl_text) const {
    std::ostringstream oss;
    oss << "Convert the following problem into a Plan JSON:\n\n";
    oss << "Problem: \"" << nl_text << "\"\n\n";
    oss << "Plan JSON:";
    return oss.str();
}

std::string Nl2Plan::buildRetryPrompt(
    const std::string& nl_text,
    const std::string& last_plan_json,
    const std::vector<std::string>& validation_errors) const {

    std::ostringstream oss;
    oss << "The previous Plan had validation errors. Please fix them.\n\n";
    oss << "Problem: \"" << nl_text << "\"\n\n";
    oss << "Previous Plan:\n" << last_plan_json << "\n\n";
    oss << "Validation errors:\n";
    for (const auto& err : validation_errors) {
        oss << "- " << err << "\n";
    }
    oss << "\nCorrected Plan JSON:";
    return oss.str();
}

// ── Prompt library loading ─────────────────────────────────────────

const Nl2Plan::PromptLibrary& Nl2Plan::loadPromptLibrary() const {
    if (prompt_lib_.loaded) return prompt_lib_;

    std::ifstream ifs(config_.prompt_template_path);
    if (!ifs) {
        if (config_.verbose) {
            std::cerr << "[Nl2Plan] Prompt template not found at: "
                      << config_.prompt_template_path << " (using fallback)\n";
        }
        prompt_lib_.loaded = false;
        return prompt_lib_;
    }

    try {
        nlohmann::json j;
        ifs >> j;

        prompt_lib_.version = j.value("version", "1.0");
        prompt_lib_.system_message = j.value("system_message",
            "You are a constraint modeling assistant.");

        if (j.contains("output_rules") && j["output_rules"].is_array()) {
            for (const auto& rule : j["output_rules"]) {
                if (rule.is_string()) {
                    prompt_lib_.output_rules.push_back(rule.get<std::string>());
                }
            }
        }

        if (j.contains("few_shot_examples")) {
            prompt_lib_.few_shot_examples = j["few_shot_examples"];
        }

        if (j.contains("schema_description")) {
            prompt_lib_.schema_description = j["schema_description"];
        }

        prompt_lib_.loaded = true;

        if (config_.verbose) {
            std::cerr << "[Nl2Plan] Loaded prompt library v" << prompt_lib_.version
                      << " from " << config_.prompt_template_path << "\n";
        }
    } catch (const std::exception& e) {
        if (config_.verbose) {
            std::cerr << "[Nl2Plan] Failed to parse prompt template: " << e.what()
                      << " (using fallback)\n";
        }
        prompt_lib_.loaded = false;
    }

    return prompt_lib_;
}

// ── Op catalog ─────────────────────────────────────────────────────

std::string Nl2Plan::buildOpCatalog() const {
    std::ostringstream oss;
    static const std::vector<std::pair<std::string, std::string>> ops = {
        {"bool_and", "Boolean conjunction (variadic)"},
        {"bool_or", "Boolean disjunction (variadic)"},
        {"bool_not", "Boolean negation (1 arg)"},
        {"bool_implies", "Boolean implication (2 args)"},
        {"bool_iff", "Boolean equivalence (2 args)"},
        {"bool_xor", "Boolean exclusive or (2 args)"},
        {"eq", "Equality (=)"},
        {"neq", "Disequality (distinct)"},
        {"lt", "Less than (<)"},
        {"le", "Less than or equal (<=)"},
        {"gt", "Greater than (>)"},
        {"ge", "Greater than or equal (>=)"},
        {"int_add", "Integer addition (+, variadic)"},
        {"int_sub", "Integer subtraction (-, 2 args)"},
        {"int_neg", "Integer negation (-, 1 arg)"},
        {"int_mul", "Integer multiplication (*, variadic)"},
        {"int_div", "Integer division (div, 2 args)"},
        {"int_mod", "Integer modulo (mod, 2 args)"},
        {"int_pow", "Integer power (^, 2 args)"},
        {"array_access", "Array element access (select)"},
        {"forall", "Universal quantifier"},
        {"exists", "Existential quantifier"},
        {"set_in", "Set membership (in)"},
        {"set_union", "Set union"},
        {"all_different", "All elements must be pairwise different"},
        {"all_equal", "All elements must be equal"},
        {"increasing", "Array elements in non-decreasing order"},
        {"strictly_increasing", "Array elements in strictly increasing order"},
        {"member", "Value is a member of array"},
        {"count_eq", "Count occurrences of value in array"},
    };

    for (const auto& [name, desc] : ops) {
        oss << "  - " << name << ": " << desc << "\n";
    }
    return oss.str();
}

// ── Few-shot examples from library ─────────────────────────────────

std::string Nl2Plan::buildFewShotExamplesFromLibrary() const {
    std::ostringstream oss;
    oss << "Examples:\n\n";

    if (!prompt_lib_.few_shot_examples.is_array()) {
        return buildFewShotExamplesHardcoded();
    }

    int example_num = 1;
    for (const auto& ex : prompt_lib_.few_shot_examples) {
        if (!ex.contains("problem") || !ex.contains("plan")) continue;

        oss << "Example " << example_num << ":\n";
        oss << "Problem: \"" << ex.value("problem", "") << "\"\n";
        if (ex.contains("logic")) {
            oss << "Logic: " << ex.value("logic", "") << "\n";
        }
        oss << "Plan:\n";
        if (ex.contains("plan")) {
            oss << ex["plan"].dump(2) << "\n";
        }
        oss << "\n";
        ++example_num;
    }

    if (example_num == 1) {
        // No valid examples in library, fallback
        return buildFewShotExamplesHardcoded();
    }

    return oss.str();
}

// ── Few-shot examples (hardcoded fallback) ─────────────────────────

std::string Nl2Plan::buildFewShotExamplesHardcoded() const {
    return R"PROMPT(Examples:

Example 1:
Problem: "x and y are integers. x is at least 3. y is at most 10. Minimize x + y."
Plan:
{
  "version": "1",
  "logic_hint": "QF_LIA",
  "symbols": [
    {"name": "x", "unified_type": "int", "is_var": true},
    {"name": "y", "unified_type": "int", "is_var": true}
  ],
  "constraints": [
    {"expr": {"op": "ge", "args": [{"var": "x"}, {"lit": 3}]}},
    {"expr": {"op": "le", "args": [{"var": "y"}, {"lit": 10}]}}
  ],
  "objective": {"mode": "minimize", "expr": {"op": "int_add", "args": [{"var": "x"}, {"var": "y"}]}}
}

Example 2:
Problem: "a and b are boolean variables. If a is true then b must be false."
Plan:
{
  "version": "1",
  "logic_hint": "SAT",
  "symbols": [
    {"name": "a", "unified_type": "bool", "is_var": true},
    {"name": "b", "unified_type": "bool", "is_var": true}
  ],
  "constraints": [
    {"expr": {"op": "bool_implies", "args": [{"var": "a"}, {"op": "bool_not", "args": [{"var": "b"}]}]}}
  ]
}

Example 3:
Problem: "All elements in array A of size 5 must be different. Each element is an integer from 1 to 5."
Plan:
{
  "version": "1",
  "logic_hint": "CP",
  "symbols": [
    {"name": "A", "unified_type": "array(int)", "is_var": true, "domain": "1..5"}
  ],
  "constraints": [
    {"expr": {"op": "all_different", "args": [{"var": "A"}]}}
  ]
}

Example 4:
Problem: "p and q are real numbers. p is strictly greater than 0.5. q equals 2 times p. Maximize q."
Plan:
{
  "version": "1",
  "logic_hint": "QF_LRA",
  "symbols": [
    {"name": "p", "unified_type": "real", "is_var": true},
    {"name": "q", "unified_type": "real", "is_var": true}
  ],
  "constraints": [
    {"expr": {"op": "gt", "args": [{"var": "p"}, {"lit": 0.5}]}},
    {"expr": {"op": "eq", "args": [{"var": "q"}, {"op": "int_mul", "args": [{"lit": 2}, {"var": "p"}]}]}}
  ],
  "objective": {"mode": "maximize", "expr": {"var": "q"}}}
}

Example 5:
Problem: "n is a non-negative integer. m is an integer. m is exactly 3 more than n. Either n is even or m is greater than 10."
Plan:
{
  "version": "1",
  "logic_hint": "QF_LIA",
  "symbols": [
    {"name": "n", "unified_type": "int", "is_var": true},
    {"name": "m", "unified_type": "int", "is_var": true}
  ],
  "constraints": [
    {"expr": {"op": "ge", "args": [{"var": "n"}, {"lit": 0}]}},
    {"expr": {"op": "eq", "args": [{"var": "m"}, {"op": "int_add", "args": [{"var": "n"}, {"lit": 3}]}]}},
    {"expr": {"op": "bool_or", "args": [
      {"op": "eq", "args": [{"op": "int_mod", "args": [{"var": "n"}, {"lit": 2}]}, {"lit": 0}]},
      {"op": "gt", "args": [{"var": "m"}, {"lit": 10}]}
    ]}}
  ]
}
)PROMPT";
}

// ── Helpers ────────────────────────────────────────────────────────

void Nl2Plan::addError(const std::string& msg) {
    errors_.push_back(msg);
    if (config_.verbose) {
        std::cerr << "[Nl2Plan] Error: " << msg << "\n";
    }
}

} // namespace SOMTParser::Frontend::Natural
