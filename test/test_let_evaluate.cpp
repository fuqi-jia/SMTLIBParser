#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "somtparser/frontend/parser.h"
#include "somtparser/ir/dag.h"
#include <cassert>

// NDEBUG-safe assertion
#define VERIFY(expr) do { if(!(expr)) { std::cerr << "VERIFY failed: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; std::abort(); } } while(0)

using namespace SOMTParser;

// Helper: collect variable names from a set of DAGNodes
static std::unordered_set<std::string> getVarNames(const std::unordered_set<std::shared_ptr<DAGNode>>& vars) {
    std::unordered_set<std::string> names;
    for (const auto& v : vars) names.insert(v->getName());
    return names;
}

// Helper: verify that a set of variable names exactly matches the expected set
static void expectVarSet(const std::unordered_set<std::string>& actual,
                         const std::unordered_set<std::string>& expected,
                         const char* testName) {
    if (actual != expected) {
        std::cerr << "FAIL [" << testName << "] expected { ";
        for (const auto& e : expected) std::cerr << e << " ";
        std::cerr << "} got { ";
        for (const auto& a : actual) std::cerr << a << " ";
        std::cerr << "}\n";
        std::abort();
    }
    std::cout << "  PASS " << testName << "\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase 1: Strict collectVars tests for let bindings
// ═══════════════════════════════════════════════════════════════════════════
void test_collectVars_strict() {
    std::cout << "=== test_collectVars_strict ===\n";
    ParserPtr p = newParser();

    // Declare free variables
    p->mkVarInt("x");
    p->mkVarInt("b");
    p->mkVarInt("d");
    p->mkVarInt("p");
    p->mkVarInt("q");
    p->mkVarInt("r");

    // 1. Single binding: (let ((y x)) y) → {x}
    {
        auto expr = p->mkExpr("(let ((y x)) y)");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"x"}, "single_binding");
    }

    // 2. Multi binding: (let ((a b) (c d)) (+ a c)) → {b, d}
    {
        auto expr = p->mkExpr("(let ((a b) (c d)) (+ a c))");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"b", "d"}, "multi_binding");
    }

    // 3. Nested let: (let ((y x)) (let ((z y)) z)) → {x}
    {
        auto expr = p->mkExpr("(let ((y x)) (let ((z y)) z))");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"x"}, "nested_let");
    }

    // 4. Let with compound expression: (let ((v (+ p q))) (+ v r)) → {p, q, r}
    {
        auto expr = p->mkExpr("(let ((v (+ p q))) (+ v r))");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"p", "q", "r"}, "compound_expr");
    }

    // 5. Shadowing: (let ((x 1)) x) → {}  (x is bound, not free)
    {
        auto expr = p->mkExpr("(let ((x 1)) x)");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {}, "shadowing_no_free");
    }

    // 6. Mixed free and bound: (let ((y x)) (+ y z)) → {x, z}
    // Note: z was not declared above. Let's declare it.
    p->mkVarInt("z");
    {
        auto expr = p->mkExpr("(let ((y x)) (+ y z))");
        VERIFY(expr && !expr->isErr());
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"x", "z"}, "mixed_free_and_bound");
    }

    std::cout << "test_collectVars_strict: all passed\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase 2: expandLet tests — verify structural expansion is correct
