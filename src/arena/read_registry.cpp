// II-2b-3 (P3.a): trivial impl of the parser-side ExprId -> Sort read registry (see header).
// A translation-unit-local function-static singleton is the smallest correct first step; a
// NodeManager-owned instance can replace it in a later increment without changing the interface.
#include "somtparser/arena/read_registry.h"

#ifdef SOMTPARSER_WITH_ARENA
#include <utility>

namespace SOMTParser {

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

void ArenaReadRegistry::clear() {
    sort_.clear();
    value_.clear();  // II-2b-3 (P3.b): clear the value map too
}

}  // namespace SOMTParser
#endif  // SOMTPARSER_WITH_ARENA
