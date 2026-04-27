/* -*- Header -*-
 *
 * LLM Backend Abstraction for NL4SMT.
 *
 * Provides a unified interface for calling LLM APIs.
 * Concrete implementations:
 *   - OpenAiBackend: HTTP POST to OpenAI-compatible API (OpenAI, Ollama, etc.)
 *   - PythonScriptBackend: delegates to external Python script (e.g. LiteLLM)
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef LLM_BACKEND_H
#define LLM_BACKEND_H

#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace SOMTParser::Frontend::Natural {

// ── LLM response ───────────────────────────────────────────────────

struct LlmResponse {
    bool success = false;
    std::string text;           // generated text (may contain markdown fences)
    std::string raw_response;   // full HTTP response body (for debugging)
    std::string error_message;
    int http_status = 0;
    int retry_after = 0;        // seconds to wait before retry (from 429)
};

// ── Abstract LLM backend ───────────────────────────────────────────

class LlmBackend {
public:
    virtual ~LlmBackend() = default;

    /** Send a prompt and return the LLM's response. */
    virtual LlmResponse complete(const std::string& system_prompt,
                                  const std::string& user_prompt) = 0;

    /** Backend name for logging. */
    virtual std::string name() const = 0;
};

// ── Factory ────────────────────────────────────────────────────────

/** Create a backend from environment / config.
 *
 *  Priority:
 *    1. If NL2SMT_LLM_SCRIPT is set → PythonScriptBackend
 *    2. If OPENAI_API_KEY is set    → OpenAiBackend (OpenAI)
 *    3. If OLLAMA_HOST is set       → OpenAiBackend (Ollama)
 *    4. Fallback: OpenAiBackend with base_url=http://localhost:11434/v1
 *       (assumes Ollama is running locally)
 */
std::shared_ptr<LlmBackend> createDefaultBackend();

/** Create an OpenAI-compatible backend directly. */
std::shared_ptr<LlmBackend> createOpenAiBackend(
    const std::string& api_base,
    const std::string& api_key,
    const std::string& model,
    int timeout_seconds = 60);

/** Create a backend that delegates to a Python script. */
std::shared_ptr<LlmBackend> createPythonScriptBackend(
    const std::string& script_path);

// ── Utility: JSON extraction ───────────────────────────────────────

/** Extract a JSON object from LLM text that may contain markdown fences,
 *  explanations, or extra whitespace.
 */
std::optional<std::string> extractJsonBlock(const std::string& text);

} // namespace SOMTParser::Frontend::Natural

#endif // LLM_BACKEND_H
