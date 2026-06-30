#pragma once
// II-2a parallel build: walk SOMTParser DAGNodes and build the native SOMTArena term.
// Mirrors the verified adapter (let-elim by node identity) but emits NATIVE arena nodes
// (no >/>= flip, one Const kind, de Bruijn quantifiers). Memo keyed by DAGNode* (SOMTParser
// hash-conses, so pointer identity == structural identity).
#include "somtparser/arena/map.h"
#include "somtparser/parser.h"
#include "somtarena/Arena.h"

#include <memory>
#include <set>
#include <string>
#include <unordered_map>

namespace xarena_cov {

// Per-corpus-file build state (reset per file; the Arena + funcDecls persist across roots
// in a file so shared function symbols dedup).
struct BuildState {
    std::unordered_map<const SOMTParser::DAGNode*, somtarena::ExprId> memo;
    std::unordered_map<std::string, somtarena::ExprId> funcDecls;  // UF name -> FuncDecl node
    // Scope stack for quantifier de Bruijn (Task 6): bound-var node -> binder depth.
    std::unordered_map<const SOMTParser::DAGNode*, std::uint64_t> boundDepth;
    std::uint64_t depth = 0;  // current number of enclosing binders
    std::set<int> seenKinds;  // NODE_KINDs exercised (coverage report)
};

// Build the native arena term for a DAGNode root (recursive + memoized). Records gaps in g.
somtarena::ExprId buildArena(const std::shared_ptr<SOMTParser::DAGNode>& node,
                             somtarena::Arena& a, GapSink& g, BuildState& st);

// Light structural check: the built node is a live handle whose sort matches mapSort and,
// for generic operator nodes, whose Kind + child count match the DAGNode. Records a hard
// gap on mismatch. (buildArena's gap-recording is the primary coverage signal; this catches
// a wrong-shape build, e.g. a let-elim arity error.)
bool checkEquivalent(const std::shared_ptr<SOMTParser::DAGNode>& node,
                     somtarena::ExprId id, somtarena::Arena& a, GapSink& g);

// SOMTParser's public "produce native arena" entry: build a SOMTArena term for every parsed
// assertion into `arena`, returning the assertion-root ExprIds in order. This is the seam
// Xolver consumes (II-2b-2) instead of walking DAGNodes. Gaps (should be none — proven over
// 982 corpus files) are recorded in `g`.
std::vector<somtarena::ExprId> buildAssertions(SOMTParser::Parser& parser,
                                               somtarena::Arena& arena, GapSink& g);

}  // namespace xarena_cov