// ═══════════════════════════════════════════════════════════════════════════
void test_expandLet() {
    std::cout << "=== test_expandLet ===\n";
    ParserPtr p = newParser();

    // Declare variables used in expansions
    p->mkVarInt("x");
    p->mkVarInt("b");
    p->mkVarInt("d");

    // Ensure preserve-let mode is on (default)
    VERIFY(p->getOptions()->getKeepLet() == true);

    // 1. Single binding: (let ((y x)) y) → x
    {
        auto expr = p->mkExpr("(let ((y x)) y)");
        VERIFY(expr && (expr->isLet() || expr->isLetChain()));
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        std::string s = p->toString(expanded);
        std::cout << "  expand (let ((y x)) y) => " << s << "\n";
        VERIFY(s == "x");
    }

    // 2. Constant binding: (let ((a 5)) (+ a 3))
    //    expandLet replaces 'a' with 5, then mkOper folds (+ 5 3) → 8
    {
        auto expr = p->mkExpr("(let ((a 5)) (+ a 3))");
        VERIFY(expr && (expr->isLet() || expr->isLetChain()));
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        std::string s = p->toString(expanded);
        std::cout << "  expand (let ((a 5)) (+ a 3)) => " << s << "\n";
        VERIFY(s == "8");
    }

    // 3. Multi binding: (let ((a b) (c d)) (+ a c)) → (+ b d)
    {
        auto expr = p->mkExpr("(let ((a b) (c d)) (+ a c))");
        VERIFY(expr && (expr->isLet() || expr->isLetChain()));
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        std::string s = p->toString(expanded);
        std::cout << "  expand (let ((a b) (c d)) (+ a c)) => " << s << "\n";
        VERIFY(s == "(+ b d)");
    }

    // 4. Nested let: (let ((y x)) (let ((z y)) z)) → x
    {
        auto expr = p->mkExpr("(let ((y x)) (let ((z y)) z))");
        VERIFY(expr && (expr->isLet() || expr->isLetChain()));
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        std::string s = p->toString(expanded);
        std::cout << "  expand nested let => " << s << "\n";
        VERIFY(s == "x");
    }

    // 5. No-op expansion on non-let
    // Note: mkExpr may already fold constants, so (+ 1 2) could become 3.
    {
        auto expr = p->mkExpr("(+ x 2)");
        VERIFY(expr && !expr->isLet() && !expr->isLetChain());
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        std::string s = p->toString(expanded);
        VERIFY(s == "(+ x 2)");
    }

    std::cout << "test_expandLet: all passed\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase 3: evaluate let without explicit expansion (dispatch goes through
// evaluateLet → expandLet → evaluate body)
// ═══════════════════════════════════════════════════════════════════════════
void test_evaluate_let() {
    std::cout << "=== test_evaluate_let ===\n";
    ParserPtr p = newParser();
    ModelPtr m = newModel();

    // Declare variables and populate model
    auto var_x = p->mkVarInt("x");
    auto var_y = p->mkVarInt("y");
    auto var_z = p->mkVarInt("z");
    m->add(var_x, p->mkConstInt(10));
    m->add(var_y, p->mkConstInt(20));
    m->add(var_z, p->mkConstInt(30));

    // 1. (let ((a x)) a) with x=10 → 10
    {
        auto expr = p->mkExpr("(let ((a x)) a)");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((a x)) a) with x=10 => " << s << "\n";
        VERIFY(s == "10");
    }

    // 2. (let ((a x)) (+ a 5)) with x=10 → 15
    {
        auto expr = p->mkExpr("(let ((a x)) (+ a 5))");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((a x)) (+ a 5)) with x=10 => " << s << "\n";
        VERIFY(s == "15");
    }

    // 3. Multi-binding: (let ((a x) (b y)) (+ a b)) with x=10,y=20 → 30
    {
        auto expr = p->mkExpr("(let ((a x) (b y)) (+ a b))");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((a x) (b y)) (+ a b)) => " << s << "\n";
        VERIFY(s == "30");
    }

    // 4. Nested let: (let ((a x)) (let ((b a)) (+ b 1))) with x=10 → 11
    {
        auto expr = p->mkExpr("(let ((a x)) (let ((b a)) (+ b 1)))");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval nested let => " << s << "\n";
        VERIFY(s == "11");
    }

    // 5. Compound bound expression: (let ((a (+ x y))) (+ a z))
    //    x=10,y=20,z=30 → 10+20+30 = 60
    {
        auto expr = p->mkExpr("(let ((a (+ x y))) (+ a z))");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((a (+ x y))) (+ a z)) => " << s << "\n";
        VERIFY(s == "60");
    }

    // 6. Boolean let: (let ((p true)) (and p false)) → false
    {
        auto expr = p->mkExpr("(let ((p true)) (and p false))");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((p true)) (and p false)) => " << s << "\n";
        VERIFY(s == "false");
    }

    // 7. Unused binding: (let ((a x)) y) with x=10,y=20 → 20
    {
        auto expr = p->mkExpr("(let ((a x)) y)");
        VERIFY(expr && !expr->isErr());
        auto ev = p->evaluate(expr, m);
        VERIFY(ev);
        std::string s = p->toString(ev);
        std::cout << "  eval (let ((a x)) y) with y=20 => " << s << "\n";
        VERIFY(s == "20");
    }

    std::cout << "test_evaluate_let: all passed\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase 4: expand-then-evaluate consistency test
