/* -*- Header -*-
 *
 * Node API and NodeRange (IR API Phase 1)
 *
 * Provides stable IR abstraction: Node type alias, free functions
 * (kind, sort, numChildren, child, children), and STL-style NodeRange
 * for iterating over children.
 *
 * Preferred usage:
 *   Node n = ...;
 *   for (Node c : children(n)) { ... }
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
  * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NODE_HEADER
#define NODE_HEADER

#include "somtparser/ir/dag.h"

#include <cstddef>

namespace SOMTParser {

/** IR Node type: shared_ptr to DAGNode (same as NodePtr). */
using Node = NodePtr;

/** @return kind of n; NT_UNKNOWN if n is null */
inline NODE_KIND kind(Node n) {
    return n ? n->getKind() : NODE_KIND::NT_UNKNOWN;
}

/** @return sort of n; nullptr if n is null */
inline std::shared_ptr<Sort> sort(Node n) {
    return n ? n->getSort() : nullptr;
}

/** @return number of children; 0 if n is null */
inline size_t numChildren(Node n) {
    return n ? n->getChildrenSize() : 0;
}

/** @return i-th child, or nullptr if n is null or i out of range */
inline Node child(Node n, size_t i) {
    if (!n || i >= n->getChildrenSize())
        return nullptr;
    return n->getChild(static_cast<int>(i));
}

/** STL-style range over children of a node. */
class NodeRange {
public:
    class iterator {
    public:
        using value_type        = Node;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Node*;
        using reference         = Node;
        using iterator_category = std::forward_iterator_tag;

        iterator() : node_(nullptr), index_(0) {}
        iterator(Node node, size_t index) : node_(node), index_(index) {}

        Node operator*() const {
            return node_ && index_ < node_->getChildrenSize()
                ? node_->getChild(static_cast<int>(index_))
                : nullptr;
        }

        iterator& operator++() {
            ++index_;
            return *this;
        }

        iterator operator++(int) {
            iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const iterator& other) const {
            return node_ == other.node_ && index_ == other.index_;
        }

        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }

    private:
        Node   node_;
        size_t index_;
    };

    explicit NodeRange(Node n) : node_(n) {}

    iterator begin() const {
        return iterator(node_, 0);
    }

    iterator end() const {
        return iterator(node_, numChildren(node_));
    }

private:
    Node node_;
};

/** @return range over children of n (for range-for). Prefer: for (Node c : children(n)) */
inline NodeRange children(Node n) {
    return NodeRange(n);
}

}  // namespace SOMTParser

#endif /* NODE_HEADER */
