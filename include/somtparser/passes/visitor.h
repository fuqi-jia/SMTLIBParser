/* -*- Header -*-
 *
 * NodeVisitor: DAG traversal with visit-once guarantee (IR API Phase 2)
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef VISITOR_HEADER
#define VISITOR_HEADER

#include "somtparser/ir/node.h"

#include <unordered_set>

namespace SOMTParser {

/** DAG-safe visitor: each node is visited at most once (pre-order). */
class NodeVisitor {
public:
    virtual ~NodeVisitor() = default;

    /** Override to process each node. Default no-op. */
    virtual void visit(Node n) { (void)n; }

    /** Traverse DAG from root; calls visit(n) once per node. */
    void walk(Node root);

private:
    std::unordered_set<Node, NodeHash, NodeEqual> visited_;
};

}  // namespace SOMTParser

#endif /* VISITOR_HEADER */
