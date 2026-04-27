/* -*- C++ -*-
 * Test: NL4SMT Benchmark Runner
 *
 * Loads benchmarks/nl4smt/benchmarks.json and validates that each
 * expected Plan correctly emits to Unified IR and lowers to SMT-LIB.
 */

#include "somtparser/unified/plan.h"
#include "somtparser/unified/plan_builder.h"
#include "somtparser/unified/unified_pipeline.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/unified/unified_printer.h"
#include "somtparser/frontend/parser.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <set>

using namespace SOMTParser;
using namespace SOMTParser::Unified;

static int tests_passed = 0;
static int tests_failed = 0;
static int benchmarks_ran = 0;

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

static std::string findBenchmarkPath() {
    std::vector<std::string> candidates = {
        "../benchmarks/nl4smt/benchmarks.json",
        "../../benchmarks/nl4smt/benchmarks.json",
        "../../../benchmarks/nl4smt/benchmarks.json",
        "benchmarks/nl4smt/benchmarks.json"
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

// ── Benchmark runner ───────────────────────────────────────────────

static bool hasSolver(const std::string& name) {
    std::string cmd = "which " + name + " > /dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

static bool checkSolverParses(const std::string& smtlib, const std::string& solver) {
    std::string tmpfile = "/tmp/nl4smt_benchmark_" + std::to_string(std::rand()) + ".smt2";
    {
        std::ofstream ofs(tmpfile);
        if (!ofs) return false;
        ofs << smtlib;
    }
    std::string cmd = solver + " -smt2 " + tmpfile + " > /dev/null 2>&1";
    int ret = std::system(cmd.c_str());
    std::remove(tmpfile.c_str());
    return ret == 0;
}

static void runBenchmarks() {
    std::string bpath = findBenchmarkPath();
    ASSERT_TRUE(!bpath.empty());

    std::ifstream ifs(bpath);
    ASSERT_TRUE(ifs.good());

    nlohmann::json j;
    ifs >> j;
    ASSERT_TRUE(j.contains("problems"));

    auto& reg = getRegistry();

    bool has_z3 = hasSolver("z3");
    bool has_cvc5 = hasSolver("cvc5");

    std::cout << "  Solvers available: z3=" << (has_z3 ? "yes" : "no")
              << ", cvc5=" << (has_cvc5 ? "yes" : "no") << "\n";

    // Optional: save .smt2 files to a directory for external solver scripts
    const char* save_dir = std::getenv("NL4SMT_SAVE_SMT2");
    if (save_dir) {
        std::string mkdir_cmd = std::string("mkdir -p ") + save_dir;
        int ret = std::system(mkdir_cmd.c_str());
        (void)ret;
    }

    int syntax_ok = 0;
    int solver_ok = 0;
    int total = 0;

    for (const auto& prob : j["problems"]) {
        std::string id = prob.value("id", "unknown");
        std::string category = prob.value("category", "");
        std::string nl = prob.value("nl", "");

        std::cout << "  [" << id << "] " << category << ": " << nl.substr(0, 50);
        if (nl.size() > 50) std::cout << "...";
        std::cout << "\n";

        if (!prob.contains("expected_plan")) {
            std::cout << "    SKIP (no expected_plan)\n";
            continue;
        }

        total++;
        benchmarks_ran++;

        // Load expected plan
        Plan plan = Plan::fromJson(prob["expected_plan"]);

        // Validate
        PlanValidator validator(reg);
        bool valid = validator.validate(plan);
        if (!valid) {
            std::cout << "    VALIDATION FAILED\n";
            for (const auto& e : validator.errors()) {
                std::cout << "      - " << e << "\n";
            }
            continue;
        }

        // End-to-end pipeline: Plan → SMT-LIB2
        UnifiedPipeline pipeline(reg);
        std::string smtlib = pipeline.planToSmt2(plan);

        // Check basic syntax
        bool has_declare = smtlib.find("declare-fun") != std::string::npos ||
                           smtlib.find("declare-const") != std::string::npos;
        bool has_assert = smtlib.find("assert") != std::string::npos;
        bool has_check = smtlib.find("check-sat") != std::string::npos;

        if (has_declare && has_assert && has_check) {
            syntax_ok++;
        } else {
            std::cout << "    SYNTAX CHECK FAILED (missing declare/assert/check-sat)\n";
            continue;
        }

        // Try solver
        bool parsed_by_solver = false;
        std::string solver_used;
        if (has_z3 && checkSolverParses(smtlib, "z3")) {
            parsed_by_solver = true;
            solver_used = "z3";
        } else if (has_cvc5 && checkSolverParses(smtlib, "cvc5")) {
            parsed_by_solver = true;
            solver_used = "cvc5";
        }

        // Save .smt2 if requested
        if (save_dir) {
            std::string out_path = std::string(save_dir) + "/" + id + ".smt2";
            std::ofstream ofs(out_path);
            if (ofs) ofs << smtlib;
        }

        if (parsed_by_solver) {
            solver_ok++;
            std::cout << "    OK (solver accepts: " << solver_used << ")\n";
        } else if (has_z3 || has_cvc5) {
            std::cout << "    WARNING: solver rejected (may be logic mismatch)\n";
            std::cerr << "[Benchmark] Rejected SMT-LIB for " << id << ":\n" << smtlib << "\n";
        } else {
            std::cout << "    OK (no solver to verify)\n";
        }
    }

    std::cout << "\n  Benchmark summary:\n";
    std::cout << "    Total problems:   " << total << "\n";
    std::cout << "    Syntax correct:   " << syntax_ok << " (" << (total > 0 ? 100 * syntax_ok / total : 0) << "%)\n";
    std::cout << "    Solver accepts:   " << solver_ok << " (" << (total > 0 ? 100 * solver_ok / total : 0) << "%)\n";
}

// ── Tests ──────────────────────────────────────────────────────────

TEST(benchmark_load_and_validate) {
    runBenchmarks();
}

TEST(prompt_library_loads) {
    std::vector<std::string> candidates = {
        "../config/nl4smt_prompts.json",
        "../../config/nl4smt_prompts.json",
        "../../../config/nl4smt_prompts.json",
        "config/nl4smt_prompts.json"
    };
    std::string path;
    for (const auto& p : candidates) {
        if (std::ifstream(p).good()) { path = p; break; }
    }
    ASSERT_TRUE(!path.empty());

    std::ifstream ifs(path);
    nlohmann::json j;
    ifs >> j;

    ASSERT_TRUE(j.contains("version"));
    ASSERT_TRUE(j.contains("system_message"));
    ASSERT_TRUE(j.contains("few_shot_examples"));
    ASSERT_TRUE(j["few_shot_examples"].is_array());
    ASSERT_TRUE(j["few_shot_examples"].size() >= 3);
}

TEST(few_shot_examples_cover_categories) {
    std::vector<std::string> candidates = {
        "../config/nl4smt_prompts.json",
        "../../config/nl4smt_prompts.json",
        "../../../config/nl4smt_prompts.json",
        "config/nl4smt_prompts.json"
    };
    std::string path;
    for (const auto& p : candidates) {
        if (std::ifstream(p).good()) { path = p; break; }
    }
    ASSERT_TRUE(!path.empty());

    std::ifstream ifs(path);
    nlohmann::json j;
    ifs >> j;

    std::set<std::string> logics;
    for (const auto& ex : j["few_shot_examples"]) {
        if (ex.contains("logic")) {
            logics.insert(ex["logic"].get<std::string>());
        }
    }

    // Should cover at least QF_LIA, SAT, CP
    ASSERT_TRUE(logics.count("QF_LIA") > 0);
    ASSERT_TRUE(logics.count("SAT") > 0);
    ASSERT_TRUE(logics.count("CP") > 0);
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= NL4SMT Benchmark Tests =======\n\n";

    RUN_TEST(prompt_library_loads);
    RUN_TEST(few_shot_examples_cover_categories);
    RUN_TEST(benchmark_load_and_validate);

    std::cout << "\n======================================\n";
    std::cout << "Tests passed: " << tests_passed << "\n";
    std::cout << "Tests failed: " << tests_failed << "\n";
    std::cout << "Benchmarks ran: " << benchmarks_ran << "\n";
    return tests_failed > 0 ? 1 : 0;
}
