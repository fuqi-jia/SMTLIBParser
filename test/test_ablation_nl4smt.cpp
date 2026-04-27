/* -*- C++ -*-
 * Test: NL4SMT Ablation Study Framework
 *
 * Validates that Nl2Plan ablation switches (few-shot, retry, op-catalog)
 * correctly affect prompt generation and conversion behavior.
 */

#include "somtparser/frontends/natural/nl2plan.h"
#include "somtparser/unified/plan.h"
#include "somtparser/unified/plan_builder.h"
#include "somtparser/unified/unified_pipeline.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/frontend/parser.h"

#include <iostream>
#include <fstream>

using namespace SOMTParser;
using namespace SOMTParser::Frontend::Natural;
using namespace SOMTParser::Unified;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  Running " #name "... "; \
    try { test_##name(); tests_passed++; std::cout << "OK\n"; } \
    catch (const std::exception& e) { tests_failed++; std::cout << "FAILED: " << e.what() << "\n"; } \
} while(0)

#define ASSERT_TRUE(x) do { if (!(x)) { throw std::runtime_error("Assertion failed: " #x); } } while(0)
#define ASSERT_EQ(a, b) do { if ((a) != (b)) { std::ostringstream oss; oss << "Assertion failed: " #a " == " #b " (got " << (a) << " vs " << (b) << ")"; throw std::runtime_error(oss.str()); } } while(0)

static std::string findConfigPath() {
    std::vector<std::string> candidates = {
        "../config/unified_ops.json",
        "../../config/unified_ops.json",
        "../../../config/unified_ops.json",
        "config/unified_ops.json"
    };
    for (const auto& p : candidates) {
        if (std::ifstream(p).good()) return p;
    }
    return "";
}

static UnifiedOpRegistry& getRegistry() {
    static UnifiedOpRegistry reg;
    static bool loaded = false;
    if (!loaded) {
        std::string path = findConfigPath();
        if (!path.empty()) {
            bool ok = reg.loadFromFile(path);
            (void)ok;
        }
        loaded = true;
    }
    return reg;
}

// ── Mock backend that records the prompt ───────────────────────────

class RecordingBackend : public LlmBackend {
public:
    std::string last_system;
    std::string last_user;
    std::string fixed_response;

    explicit RecordingBackend(std::string response)
        : fixed_response(std::move(response)) {}

    std::string name() const override { return "RecordingBackend"; }

    LlmResponse complete(const std::string& system, const std::string& user) override {
        last_system = system;
        last_user = user;
        LlmResponse r;
        r.success = true;
        r.text = fixed_response;
        return r;
    }
};

// ── Tests ──────────────────────────────────────────────────────────

TEST(few_shot_toggle) {
    auto& reg = getRegistry();

    // With few-shot
    {
        auto backend = std::make_shared<RecordingBackend>(
            "```json\n{\"version\":\"1\",\"symbols\":[],\"constraints\":[]}\n```");
        Nl2PlanConfig cfg;
        cfg.backend = backend;
        cfg.use_few_shot = true;
        Nl2Plan nl2plan(reg, cfg);
        try { nl2plan.convert("test"); } catch (...) {}
        ASSERT_TRUE(backend->last_system.find("Example") != std::string::npos);
    }

    // Without few-shot
    {
        auto backend = std::make_shared<RecordingBackend>(
            "```json\n{\"version\":\"1\",\"symbols\":[],\"constraints\":[]}\n```");
        Nl2PlanConfig cfg;
        cfg.backend = backend;
        cfg.use_few_shot = false;
        Nl2Plan nl2plan(reg, cfg);
        try { nl2plan.convert("test"); } catch (...) {}
        ASSERT_TRUE(backend->last_system.find("Example") == std::string::npos);
    }
}

TEST(op_catalog_toggle) {
    auto& reg = getRegistry();

    // With catalog
    {
        auto backend = std::make_shared<RecordingBackend>(
            "```json\n{\"version\":\"1\",\"symbols\":[],\"constraints\":[]}\n```");
        Nl2PlanConfig cfg;
        cfg.backend = backend;
        cfg.use_op_catalog = true;
        Nl2Plan nl2plan(reg, cfg);
        try { nl2plan.convert("test"); } catch (...) {}
        ASSERT_TRUE(backend->last_system.find("bool_and") != std::string::npos);
    }

    // Without catalog
    {
        auto backend = std::make_shared<RecordingBackend>(
            "```json\n{\"version\":\"1\",\"symbols\":[],\"constraints\":[]}\n```");
        Nl2PlanConfig cfg;
        cfg.backend = backend;
        cfg.use_op_catalog = false;
        Nl2Plan nl2plan(reg, cfg);
        try { nl2plan.convert("test"); } catch (...) {}
        ASSERT_TRUE(backend->last_system.find("bool_and") == std::string::npos);
    }
}

TEST(retry_toggle) {
    auto& reg = getRegistry();

    // Mock backend returns invalid JSON that fails validation
    auto backend = std::make_shared<RecordingBackend>(
        "```json\n{\"version\":\"1\",\"constraints\":[{\"op\":\"nonexistent\"}]}\n```");

    // With retry: should attempt multiple calls
    {
        Nl2PlanConfig cfg;
        cfg.backend = backend;
        cfg.use_retry = true;
        cfg.max_retries = 2;
        Nl2Plan nl2plan(reg, cfg);
        int calls_before = 0;
        try { nl2plan.convert("test"); } catch (...) {}
        // The backend was called at least twice (initial + retry)
        ASSERT_TRUE(backend->last_user.find("validation errors") != std::string::npos ||
                    backend->last_user.find("validation") != std::string::npos);
    }

    // Without retry: should fail fast
    {
        auto backend2 = std::make_shared<RecordingBackend>(
            "```json\n{\"version\":\"1\",\"constraints\":[{\"op\":\"nonexistent\"}]}\n```");
        Nl2PlanConfig cfg;
        cfg.backend = backend2;
        cfg.use_retry = false;
        Nl2Plan nl2plan(reg, cfg);
        try { nl2plan.convert("test"); } catch (...) {}
        // Should not contain retry-specific text
        ASSERT_TRUE(backend2->last_user.find("validation errors") == std::string::npos);
    }
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= NL4SMT Ablation Tests =======\n\n";

    RUN_TEST(few_shot_toggle);
    RUN_TEST(op_catalog_toggle);
    RUN_TEST(retry_toggle);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
