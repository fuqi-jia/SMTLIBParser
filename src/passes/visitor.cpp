/* -*- Source -*-
 *
 * NodeVisitor implementation (IR API Phase 2)
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#include "somtparser/passes/visitor.h"

#include <stack>
#include <vector>

namespace SOMTParser {

void NodeVisitor::walk(Node root) {
    if (!root)
        return;

    std::stack<Node> stack;
    stack.push(root);

    while (!stack.empty()) {
        Node n = stack.top();
        stack.pop();

        if (visited_.count(n))
            continue;
        visited_.insert(n);

        visit(n);

        std::vector<Node> kids;
        for (Node c : children(n)) {
            if (c)
                kids.push_back(c);
        }
        for (auto it = kids.rbegin(); it != kids.rend(); ++it)
            stack.push(*it);
    }
}

}  // namespace SOMTParser