// Ensure expandLet + evaluate produces the same result as direct evaluate
// ═══════════════════════════════════════════════════════════════════════════
void test_expand_then_evaluate() {
    std::cout << "=== test_expand_then_evaluate ===\n";
    ParserPtr p = newParser();
    ModelPtr m = newModel();

    auto var_x2 = p->mkVarInt("x");
    auto var_y2 = p->mkVarInt("y");
    m->add(var_x2, p->mkConstInt(7));
    m->add(var_y2, p->mkConstInt(3));

    auto testExpr = [&](const char* smt2, const char* expected) {
        auto expr = p->mkExpr(smt2);
        VERIFY(expr && !expr->isErr());

        // Path A: direct evaluate (goes through evaluateLet → expandLet internally)
        auto ev_direct = p->evaluate(expr, m);
        VERIFY(ev_direct);
        std::string s_direct = p->toString(ev_direct);

        // Path B: manual expand then evaluate
        auto expanded = p->expandLet(expr);
        VERIFY(expanded);
        auto ev_expanded = p->evaluate(expanded, m);
        VERIFY(ev_expanded);
        std::string s_expanded = p->toString(ev_expanded);

        if (s_direct != expected || s_expanded != expected || s_direct != s_expanded) {
            std::cerr << "FAIL [" << smt2 << "]\n"
                      << "  direct=" << s_direct << " expanded=" << s_expanded
                      << " expected=" << expected << "\n";
            std::abort();
        }
        std::cout << "  [" << smt2 << "] => " << s_direct << "\n";
    };

    testExpr("(let ((a x)) (+ a y))", "10");      // 7+3
    testExpr("(let ((a (+ x y))) (* a 2))", "20"); // (7+3)*2
    testExpr("(let ((a x)) (let ((b a)) (- b y)))", "4"); // 7-3
    testExpr("(let ((p (> x y))) (and p true))", "true");  // 7>3

    std::cout << "test_expand_then_evaluate: all passed\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Phase 5: collectVars on already-expanded let (sanity check)
// After expandLet, there should be no let-bind vars left in the tree.
// ═══════════════════════════════════════════════════════════════════════════
void test_collectVars_after_expand() {
    std::cout << "=== test_collectVars_after_expand ===\n";
    ParserPtr p = newParser();
    p->mkVarInt("x");
    p->mkVarInt("y");

    auto expr = p->mkExpr("(let ((a x) (b y)) (+ a b))");
    VERIFY(expr && (expr->isLet() || expr->isLetChain()));

    // Before expand: collectVars should find {x, y}
    {
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expr, vars);
        expectVarSet(getVarNames(vars), {"x", "y"}, "before_expand");
    }

    // After expand: collectVars should still find {x, y} (let-bind vars gone)
    auto expanded = p->expandLet(expr);
    VERIFY(expanded);
    {
        std::unordered_set<std::shared_ptr<DAGNode>> vars;
        p->collectVars(expanded, vars);
        expectVarSet(getVarNames(vars), {"x", "y"}, "after_expand");
    }

    // Verify the expanded tree contains no let-bind vars
    {
        std::unordered_set<std::shared_ptr<DAGNode>> visited;
        std::function<void(std::shared_ptr<DAGNode>)> checkNoLetBind = [&](std::shared_ptr<DAGNode> node) {
            if (!node || visited.find(node) != visited.end()) return;
            visited.insert(node);
            VERIFY(!node->isLetBindVar());
            for (size_t i = 0; i < node->getChildrenSize(); i++) {
                checkNoLetBind(node->getChild(i));
            }
        };
        checkNoLetBind(expanded);
    }

    std::cout << "test_collectVars_after_expand: all passed\n\n";
}

// ═══════════════════════════════════════════════════════════════════════════
// Main
// ═══════════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "======= Let Evaluate & Expand Test =======\n\n";

    test_collectVars_strict();
    test_expandLet();
    test_evaluate_let();
    test_expand_then_evaluate();
    test_collectVars_after_expand();

    std::cout << "All let tests passed!\n";
    return 0;
}
