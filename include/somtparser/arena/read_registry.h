#pragma once
// II-2b-3 (P3.a): parser-side ExprId -> Sort read registry. Lets DAGNode read accessors (getSort
// first) be served from the shared SOMTArena term-IR by arena ExprId, while keeping the rich
// SOMTParser::Sort object parser-side (SOMTArena must never depend on SOMTParser types — layering).
// Verdict-neutral by construction: registerSort stores the SAME interned shared_ptr<Sort> the
// DAGNode field holds, so sortOf() returns the identical object (pointer + structural equality).
//
// Arena-tagged: entries carry the owning Arena*, and sortOf() only returns a sort when the caller's
// Arena* matches. The AUTO flow can build the arena twice in one process (inline during parse, then
// a fallback walk after discarding the inline arena), reusing the same numeric ExprIds for different
// terms. Without the tag, a DAGNode still holding a handle into the discarded arena would read a
// foreign sort from the rebuilt registry. Matching the Arena* rejects such stale handles (they fall
// back to the field). The builder itself never reads this registry — it uses the authoritative field
// (DAGNode::getSortRaw) — so the arena is always constructed from correct sorts.
#ifdef SOMTPARSER_WITH_ARENA
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

namespace somtarena { class Arena; }

namespace SOMTParser {
    class Sort;   // forward-declared: this header must not pull in the full Sort definition.
    class Value;  // II-2b-3 (P3.b): forward-declared too (Value is a rich MPFR/Interval-bearing type;
                  // keep this header light — layering, no full Value include).
    class DAGNode;  // II-2b-3 (P3.c): forward-declared for the ExprId -> DAGNode node map (children
                    // materialization). Do NOT include dag.h — dag.h includes THIS header, so pulling
                    // it in would create a cycle. nodeFor only tag-compares the Arena* (never derefs).

    // Per-parse singleton keyed by somtarena ExprId (uint64), each entry tagged with its owning
    // Arena*. Populated at arena-handle-set time by the builder (src/arena/build.cpp), cleared per
    // build. A missing key, or an Arena* mismatch, returns nullptr so the caller falls back to the
    // DAGNode's own field.
    class ArenaReadRegistry {
        struct Entry {
            const somtarena::Arena* arena;   // owning arena (compared, never dereferenced)
            std::shared_ptr<Sort>   sort;
        };
        // II-2b-3 (P3.b): a SEPARATE Arena*-tagged ExprId -> Value map, distinct from the Sort map.
        // Value-bearing nodes are a subset (const/value-carrying); keeping this map separate avoids
        // bloating the Sort entry with an always-present (usually null) value slot.
        struct ValueEntry {
            const somtarena::Arena* arena;   // owning arena (compared, never dereferenced)
            std::shared_ptr<Value>  value;
        };
        // II-2b-3 (P3.c): Arena*-tagged ExprId -> DAGNode map. Lets DAGNode::getChild(i) turn a child
        // ExprId (drawn from children_) back into its canonical DAGNode. registerNode stores the SAME
        // interned shared_ptr<DAGNode> the parser hash-conses, so nodeFor returns the identical node.
        struct NodeEntry {
            const somtarena::Arena*  arena;   // owning arena (compared, never dereferenced)
            std::shared_ptr<DAGNode> node;
        };
        // II-2b-3 (P3.c): Arena*-tagged ExprId -> child ExprId list, in DAGNode child NORMAL FORM
        // (Apply nodes have the funcDecl — arena child 0 — already stripped via applyArgs). The
        // childIds are captured at BUILD time (arena alive); recomputing at read time would
        // use-after-free on the discard path (the inline arena is freed by ir.reset()). `owner` is the
        // DAGNode that owns this ExprId's structural node; childrenOf returns the list only when the
        // querying node IS that owner — rejecting the let-forward alias (a let node forwards a child's
        // ExprId as its own handle, but must read its OWN field children, not the child's arena list).
        struct ChildrenEntry {
            const somtarena::Arena*    arena;    // owning arena (compared, never dereferenced)
            const DAGNode*             owner;    // structural owner (compared, never dereferenced)
            std::vector<std::uint64_t> childIds;
        };
        std::unordered_map<std::uint64_t, Entry>         sort_;
        std::unordered_map<std::uint64_t, ValueEntry>    value_;
        std::unordered_map<std::uint64_t, NodeEntry>     node_;
        std::unordered_map<std::uint64_t, ChildrenEntry> children_;

       public:
        static ArenaReadRegistry& instance();  // per-parse singleton; cleared per build

        void registerSort(const somtarena::Arena* arena, std::uint64_t exprId,
                          std::shared_ptr<Sort> s);
        // nullptr if the ExprId is absent OR was registered for a different arena.
        std::shared_ptr<Sort> sortOf(const somtarena::Arena* arena, std::uint64_t exprId) const;

        // II-2b-3 (P3.b): same contract for Value. registerValue stores the SAME interned
        // shared_ptr<Value> the DAGNode field holds (populated by the builder at handle-set time, and
        // kept in sync by DAGNode::setValue on post-handle mutation), so valueOf() returns the
        // identical object. nullptr if the ExprId is absent OR was registered for a different arena.
        void registerValue(const somtarena::Arena* arena, std::uint64_t exprId,
                           std::shared_ptr<Value> v);
        std::shared_ptr<Value> valueOf(const somtarena::Arena* arena, std::uint64_t exprId) const;

        // II-2b-3 (P3.c): ExprId -> DAGNode (children materialization). registerNode stores the SAME
        // interned shared_ptr the parser hash-conses; nodeFor returns nullptr on absence or Arena*
        // mismatch (stale handle into a discarded arena), so the caller falls back to its field.
        void registerNode(const somtarena::Arena* arena, std::uint64_t exprId,
                          std::shared_ptr<DAGNode> node);
        std::shared_ptr<DAGNode> nodeFor(const somtarena::Arena* arena, std::uint64_t exprId) const;

        // II-2b-3 (P3.c): ExprId -> child ExprId list (DAGNode normal form). childrenOf returns the
        // list only when BOTH the Arena* AND the owner match (nullptr otherwise: absent, foreign
        // arena, or a let-forward alias where owner != the querying node) — so the caller falls back
        // to its field. The returned pointer aliases the map entry; valid until the next clear().
        void registerChildren(const somtarena::Arena* arena, std::uint64_t exprId,
                              const DAGNode* owner, std::vector<std::uint64_t> childIds);
        const std::vector<std::uint64_t>* childrenOf(const somtarena::Arena* arena,
                                                     std::uint64_t exprId, const DAGNode* owner) const;

        void clear();  // clears ALL FOUR maps (sort_, value_, node_, children_)
    };
}  // namespace SOMTParser
#endif  // SOMTPARSER_WITH_ARENA
