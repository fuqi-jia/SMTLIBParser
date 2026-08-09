/**
 * Phase 2: NodeVisitor API unit test.
 * DAG traversal with visit-once guarantee; KindCounter example.
 */
#include "somtparser/frontend/parser.h"
#include <iostream>
#include <map>
#include "test_helpers.h"

using namespace SOMTParser;

class KindCounter : public NodeVisitor {
public:
    void visit(Node n) override {
        total_++;
        counts_[kind(n)]++;
    }
    size_t total() const { return total_; }
    size_t count(NODE_KIND k) const {
        auto it = counts_.find(k);
        return it == counts_.end() ? 0 : it->second;
    }

private:
    size_t total_ = 0;
    std::map<NODE_KIND, size_t> counts_;
};

int main() {
    std::cout << "======= NodeVisitor API (Phase 2) Test =======" << std::endl;

    ParserPtr parser = newParser();

    // Single node: 42
    Node leaf = parser->mkExpr("42");
    KindCounter c1;
    c1.walk(leaf);
    VERIFY(c1.total() == 1);
    VERIFY(c1.count(NODE_KIND::NT_CONST) == 1);
    std::cout << "Leaf 42: total=1, NT_CONST=1 OK" << std::endl;

    // (+ x 2): ADD + VAR + CONST = 3 nodes, each visited once
    parser->mkVarInt("x");
    Node add = parser->mkExpr("(+ x 2)");
    KindCounter c2;
    c2.walk(add);
    VERIFY(c2.total() == 3);
    VERIFY(c2.count(NODE_KIND::NT_ADD) == 1);
    VERIFY(c2.count(NODE_KIND::NT_VAR) == 1);
    VERIFY(c2.count(NODE_KIND::NT_CONST) == 1);
    std::cout << "(+ x 2): total=3 (ADD=1, VAR=1, CONST=1) OK" << std::endl;

    // DAG with sharing: (and (or a b) (or a b)) — (or a b) shared, 4 nodes (AND + OR + a + b)
    Node a = parser->mkVarBool("a");
    Node b = parser->mkVarBool("b");
    auto manager = parser->getNodeManager();
    Node sharedOr = manager->createNode(SortManager::BOOL_SORT,
                                        NODE_KIND::NT_OR, "or", {a, b});
    Node andNode = manager->createNode(SortManager::BOOL_SORT,
                                       NODE_KIND::NT_AND, "and",
                                       {sharedOr, sharedOr});
    KindCounter c3;
    c3.walk(andNode);
    VERIFY(c3.total() == 4 && "AND + OR + a + b, each once");
    VERIFY(c3.count(NODE_KIND::NT_AND) == 1);
    VERIFY(c3.count(NODE_KIND::NT_OR) == 1);
    VERIFY(c3.count(NODE_KIND::NT_VAR) == 2);
    std::cout << "(and (or a b) (or a b)): total=4 (DAG, shared subgraph visited once) OK" << std::endl;

    // Null root: no crash, 0 visits
    KindCounter c4;
    c4.walk(nullptr);
    VERIFY(c4.total() == 0);
    std::cout << "walk(nullptr): no visits OK" << std::endl;

    std::cout << "All NodeVisitor API tests passed." << std::endl;
    return 0;
}
