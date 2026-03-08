/* -*- Header -*-
 *
 * Rewriter: bottom-up rewrite with memo + fixpoint.
 * Rules are C++ handlers only; fixpoint default on; memo cleared each round.
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software and to permit persons to whom the
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

#ifndef REWRITER_HEADER
#define REWRITER_HEADER

#include "somtparser/context/context.h"
#include "somtparser/ir/dag.h"
#include "somtparser/core/kind.h"
#include "somtparser/ir/node.h"
#include "somtparser/passes/op_dispatcher.h"
#include "somtparser/ir/sort.h"

#include <memory>
#include <unordered_map>
#include <cstddef>

namespace SOMTParser {

/**
 * Context for rewrite handlers: provides rebuildLike(old, newKids).
 * Handlers are local: only inspect current node (children already rewritten);
 * do not call rewrite/rewriteOnce; multi-step effect by fixpoint.
 */
class RewriteContext : public Context {
public:
    explicit RewriteContext(std::shared_ptr<NodeManager> nm) noexcept : node_manager_(std::move(nm)) {}

    /**
     * If newKids equals old's children (by pointer), return old.
     * Otherwise create and return a new node with same kind/sort/name and newKids
     * (DAG / hash-consing via NodeManager).
     */
    Node rebuildLike(Node old, const std::vector<Node>& newKids) const;

    std::shared_ptr<NodeManager> getNodeManager() const noexcept { return node_manager_; }

private:
    std::shared_ptr<NodeManager> node_manager_;
};

/**
 * Rewriter: bottom-up rewrite with memo; rewrite(root) runs fixpoint by default.
 * Rule dispatch via OpDispatcher<Node, RewriteContext> (array + function pointer).
 * Memo is cleared at the start of each fixpoint round (no cross-round cache).
 */
class Rewriter {
public:
    static constexpr unsigned kDefaultMaxFixpointRounds = 256u;

    explicit Rewriter(std::shared_ptr<NodeManager> nm) noexcept;

    /** One bottom-up pass with memo; memo is not cleared inside. */
    Node rewriteOnce(Node root);

    /**
     * Rewrite until root stabilizes or max rounds. Default enable_fixpoint == true.
     * Each round: memo_.clear(); then rewriteOnce(current).
     */
    Node rewrite(Node root, bool enable_fixpoint = true,
                 unsigned max_rounds = kDefaultMaxFixpointRounds);

    RewriteContext& context() noexcept { return ctx_; }
    const RewriteContext& context() const noexcept { return ctx_; }

    /** OpDispatcher for rule registration (onNOT, onAND, onADD, ..., otherwise). */
    OpDispatcher<Node, RewriteContext>& rules() noexcept { return rules_; }
    const OpDispatcher<Node, RewriteContext>& rules() const noexcept { return rules_; }

private:
    std::shared_ptr<NodeManager> node_manager_;
    RewriteContext ctx_;
    OpDispatcher<Node, RewriteContext> rules_;
    std::unordered_map<Node, Node, NodeHash, NodeEqual> memo_;
};

/** Install minimal default rules: NOT, AND, ADD (for fixpoint demo). */
void installDefaultRewriteRules(Rewriter& r);

} // namespace SOMTParser

#endif /* REWRITER_HEADER */
