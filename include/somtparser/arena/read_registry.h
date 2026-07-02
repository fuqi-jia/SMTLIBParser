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

namespace somtarena { class Arena; }

namespace SOMTParser {
    class Sort;  // forward-declared: this header must not pull in the full Sort definition.

    // Per-parse singleton keyed by somtarena ExprId (uint64), each entry tagged with its owning
    // Arena*. Populated at arena-handle-set time by the builder (src/arena/build.cpp), cleared per
    // build. A missing key, or an Arena* mismatch, returns nullptr so the caller falls back to the
    // DAGNode's own field.
    class ArenaReadRegistry {
        struct Entry {
            const somtarena::Arena* arena;   // owning arena (compared, never dereferenced)
            std::shared_ptr<Sort>   sort;
        };
        std::unordered_map<std::uint64_t, Entry> sort_;

       public:
        static ArenaReadRegistry& instance();  // per-parse singleton; cleared per build

        void registerSort(const somtarena::Arena* arena, std::uint64_t exprId,
                          std::shared_ptr<Sort> s);
        // nullptr if the ExprId is absent OR was registered for a different arena.
        std::shared_ptr<Sort> sortOf(const somtarena::Arena* arena, std::uint64_t exprId) const;
        void clear();
    };
}  // namespace SOMTParser
#endif  // SOMTPARSER_WITH_ARENA
