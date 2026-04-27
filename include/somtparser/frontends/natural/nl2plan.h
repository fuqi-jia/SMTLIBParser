/* -*- Header -*-
 *
 * NL → Plan pipeline for NL4SMT.
 *
 * 1. Load prompt template (from config/nl4smt_prompts.json)
 * 2. Build prompt with op catalog + few-shot examples + user NL
 * 3. Call LLM backend
 * 4. Extract JSON Plan from response
 * 5. Validate Plan; on failure, retry with error feedback (1-2 rounds)
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef NL2PLAN_H
#define NL2PLAN_H

#include "somtparser/unified/plan.h"
#include "somtparser/frontends/natural/llm_backend.h"
#include "somtparser/unified/unified_op_registry.h"

// Forward declaration (defined in nl_frontend.h)
namespace SOMTParser::Unified { class PlanValidator; }

#include <memory>
#include <string>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>

namespace SOMTParser::Frontend::Natural {

// ── Nl2Plan configuration ──────────────────────────────────────────

struct Nl2PlanConfig {
    std::string prompt_template_path = "config/nl4smt_prompts.json";
    std::shared_ptr<LlmBackend> backend;  // if null, uses createDefaultBackend()
    int max_retries = 2;                  // validation-feedback retries
    bool verbose = false;
    // Ablation switches
    bool use_few_shot = true;             // include few-shot examples in prompt
    bool use_retry = true;                // enable validation-feedback retry loop
    bool use_op_catalog = true;           // include operator catalog in prompt
};

// ── NL → Plan converter ────────────────────────────────────────────

class Nl2Plan {
public:
    explicit Nl2Plan(const Unified::UnifiedOpRegistry& registry,
                      Nl2PlanConfig config = {});

    /** Convert natural language description to a validated Plan.
     *  May throw std::runtime_error on LLM failure or persistent validation errors.
     */
    Unified::Plan convert(const std::string& nl_text);

    /** Access accumulated errors (non-fatal, includes retry attempts). */
    const std::vector<std::string>& errors() const { return errors_; }

    /** Access the raw LLM response from the last call (for debugging). */
    const std::string& lastRawResponse() const { return last_raw_response_; }

private:
    const Unified::UnifiedOpRegistry& registry_;
    Nl2PlanConfig config_;
    std::shared_ptr<LlmBackend> backend_;
    std::vector<std::string> errors_;
    std::string last_raw_response_;

    // Prompt building
    std::string buildSystemPrompt() const;
    std::string buildUserPrompt(const std::string& nl_text) const;
    std::string buildRetryPrompt(const std::string& nl_text,
                                  const std::string& last_plan_json,
                                  const std::vector<std::string>& validation_errors) const;

    // Prompt library (loaded from config/nl4smt_prompts.json)
    struct PromptLibrary {
        std::string version;
        std::string system_message;
        std::vector<std::string> output_rules;
        nlohmann::json few_shot_examples;   // array of example objects
        nlohmann::json schema_description;
        bool loaded = false;
    };
    mutable PromptLibrary prompt_lib_;  // lazy-loaded

    // Helpers
    void addError(const std::string& msg);
    const PromptLibrary& loadPromptLibrary() const;  // lazy load from config
    std::string buildOpCatalog() const;
    std::string buildFewShotExamplesFromLibrary() const;
    std::string buildFewShotExamplesHardcoded() const;
};

} // namespace SOMTParser::Frontend::Natural

#endif // NL2PLAN_H
