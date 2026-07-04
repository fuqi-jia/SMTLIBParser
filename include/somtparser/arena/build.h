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
    // II-2b-3 (P4.c): apply the adapter's >/>= -> </<= child-swap flip during buildCoreNode so a
    // Stage-A-rewritten NRA arena import produces Lt/Leq(swapped kids) == the FrontendAdapter's IR
    // (byte-parity for the CAD var-order-sensitive NRA path). Default OFF (only the rewritten-NRA
    // walk sets it); every other arena path — inline non-NRA, the plain buildAssertions walk —
    // keeps the native Gt/Ge unflipped, so this is behavior-neutral for them.
    bool flipGtGe = false;
    // II-2b-3 (E5, "drop DAGNode fields" CRUX): the LIVE inline arena to source each node's
    // kind/sort/payload FROM (an arena->arena copy), instead of the DAGNode field. Non-null ONLY on
    // the NRA keep-live rewritten path, where the caller (Solver) holds the inline CoreIr alive across
    // this whole build (importNativeArenaSharedRewritten + finalizeNativeShared). buildCoreNode reads
    // the arena source for a node ONLY when node.arenaPtr()==inlineArena (the EXACT held arena) AND
    // node.arenaExprId()!=0 — a known-live handle into the arena we are holding; any other node (no
    // handle, or a handle into a different/overwritten arena) falls back to the FIELD path, so a
    // read can never deref a freed/foreign arena. Verdict-neutral by construction: the inline node
    // was built from the same DAGNode, so its k/s/payload == the field's, and traversal (children)
    // is unchanged. Null => every node uses the field path (== prior behavior).
    const somtarena::Arena* inlineArena = nullptr;
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

// II-2b-3 (P1.1b): register an INLINE arena builder on `nm` so each NEW DAGNode is built into
// `arena` AS IT IS CREATED during parsing (vs the post-parse buildArena walk). The hook mirrors
// buildArena's dispatch non-recursively over the children's already-cached handles (DAGNode::
// arenaExprId, set bottom-up): core nodes -> buildCoreNode; let-scaffolding -> forward a child's
// handle; quantifiers/bound-vars or any gap -> set `aborted` (the caller then falls back to the
// buildAssertions walk for the whole problem — de-Bruijn needs top-down context inline can't give).
// `funcDecls`, `gaps`, and `aborted` must OUTLIVE the parse (the hook captures them by reference).
// Call BEFORE parsing; clear afterwards with nm.setArenaBuilderHook({}). Gated/used by Xolver behind
// SOMTP_DAGNODE_ARENA_INLINE; validated structurally against the walk via the somtparser_arena_cov harness.
void installInlineArenaBuilder(SOMTParser::NodeManager& nm, somtarena::Arena& arena,
                               std::unordered_map<std::string, somtarena::ExprId>& funcDecls,
                               GapSink& gaps, bool& aborted);

}  // namespace xarena_cov
