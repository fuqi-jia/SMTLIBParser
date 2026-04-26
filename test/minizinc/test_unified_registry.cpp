/* -*- C++ -*-
 * Test: Unified Op Registry
 */

#include "somtparser/unified/unified_op_registry.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <sstream>

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
        "../config/unified_ops.json",        // from build/
        "../../config/unified_ops.json",     // from build/test/
        "../../../config/unified_ops.json",  // from build/test/subdir
        "config/unified_ops.json"            // from project root
    };
    for (const auto& p : candidates) {
        if (std::ifstream(p).good()) return p;
    }
    return "";
}

TEST(load_json) {
    auto path = findConfigPath();
    ASSERT_TRUE(!path.empty());
    UnifiedOpRegistry reg;
    bool ok = reg.loadFromFile(path);
    ASSERT_TRUE(ok);
    ASSERT_TRUE(reg.size() > 0);
}

TEST(lookup_by_unified_name) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    
    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());
    const auto* def = reg.getDef(ref);
    ASSERT_TRUE(def != nullptr);
    ASSERT_EQ(def->unified_name, "int_add");
    ASSERT_EQ(def->category, "arith");
    ASSERT_TRUE(def->is_commutative);
}

TEST(lookup_by_lang_name) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    
    // MiniZinc name
    auto ref_mzn = reg.lookupByLangName("minizinc", "+");
    ASSERT_TRUE(ref_mzn.valid());
    ASSERT_EQ(reg.getDef(ref_mzn)->unified_name, "int_add");
    
    // SMT-LIB name
    auto ref_smt = reg.lookupByLangName("smtlib", "+");
    ASSERT_TRUE(ref_smt.valid());
    ASSERT_EQ(reg.getDef(ref_smt)->unified_name, "int_add");
    
    // Same op from different languages
    ASSERT_EQ(ref_mzn.id, ref_smt.id);
}

TEST(smt_lowering_native) {
    UnifiedOpRegistry reg;
    bool ok = reg.loadFromFile(findConfigPath());
    ASSERT_TRUE(ok);
    ASSERT_TRUE(reg.size() > 0);
    
    auto ref = reg.lookupByUnifiedName("int_add");
    ASSERT_TRUE(ref.valid());
    const auto* def = reg.getDef(ref);
    ASSERT_TRUE(def != nullptr);
    ASSERT_TRUE(def->smt_lowering.strategy == SmtLoweringDef::Strategy::NATIVE);
    ASSERT_EQ(def->smt_lowering.native_smt_name, "+");
    ASSERT_TRUE(reg.hasSmtLowering("int_add"));
}

TEST(smt_lowering_decompose) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    
    auto ref = reg.lookupByUnifiedName("all_different");
    const auto* def = reg.getDef(ref);
    ASSERT_TRUE(def->smt_lowering.strategy == SmtLoweringDef::Strategy::DECOMPOSE);
    ASSERT_TRUE(!def->smt_lowering.decomposition_template.empty());
    ASSERT_TRUE(reg.hasSmtLowering("all_different"));
}

TEST(global_cp_ops) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    
    auto globals = reg.listOpsByCategory("global_cp");
    ASSERT_TRUE(globals.size() >= 5);  // all_different, all_equal, element, increasing, etc.
    
    auto ref = reg.lookupByUnifiedName("all_different");
    ASSERT_TRUE(ref.valid());
    const auto* def = reg.getDef(ref);
    ASSERT_EQ(def->lang_names.at("minizinc"), "all_different");
    ASSERT_EQ(def->lang_names.at("flatzinc"), "all_different_int");
}

TEST(save_roundtrip) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    size_t before = reg.size();
    
    bool ok = reg.saveToFile("/tmp/test_unified_ops.json");
    ASSERT_TRUE(ok);
    
    UnifiedOpRegistry reg2;
    reg2.loadFromFile("/tmp/test_unified_ops.json");
    ASSERT_EQ(reg2.size(), before);
    
    auto ref = reg2.lookupByUnifiedName("all_different");
    ASSERT_TRUE(ref.valid());
}

TEST(list_categories) {
    UnifiedOpRegistry reg;
    reg.loadFromFile(findConfigPath());
    auto cats = reg.categories();
    ASSERT_TRUE(!cats.empty());
}

int main() {
    std::cout << "======= Unified Op Registry Tests =======\n\n";
    RUN_TEST(load_json);
    RUN_TEST(lookup_by_unified_name);
    RUN_TEST(lookup_by_lang_name);
    RUN_TEST(smt_lowering_native);
    RUN_TEST(smt_lowering_decompose);
    RUN_TEST(global_cp_ops);
    RUN_TEST(save_roundtrip);
    RUN_TEST(list_categories);
    
    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    return tests_failed > 0 ? 1 : 0;
}
