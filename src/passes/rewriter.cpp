/* -*- Source -*-
 * Rewriter: bottom-up + memo rewriteOnce; fixpoint in rewrite().
 */
#include "somtparser/passes/rewriter.h"

namespace SOMTParser {

// --- RewriteContext::rebuildLike ---
Node RewriteContext::rebuildLike(Node old, const std::vector<Node>& newKids) const {
    if (!old || !node_manager_)
        return old;
    const size_t n = old->getChildrenSize();
    if (newKids.size() != n)
        return old;
    bool same = true;
    for (size_t i = 0; i < n; ++i) {
        if (old->getChild(static_cast<int>(i)) != newKids[i]) {
            same = false;
            break;
        }
    }
    if (same)
        return old;
    return node_manager_->createNode(old->getSort(), old->getKind(), old->getName(), newKids);
}

// --- Rewriter ---
Rewriter::Rewriter(std::shared_ptr<NodeManager> nm) noexcept
    : node_manager_(std::move(nm)), ctx_(node_manager_) {}

Node Rewriter::rewriteOnce(Node root) {
    if (!root)
        return nullptr;
    {
        auto it = memo_.find(root);
        if (it != memo_.end())
            return it->second;
    }
    std::vector<Node> newKids;
    newKids.reserve(numChildren(root));
    for (Node c : children(root)) {
        newKids.push_back(c ? rewriteOnce(c) : nullptr);
    }
    Node rebuilt = ctx_.rebuildLike(root, newKids);
    Node result = rules_.dispatch(rebuilt, ctx_);
    memo_[root] = result;
    return result;
}

Node Rewriter::rewrite(Node root, bool enable_fixpoint, unsigned max_rounds) {
    if (!root)
        return nullptr;
    if (!enable_fixpoint)
        return rewriteOnce(root);
    Node current = root;
    for (unsigned r = 0; r < max_rounds; ++r) {
        memo_.clear();
        Node next = rewriteOnce(current);
        if (next == current)
            return next;
        current = next;
    }
    return current;
}

// --- Default rule handlers (local only; children already rewritten) ---
static Node ruleNOT(Node n, RewriteContext& ctx) {
    if (!n || numChildren(n) < 1)
        return n;
    Node a = child(n, 0);
    if (!a)
        return n;
    if (kind(a) == NODE_KIND::NT_NOT && numChildren(a) >= 1)
        return child(a, 0);  // not(not x) -> x
    if (a->isTrue())
        return NodeManager::getFalse();
    if (a->isFalse())
        return NodeManager::getTrue();
    return ctx.rebuildLike(n, {a});
}

static Node ruleAND(Node n, RewriteContext& ctx) {
    if (!n || numChildren(n) < 2)
        return n;
    if (numChildren(n) == 2) {
        Node l = child(n, 0);
        Node r = child(n, 1);
        if (!l || !r)
            return n;
        if (l->isFalse() || r->isFalse())
            return NodeManager::getFalse();
        if (l->isTrue())
            return r;  // and true a -> a
        if (r->isTrue())
            return l;  // and a true -> a
        if (l == r)
            return l;  // and a a -> a
        return ctx.rebuildLike(n, {l, r});
    }
    std::vector<Node> kids;
    for (Node c : children(n))
        kids.push_back(c);
    return ctx.rebuildLike(n, kids);
}

static bool isZero(Node n) {
    if (!n || !n->isConst())
        return false;
    const std::string& name = n->getName();
    return name == "0" || name == "0.0";
}

static Node ruleADD(Node n, RewriteContext& ctx) {
    if (!n || numChildren(n) < 2)
        return n;
    if (numChildren(n) == 2) {
        Node l = child(n, 0);
        Node r = child(n, 1);
        if (!l || !r)
            return n;
        if (isZero(l))
            return r;  // (+ 0 x) -> x
        if (isZero(r))
            return l;  // (+ x 0) -> x
        return ctx.rebuildLike(n, {l, r});
    }
    std::vector<Node> kids;
    for (Node c : children(n))
        kids.push_back(c);
    return ctx.rebuildLike(n, kids);
}

static Node ruleFallback(Node n, RewriteContext& ctx) {
    if (!n)
        return n;
    std::vector<Node> kids;
    kids.reserve(numChildren(n));
    for (Node c : children(n))
        kids.push_back(c);
    return ctx.rebuildLike(n, kids);
}

void installDefaultRewriteRules(Rewriter& r) {
    r.rules()
        .onNOT(ruleNOT)
        .onAND(ruleAND)
        .onADD(ruleADD)
        .otherwise(ruleFallback);
}

} // namespace SOMTParser
