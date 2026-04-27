/* -*- C++ -*-
 * Test: Natural Language Frontend
 */

#include "somtparser/frontends/natural/nl_frontend.h"
#include "somtparser/unified/plan.h"
#include "somtparser/unified/plan_builder.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/unified/unified_printer.h"

#include <iostream>
#include <fstream>
#include <sstream>

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

// ── Tests ──────────────────────────────────────────────────────────

TEST(plan_construction) {
    Plan plan;
    plan.version = "1";
    plan.logic_hint = "QF_LIA";
    plan.symbols.push_back(PlanSymbol::mkInt("x", "1..10"));
    plan.constraints.push_back(
        PlanConstraint::mkOp("lt", std::vector<nlohmann::json>{
            PlanConstraint::mkVar("x").expr,
            PlanConstraint::mkLit(static_cast<int64_t>(10)).expr
        })
    );

    ASSERT_EQ(plan.symbols.size(), 1u);
    ASSERT_EQ(plan.constraints.size(), 1u);
}

TEST(plan_json_roundtrip) {
    Plan plan;
    plan.symbols.push_back(PlanSymbol::mkInt("x"));
    std::vector<nlohmann::json> eq_args;
    eq_args.push_back(PlanConstraint::mkVar("x").expr);
    eq_args.push_back(PlanConstraint::mkLit(static_cast<int64_t>(5)).expr);
    plan.constraints.push_back(PlanConstraint::mkOp("eq", eq_args));

    auto j = plan.toJson();
    auto plan2 = Plan::fromJson(j);

    ASSERT_EQ(plan2.symbols.size(), 1u);
    ASSERT_EQ(plan2.constraints.size(), 1u);
}

TEST(plan_validator) {
    auto& reg = getRegistry();
    PlanValidator validator(reg);

    Plan plan;
    plan.symbols.push_back(PlanSymbol::mkInt("x"));
    std::vector<nlohmann::json> lt_args;
    lt_args.push_back(PlanConstraint::mkVar("x").expr);
    lt_args.push_back(PlanConstraint::mkLit(static_cast<int64_t>(10)).expr);
    plan.constraints.push_back(PlanConstraint::mkOp("lt", lt_args));

    ASSERT_TRUE(validator.validate(plan));
    ASSERT_TRUE(validator.errors().empty());
}

TEST(plan_validator_unknown_op) {
    auto& reg = getRegistry();
    PlanValidator validator(reg);

    Plan plan;
    plan.constraints.push_back(PlanConstraint::mkOp("nonexistent_op", {}));

    ASSERT_TRUE(!validator.validate(plan));
    ASSERT_TRUE(!validator.errors().empty());
}

TEST(plan_emitter) {
    auto& reg = getRegistry();
    PlanEmitter emitter(reg);

    Plan plan;
    plan.symbols.push_back(PlanSymbol::mkInt("x"));
    plan.constraints.push_back(PlanConstraint::mkOp("lt", std::vector<nlohmann::json>{
        PlanConstraint::mkVar("x").expr,
        PlanConstraint::mkLit(static_cast<int64_t>(10)).expr
    }));

    auto model = emitter.emit(plan);
    ASSERT_EQ(model.vars.size(), 1u);
    ASSERT_EQ(model.constraints.size(), 1u);
}

TEST(nl_frontend_end_to_end) {
    auto& reg = getRegistry();
    Parser parser;
    NaturalFrontend nl(parser, reg);

    Plan plan;
    plan.symbols.push_back(PlanSymbol::mkInt("x"));
    std::vector<nlohmann::json> gt_args;
    gt_args.push_back(PlanConstraint::mkVar("x").expr);
    gt_args.push_back(PlanConstraint::mkLit(static_cast<int64_t>(0)).expr);
    plan.constraints.push_back(PlanConstraint::mkOp("gt", gt_args));
    plan.objective = PlanObjective{"minimize", PlanConstraint::mkVar("x").expr};

    auto model = nl.parsePlan(plan);

    UnifiedPrinter printer(reg);
    std::string mzn = printer.toMiniZinc(model);
    ASSERT_TRUE(mzn.find("var int: x;") != std::string::npos);
    ASSERT_TRUE(mzn.find("constraint") != std::string::npos);
    ASSERT_TRUE(mzn.find("solve minimize") != std::string::npos);
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= NL Frontend Tests =======\n\n";

    RUN_TEST(plan_construction);
    RUN_TEST(plan_json_roundtrip);
    RUN_TEST(plan_validator);
    RUN_TEST(plan_validator_unknown_op);
    RUN_TEST(plan_emitter);
    RUN_TEST(nl_frontend_end_to_end);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
