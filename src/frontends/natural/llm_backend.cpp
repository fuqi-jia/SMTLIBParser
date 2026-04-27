/* -*- C++ -*-
 *
 * LLM Backend implementation — OpenAI-compatible HTTP + JSON extraction.
 */

#include "somtparser/frontends/natural/llm_backend.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>

namespace SOMTParser::Frontend::Natural {

// ── HTTP helpers ───────────────────────────────────────────────────

static size_t curlWriteCallback(void* contents, size_t size, size_t nmemb,
                                 std::string* userp) {
    userp->append(static_cast<char*>(contents), size * nmemb);
    return size * nmemb;
}

static std::string getenvOrDefault(const char* name, const std::string& fallback) {
    const char* val = std::getenv(name);
    return val ? val : fallback;
}

// ── OpenAI-compatible backend ──────────────────────────────────────

class OpenAiBackend : public LlmBackend {
public:
    OpenAiBackend(std::string api_base, std::string api_key,
                   std::string model, int timeout)
        : api_base_(std::move(api_base)),
          api_key_(std::move(api_key)),
          model_(std::move(model)),
          timeout_(timeout) {}

    std::string name() const override {
        return "OpenAiBackend(" + model_ + ")";
    }

    LlmResponse complete(const std::string& system_prompt,
                          const std::string& user_prompt) override {
        LlmResponse result;

        CURL* curl = curl_easy_init();
        if (!curl) {
            result.error_message = "curl_easy_init() failed";
            return result;
        }

        std::string url = api_base_;
        if (!url.empty() && url.back() == '/') url.pop_back();
        url += "/chat/completions";

        nlohmann::json body;
        body["model"] = model_;
        body["messages"] = nlohmann::json::array({
            {{"role", "system"}, {"content", system_prompt}},
            {{"role", "user"},   {"content", user_prompt}}
        });
        body["temperature"] = 0.2;  // low temperature for deterministic output
        body["max_tokens"] = 4096;

        std::string body_str = body.dump();
        std::string response_str;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        std::string auth_header;
        if (!api_key_.empty()) {
            auth_header = "Authorization: Bearer " + api_key_;
            headers = curl_slist_append(headers, auth_header.c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_str);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout_);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.http_status);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        result.raw_response = response_str;

        if (res != CURLE_OK) {
            result.error_message = std::string("curl error: ") + curl_easy_strerror(res);
            return result;
        }

        if (result.http_status == 429) {
            result.error_message = "Rate limited (HTTP 429)";
            result.retry_after = 5;  // conservative default
            return result;
        }

        if (result.http_status != 200) {
            result.error_message = "HTTP " + std::to_string(result.http_status);
            return result;
        }

        try {
            auto j = nlohmann::json::parse(response_str);
            if (j.contains("choices") && !j["choices"].empty()) {
                result.text = j["choices"][0]["message"]["content"].get<std::string>();
                result.success = true;
            } else if (j.contains("error")) {
                result.error_message = j["error"]["message"].get<std::string>();
            } else {
                result.error_message = "Unexpected response format";
            }
        } catch (const std::exception& e) {
            result.error_message = std::string("JSON parse error: ") + e.what();
        }

        return result;
    }

private:
    std::string api_base_;
    std::string api_key_;
    std::string model_;
    int timeout_;
};

// ── Python script backend ──────────────────────────────────────────

class PythonScriptBackend : public LlmBackend {
public:
    explicit PythonScriptBackend(std::string script_path)
        : script_path_(std::move(script_path)) {}

    std::string name() const override { return "PythonScriptBackend"; }

    LlmResponse complete(const std::string& system_prompt,
                          const std::string& user_prompt) override {
        LlmResponse result;
        result.error_message = "PythonScriptBackend not yet implemented";
        return result;
    }

private:
    std::string script_path_;
};

// ── Factory ────────────────────────────────────────────────────────

std::shared_ptr<LlmBackend> createOpenAiBackend(
    const std::string& api_base,
    const std::string& api_key,
    const std::string& model,
    int timeout_seconds) {
    return std::make_shared<OpenAiBackend>(api_base, api_key, model, timeout_seconds);
}

std::shared_ptr<LlmBackend> createPythonScriptBackend(
    const std::string& script_path) {
    return std::make_shared<PythonScriptBackend>(script_path);
}

std::shared_ptr<LlmBackend> createDefaultBackend() {
    // Priority 1: Python script
    std::string script = getenvOrDefault("NL2SMT_LLM_SCRIPT", "");
    if (!script.empty()) {
        return createPythonScriptBackend(script);
    }

    // Priority 2: OpenAI API key
    std::string openai_key = getenvOrDefault("OPENAI_API_KEY", "");
    if (!openai_key.empty()) {
        std::string model = getenvOrDefault("NL2SMT_MODEL", "gpt-4o-mini");
        std::string base = getenvOrDefault("OPENAI_API_BASE", "https://api.openai.com/v1");
        return createOpenAiBackend(base, openai_key, model, 60);
    }

    // Priority 3: Ollama
    std::string ollama_host = getenvOrDefault("OLLAMA_HOST", "http://localhost:11434");
    std::string model = getenvOrDefault("NL2SMT_MODEL", "llama3.1");
    return createOpenAiBackend(ollama_host + "/v1", "" /* no key needed */, model, 120);
}

// ── JSON extraction ────────────────────────────────────────────────

std::optional<std::string> extractJsonBlock(const std::string& text) {
    // Strategy 1: Look for ```json ... ``` fence
    size_t start = text.find("```json");
    if (start != std::string::npos) {
        start += 7;  // skip "```json"
        size_t end = text.find("```", start);
        if (end != std::string::npos) {
            // Trim leading/trailing whitespace/newlines
            std::string block = text.substr(start, end - start);
            size_t b = block.find_first_not_of(" \t\r\n");
            size_t e = block.find_last_not_of(" \t\r\n");
            if (b != std::string::npos && e != std::string::npos) {
                return block.substr(b, e - b + 1);
            }
        }
    }

    // Strategy 2: Look for ``` ... ``` fence (no json tag)
    start = text.find("```");
    if (start != std::string::npos) {
        start += 3;
        size_t end = text.find("```", start);
        if (end != std::string::npos) {
            std::string block = text.substr(start, end - start);
            size_t b = block.find_first_not_of(" \t\r\n");
            size_t e = block.find_last_not_of(" \t\r\n");
            if (b != std::string::npos && e != std::string::npos) {
                return block.substr(b, e - b + 1);
            }
        }
    }

    // Strategy 3: Find first '{' and last '}'
    size_t first_brace = text.find_first_of('{');
    size_t last_brace = text.find_last_of('}');
    if (first_brace != std::string::npos && last_brace != std::string::npos
        && first_brace < last_brace) {
        return text.substr(first_brace, last_brace - first_brace + 1);
    }

    return std::nullopt;
}

} // namespace SOMTParser::Frontend::Natural
