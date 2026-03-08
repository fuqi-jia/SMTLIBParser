/**
 * Test Rewriter: fixpoint rewrite with default NOT/AND/ADD rules.
 * Parses (assert (and true (not (not p)))), rewrites, asserts result == p.
 */
#include "somtparser/frontend/parser.h"
#include "somtparser/passes/rewriter.h"
#include <cassert>
#include <iostream>

using namespace SOMTParser;

int main() {
    std::cout << "======= Rewriter (fixpoint) Test =======" << std::endl;

    Parser parser;
    parser.parseStr("(declare-const p Bool)");
    bool ok = parser.parseStr("(assert (and true (not (not p))))");
    assert(ok);

    Node root = parser.getAssertions().back();
    assert(kind(root) == NODE_KIND::NT_AND);
    assert(numChildren(root) >= 2);
    Node origVar = child(child(child(root, 1), 0), 0);  // p inside (not (not p))
    assert(origVar && kind(origVar) == NODE_KIND::NT_VAR);

    Rewriter rewriter(parser.getNodeManager());
    installDefaultRewriteRules(rewriter);

    Node result = rewriter.rewrite(root);  // default fixpoint true
    assert(result && "rewrite should return non-null");
    assert(result == origVar && "fixpoint: (and true (not (not p))) -> p");
    std::cout << "fixpoint (and true (not (not p))) -> p OK" << std::endl;

    // rewriteOnce alone may not reach p (depends on order); one round: (not (not p)) -> p, then (and true p) -> p next round
    parser.parseStr("(declare-const q Bool)");
    ok = parser.parseStr("(assert (and true (not (not q))))");
    assert(ok);
    Node root2 = parser.getAssertions().back();
    Node once1 = rewriter.rewriteOnce(root2);
    Node fixed = rewriter.rewrite(root2, true);
    Node expectQ = child(child(child(root2, 1), 0), 0);
    assert(fixed == expectQ && "fixpoint (and true (not (not q))) -> q");
    std::cout << "fixpoint (and true (not (not q))) -> q OK" << std::endl;

    // ADD: (+ x 0) -> x
    parser.parseStr("(declare-const x Int)");
    parser.parseStr("(assert (= (+ x 0) x))");
    Node eqNode = parser.getAssertions().back();
    assert(kind(eqNode) == NODE_KIND::NT_EQ);
    Node lhs = child(eqNode, 0);
    Node rhs = child(eqNode, 1);
    Node lhsRewritten = rewriter.rewrite(lhs);
    assert(lhsRewritten == rhs && "(+ x 0) -> x");
    std::cout << "ADD (+ x 0) -> x OK" << std::endl;

    std::cout << "======= All Rewriter tests passed =======" << std::endl;
    return 0;
}
