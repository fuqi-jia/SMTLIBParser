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
    Node origVar = parser.mkVarBool("p");
    auto manager = parser.getNodeManager();
    Node innerNot = manager->createNode(SortManager::BOOL_SORT,
                                        NODE_KIND::NT_NOT, "not", {origVar});
    Node outerNot = manager->createNode(SortManager::BOOL_SORT,
                                        NODE_KIND::NT_NOT, "not", {innerNot});
    Node root = manager->createNode(SortManager::BOOL_SORT, NODE_KIND::NT_AND,
                                    "and", {NodeManager::getTrue(), outerNot});

    Rewriter rewriter(parser.getNodeManager());
    installDefaultRewriteRules(rewriter);

    Node result = rewriter.rewrite(root);  // default fixpoint true
    assert(result && "rewrite should return non-null");
    assert(result == origVar && "fixpoint: (and true (not (not p))) -> p");
    std::cout << "fixpoint (and true (not (not p))) -> p OK" << std::endl;

    // rewriteOnce alone may not reach p (depends on order); one round: (not (not p)) -> p, then (and true p) -> p next round
    Node expectQ = parser.mkVarBool("q");
    Node qNot = manager->createNode(SortManager::BOOL_SORT,
                                    NODE_KIND::NT_NOT, "not", {expectQ});
    Node qNotNot = manager->createNode(SortManager::BOOL_SORT,
                                       NODE_KIND::NT_NOT, "not", {qNot});
    Node root2 = manager->createNode(SortManager::BOOL_SORT,
                                     NODE_KIND::NT_AND, "and",
                                     {NodeManager::getTrue(), qNotNot});
    Node once1 = rewriter.rewriteOnce(root2);
    (void)once1;
    Node fixed = rewriter.rewrite(root2, true);
    assert(fixed == expectQ && "fixpoint (and true (not (not q))) -> q");
    std::cout << "fixpoint (and true (not (not q))) -> q OK" << std::endl;

    // ADD: (+ x 0) -> x
    Node rhs = parser.mkVarInt("x");
    Node zero = parser.mkConstInt(0);
    Node lhs = manager->createNode(SortManager::INT_SORT, NODE_KIND::NT_ADD,
                                   "+", {rhs, zero});
    Node lhsRewritten = rewriter.rewrite(lhs);
    assert(lhsRewritten == rhs && "(+ x 0) -> x");
    std::cout << "ADD (+ x 0) -> x OK" << std::endl;

    std::cout << "======= All Rewriter tests passed =======" << std::endl;
    return 0;
}
