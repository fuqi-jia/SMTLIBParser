// II-2b-3 (P3.a): trivial impl of the parser-side ExprId -> Sort read registry (see header).
// A translation-unit-local function-static singleton is the smallest correct first step; a
// NodeManager-owned instance can replace it in a later increment without changing the interface.
#include "somtparser/arena/read_registry.h"

#ifdef SOMTPARSER_WITH_ARENA
#include <utility>

namespace SOMTParser {

// II-2b-3 (reader-side): g_liveArena is defined in src/arena/build.cpp and declared in dag.h, but dag.h
// includes THIS header (cycle) so it can't be pulled in — re-declare the extern here. clear() nulls it
// at every arena-discard seam so a stale DAGNode handle into the discarded arena falls to its field.
extern thread_local const somtarena::Arena* g_liveArena;

ArenaReadRegistry& ArenaReadRegistry::instance() {
    static ArenaReadRegistry inst;
    return inst;
}

void ArenaReadRegistry::registerSort(const somtarena::Arena* arena, std::uint64_t exprId,
                                     std::shared_ptr<Sort> s) {
    // insert-or-assign: within one arena, hash-cons makes the ExprId structural, so any node mapping
    // to a given ExprId shares its sort. The Arena* tag disambiguates ExprIds reused across builds.
    sort_[exprId] = Entry{arena, std::move(s)};
}

std::shared_ptr<Sort> ArenaReadRegistry::sortOf(const somtarena::Arena* arena,
                                                std::uint64_t exprId) const {
    auto it = sort_.find(exprId);
    if (it == sort_.end() || it->second.arena != arena) return nullptr;  // absent or foreign arena
    return it->second.sort;
}

// II-2b-3 (P3.b): mirror of registerSort/sortOf for Value. Same Arena*-tag discipline: the entry is
// tagged with its owning arena and valueOf() only returns on a matching arena, so a DAGNode still
// holding a stale handle into a discarded (double-build) arena falls back to its field.
void ArenaReadRegistry::registerValue(const somtarena::Arena* arena, std::uint64_t exprId,
                                      std::shared_ptr<Value> v) {
    value_[exprId] = ValueEntry{arena, std::move(v)};
}

std::shared_ptr<Value> ArenaReadRegistry::valueOf(const somtarena::Arena* arena,
                                                  std::uint64_t exprId) const {
    auto it = value_.find(exprId);
    if (it == value_.end() || it->second.arena != arena) return nullptr;  // absent or foreign arena
    return it->second.value;
}

// II-2b-3 (P3.c): ExprId -> DAGNode. Same Arena*-tag discipline: nodeFor returns the node only on a
// matching arena, so a DAGNode still holding a stale handle into a discarded arena reads null. The
// DAGNode type is incomplete here (forward-declared) — fine, we only move/store/compare shared_ptrs
// (the deleter is type-erased in the control block created parser-side).
void ArenaReadRegistry::registerNode(const somtarena::Arena* arena, std::uint64_t exprId,
                                     std::shared_ptr<DAGNode> node) {
    node_[exprId] = NodeEntry{arena, std::move(node)};
}

std::shared_ptr<DAGNode> ArenaReadRegistry::nodeFor(const somtarena::Arena* arena,
                                                    std::uint64_t exprId) const {
    auto it = node_.find(exprId);
    if (it == node_.end() || it->second.arena != arena) return nullptr;  // absent or foreign arena
    return it->second.node;
}

// II-2b-3 (P3.c): ExprId -> child ExprId list (DAGNode normal form; Apply funcDecl already stripped
// at the call site). childrenOf gates on BOTH the Arena* AND the owner: the owner tag rejects the
// let-forward alias, where a let node's handle is a child's ExprId owned by a DIFFERENT DAGNode.
void ArenaReadRegistry::registerChildren(const somtarena::Arena* arena, std::uint64_t exprId,
                                         const DAGNode* owner, std::vector<std::uint64_t> childIds) {
    children_[exprId] = ChildrenEntry{arena, owner, std::move(childIds)};
}

const std::vector<std::uint64_t>* ArenaReadRegistry::childrenOf(const somtarena::Arena* arena,
                                                                std::uint64_t exprId,
                                                                const DAGNode* owner) const {
    auto it = children_.find(exprId);
    if (it == children_.end() || it->second.arena != arena || it->second.owner != owner)
        return nullptr;  // absent, foreign arena, or let-forward alias (owner mismatch)
    return &it->second.childIds;
}

// II-2b-3 (P3.e): ExprId -> node NAME. Same owner-tag discipline as childrenOf: nameFor gates on BOTH
// the Arena* AND the owner. The owner tag rejects the let-forward alias — a let node forwards a
// child's ExprId as its own handle, but its OWN name (the bound var) differs from the forwarded
// child's name, so it must fall back to its own field rather than read the child's registered name.
void ArenaReadRegistry::registerName(const somtarena::Arena* arena, std::uint64_t exprId,
                                     const DAGNode* owner, std::string name) {
    name_[exprId] = NameEntry{arena, owner, std::move(name)};
}

const std::string* ArenaReadRegistry::nameFor(const somtarena::Arena* arena, std::uint64_t exprId,
                                              const DAGNode* owner) const {
    auto it = name_.find(exprId);
    if (it == name_.end() || it->second.arena != arena || it->second.owner != owner)
        return nullptr;  // absent, foreign arena, or let-forward alias (owner mismatch)
    return &it->second.name;
}

void ArenaReadRegistry::clear() {
    sort_.clear();
    value_.clear();     // II-2b-3 (P3.b): clear the value map too
    node_.clear();      // II-2b-3 (P3.c): and the node map
    children_.clear();  // II-2b-3 (P3.c): and the children map
    name_.clear();      // II-2b-3 (P3.e): and the name map
    g_liveArena = nullptr;  // II-2b-3: arena-discard seam — stale handles now fall to field
}

// II-2b-3 (big-field-drop): enumerate the (ExprId, owner) of every NAME entry tagged with `a`. The
// name map has one entry per structural core node the inline hook registered (registerName owner ==
// the node's own DAGNode), so this yields exactly the parse-built nodes' arena ids paired with their
// owning DAGNode — the seam the NRA rewritten import snapshots (sort/name/value via sortOf/valueOf/
// nameFor over these ids) into its pass-2 metaMap before clearing the registry. Owners are copied out
// as raw pointers (keys/comparison only), never dereferenced here.
std::vector<std::pair<std::uint64_t, const DAGNode*>> ArenaReadRegistry::ownedEntries(
    const somtarena::Arena* a) const {
    std::vector<std::pair<std::uint64_t, const DAGNode*>> out;
    out.reserve(name_.size());
    for (const auto& [id, e] : name_)
        if (e.arena == a) out.emplace_back(id, e.owner);
    return out;
}

}  // namespace SOMTParser
#endif  // SOMTPARSER_WITH_ARENA
