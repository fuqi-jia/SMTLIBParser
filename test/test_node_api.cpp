/**
 * Phase 1: Node API and NodeRange unit test.
 * Exercises kind(), sort(), numChildren(), child(), children() and range-for.
 */
#include "somtparser/ir/node.h"
#include "somtparser/frontend/parser.h"
#include <cassert>
#include <iostream>

using namespace SOMTParser;

int main() {
    std::cout << "======= Node API (Phase 1) Test =======" << std::endl;

    ParserPtr parser = newParser();

    // Leaf: constant
    Node leaf = parser->mkExpr("42");
    assert(leaf);
    assert(kind(leaf) == NODE_KIND::NT_CONST);
    assert(sort(leaf));
    assert(numChildren(leaf) == 0);
    assert(child(leaf, 0) == nullptr);
    size_t count = 0;
    std::cout << "Leaf (42): " << parser->toString(leaf) << std::endl;
    for (Node c : children(leaf)) {
        std::cout << "Child: " << parser->toString(c) << std::endl;
        ++count;
    }
    assert(count == 0);
    std::cout << "Leaf (42): kind=NT_CONST, numChildren=0, range size=0 OK" << std::endl;

    // (+ 1 2): parser may fold to constant 3 or keep ADD node; both are valid for testing Node API
    Node add = parser->mkExpr("(+ 1 2)");
    assert(add);
    std::cout << "(+ 1 2) -> " << parser->toString(add)
              << ", kind=" << (kind(add) == NODE_KIND::NT_CONST ? "NT_CONST" : "NT_ADD")
              << ", numChildren=" << numChildren(add) << std::endl;

    if (kind(add) == NODE_KIND::NT_CONST) {
        // Folded to constant 3 at parse time
        assert(numChildren(add) == 0);
        count = 0;
        for (Node c : children(add)) { (void)c; ++count; }
        assert(count == 0);
        std::cout << "  (folded to constant: 0 children) OK" << std::endl;
    } else {
        // Not folded; still ADD node with two children 1 and 2
        assert(kind(add) == NODE_KIND::NT_ADD);
        assert(numChildren(add) == 2);
        Node a = child(add, 0), b = child(add, 1);
        assert(a && b);
        count = 0;
        for (Node c : children(add)) {
            std::cout << "  child: " << parser->toString(c) << std::endl;
            assert(c);
            ++count;
        }
        assert(count == 2);
        std::cout << "  (ADD with 2 children) OK" << std::endl;
    }

    // (+ x 2): with variable, no folding; must be ADD with 2 children
    parser->mkVarInt("x");
    Node addX = parser->mkExpr("(+ x 2)");
    assert(addX);
    assert(kind(addX) == NODE_KIND::NT_ADD);
    assert(numChildren(addX) == 2);
    count = 0;
    std::cout << "(+ x 2): " << parser->toString(addX) << std::endl;
    for (Node c : children(addX)) {
        std::cout << "  child: " << parser->toString(c) << std::endl;
        assert(c);
        ++count;
    }
    assert(count == 2);
    std::cout << "(+ x 2): kind=NT_ADD, numChildren=2, range-for count=2 OK" << std::endl;

    // Null safety
    Node empty;
    assert(kind(empty) == NODE_KIND::NT_UNKNOWN);
    assert(sort(empty) == nullptr);
    assert(numChildren(empty) == 0);
    assert(child(empty, 0) == nullptr);

    std::cout << "Empty node: " << parser->toString(empty) << std::endl;
    for (Node c : children(empty)) {
        assert(false && "should not iterate");
    }
    std::cout << "Null node: safe defaults OK" << std::endl;
    std::cout << "All Node API tests passed." << std::endl;
    return 0;
}
