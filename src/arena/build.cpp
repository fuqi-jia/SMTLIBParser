// II-2a parallel build implementation.
#include "somtparser/arena/build.h"
#include "somtparser/arena/read_registry.h"  // II-2b-3 (P3.a): ExprId -> Sort read registry
#include "somtarena/Payload.h"

#include <span>
#include <string>
#include <utility>
#include <vector>

namespace SOMTParser {
// II-2b-3 (foundation): definition of the thread_local front-end-phase flag declared in dag.h. Default
// false. Set true by installInlineArenaBuilder / buildAssertions (below) for the parse+import window;
// cleared false by the parent (Solver_impl_solve.cpp) right before parser.reset(). Unwired this
// increment (nothing reads it yet via arenaKind) — purely machinery for the crash-proof is* migration.
thread_local bool g_frontendPhase = false;
}  // namespace SOMTParser

namespace xarena_cov {

namespace {
using NK = SOMTParser::NODE_KIND;
bool isVarLeaf(NK k) { return k == NK::NT_VAR; }
bool isApply(NK k) {
    return k == NK::NT_UF_APPLY || k == NK::NT_FUNC_APPLY || k == NK::NT_FUNC_REC_APPLY;
}
// II-2b-3 (P3.c): at an own-handle site, mirror this node into the read registry so the DAGNode child
// accessors (getChildrenSize/getChildren/getChild) can be served from the arena. registerNode lets a
// child ExprId resolve back to its DAGNode; registerChildren captures the child ExprId list NOW (the
// arena is alive) — computing it at read time would use-after-free on the discard path (ir.reset()
// frees the inline arena). Apply nodes strip the funcDecl (arena child 0) via applyArgs so the list
// matches the DAGNode child normal form (func symbol lives in the DAGNode's name, not its children).
// Quantifiers (arena de Bruijn single-binder [paramSort, body] vs DAGNode [body, vars...]) and empty
// leaves stay field-backed — childrenOf returns null there so the reader uses its field.
void registerArenaNode(somtarena::Arena& a, somtarena::ExprId id,
                       const std::shared_ptr<SOMTParser::DAGNode>& node) {
    auto& reg = SOMTParser::ArenaReadRegistry::instance();
    reg.registerNode(&a, id, node);
    // II-2b-3 (P3.e): register this node's OWN (field) name, owner-tagged. Done for EVERY structural
    // node — including quantifiers, leaves, and operators whose name is "" — so it must precede the
    // quantifier/leaf early-returns below. registerArenaNode is only called at the 4 structural-owner
    // sites; let scaffolding returns early and never calls it, so a let node never registers a name →
    // the owner-tag rejects its forwarded-ExprId query → field fallback. Correct by construction.
    reg.registerName(&a, id, node.get(), node->getNameRaw());
    somtarena::Kind k = a.kind(id);
    if (somtarena::isQuantifier(k)) return;  // field-backed: child normal form differs
    std::span<const somtarena::ExprId> cs =
        (k == somtarena::Kind::Apply) ? a.applyArgs(id) : a.children(id);  // strip funcDecl on Apply
    if (cs.empty()) return;  // leaf: field-backed
    reg.registerChildren(&a, id, node.get(), std::vector<std::uint64_t>(cs.begin(), cs.end()));
}
somtarena::ExprId buildQuantifier(const std::shared_ptr<SOMTParser::DAGNode>& node,
                                  somtarena::Arena& a, GapSink& g, BuildState& st);
// II-2b-3 (P1.1): build the arena node for ONE non-let, non-quant DAGNode given its children's
// already-built handles `kids`. Shared by the recursive walk (buildArena) and the inline hook
// (installInlineArenaBuilder) so both emit byte-identical nodes. Returns NullExpr and records a gap
// on an unmapped kind. Does NOT memoize or set the DAGNode handle — the caller owns that.
// II-2b-3 (E4 step4b.1): the candidate kind is THREADED in as `nk` (last param) rather than read
// from node.getKind() — this is the SHARED core builder's last candidate kind-field dependency. Both
// callers pass what getKind() returns today (walk: node->getKind(); hook: the threaded nk_param), so
// this is verdict-neutral by construction. No default (both flipGtGe + nk are always passed
// explicitly) so a future caller can't silently inherit the wrong kind.
// II-2b-3 (E5): last param `liveInline` — the LIVE inline arena to source k/s/payload FROM (an
// arena->arena copy) when the node owns a known-live handle into it; null => field path. See below.
somtarena::ExprId buildCoreNode(const SOMTParser::DAGNode& node,
                                const std::vector<somtarena::ExprId>& kids,
                                somtarena::Arena& a, GapSink& g,
                                std::unordered_map<std::string, somtarena::ExprId>& funcDecls,
                                bool flipGtGe, NK nk, const somtarena::Arena* liveInline);
}  // namespace

somtarena::ExprId buildArena(const std::shared_ptr<SOMTParser::DAGNode>& node,
                             somtarena::Arena& a, GapSink& g, BuildState& st) {
    if (!node) return somtarena::NullExpr;
    NK nk = node->getKind();
    st.seenKinds.insert(static_cast<int>(nk));

    // Bound (quantified) variable: depth-dependent de Bruijn index, NEVER memoized
    // (the same qvar node yields different indices under different binder depths).
    if (nk == NK::NT_QUANT_VAR) {
        auto it = st.boundDepth.find(node.get());
        if (it == st.boundDepth.end()) {
            g.hardGap("unbound quant var: '" + node->getNameRaw() + "'");  // P3.e: field, not registry
            return somtarena::NullExpr;
        }
        std::uint64_t index = st.depth - 1 - it->second;
        return a.mkBoundVar(mapSort(node->getSortRaw(), a, g), index);  // P3.a: field, not registry
    }

    const SOMTParser::DAGNode* key = node.get();
    if (auto it = st.memo.find(key); it != st.memo.end()) return it->second;

    // --- let elimination (by node identity, mirrors the verified adapter) ---
    // P3.c: the builder reads the authoritative FIELD children (getChild*Raw), never the registry
    // (which it is the source of; a node may still hold a stale handle from a discarded build).
    if (nk == NK::NT_LET_BIND_VAR) {
        somtarena::ExprId id = node->getChildrenSizeRaw() > 0
                                   ? buildArena(node->getChildRaw(0), a, g, st)
                                   : somtarena::NullExpr;
        st.memo[key] = id;
        return id;
    }
    if (nk == NK::NT_LET || nk == NK::NT_LET_CHAIN) {
        size_t nc = node->getChildrenSizeRaw();
        somtarena::ExprId id = somtarena::NullExpr;
        if (nc > 0) {
            size_t bi = (nk == NK::NT_LET) ? 0 : (nc - 1);
            id = buildArena(node->getChildRaw(bi), a, g, st);
        }
        st.memo[key] = id;
        return id;
    }
    if (nk == NK::NT_LET_BIND_VAR_LIST) {
        st.memo[key] = somtarena::NullExpr;
        return somtarena::NullExpr;
    }

    // --- quantifiers: build in de Bruijn form (Task 6) ---
    if (nk == NK::NT_FORALL || nk == NK::NT_EXISTS) {
        somtarena::ExprId id = buildQuantifier(node, a, g, st);
        if (id != somtarena::NullExpr) {
            node->setArenaHandle(&a, id, /*finalized=*/true);  // II-2b-3 (P0): core node (own handle)
            // P3.a: register this node's own (field) sort under its arena handle (own-handle site).
            SOMTParser::ArenaReadRegistry::instance().registerSort(&a, id, node->getSortRaw());
            // P3.b: register this node's own (field) value beside the sort (authoritative field).
            SOMTParser::ArenaReadRegistry::instance().registerValue(&a, id, node->getValueRaw());
            // P3.c: register the node (children skipped for quantifiers — normal form differs).
            registerArenaNode(a, id, node);
        }
        st.memo[key] = id;
        return id;
    }

    // --- build children first (post-order) --- P3.c: field (Raw) children, not the registry.
    std::vector<somtarena::ExprId> kids;
    kids.reserve(node->getChildrenSizeRaw());
    for (size_t i = 0; i < node->getChildrenSizeRaw(); ++i) {
        if (auto c = node->getChildRaw(i)) kids.push_back(buildArena(c, a, g, st));
    }

    // II-2b-3 (E4 step4b.1): the walk's ONE remaining candidate kind-field read, centralized here as
    // the buildCoreNode `nk` argument (subject of step4b.2 — can the walk be made kind-field-free?).
    // II-2b-3 (E5): pass st.inlineArena — when set (NRA keep-live), buildCoreNode sources k/s/payload
    // from that live inline arena for a known-live-handle node instead of node's kind/sort/value field.
    somtarena::ExprId id =
        buildCoreNode(*node, kids, a, g, st.funcDecls, st.flipGtGe, node->getKind(), st.inlineArena);

    // II-2b-3 (P0): record this core node's arena handle on the DAGNode. The inline hook (P1.1) does
    // the same via buildCoreNode; populating it is verdict-neutral (cmp_native.sh is the gate).
    if (id != somtarena::NullExpr) {
        node->setArenaHandle(&a, id, /*finalized=*/true);
        // P3.a: register this node's own (field) sort under its arena handle (own-handle site).
        SOMTParser::ArenaReadRegistry::instance().registerSort(&a, id, node->getSortRaw());
        // P3.b: register this node's own (field) value beside the sort (authoritative field).
        SOMTParser::ArenaReadRegistry::instance().registerValue(&a, id, node->getValueRaw());
        // P3.c: register the node + its (Apply-remapped) child ExprId list for arena-served reads.
        registerArenaNode(a, id, node);
    }
    st.memo[key] = id;
    return id;
}

namespace {
// Build a quantifier in de Bruijn form. SOMTParser layout: child[0] = body,
// children[1..N] = bound vars (outermost first). Nest single-var mkForall/mkExists from
// innermost (xN) outward; each bound occurrence in the body resolves to mkBoundVar with
// index = currentDepth-1-binderPosition (index 0 = nearest binder).
somtarena::ExprId buildQuantifier(const std::shared_ptr<SOMTParser::DAGNode>& node,
                                  somtarena::Arena& a, GapSink& g, BuildState& st) {
    NK nk = node->getKind();
    auto body = node->getQuantBody();
    auto vars = node->getQuantVars();
    if (vars.empty() || !body) {
        g.hardGap("malformed quantifier (no vars/body)");
        return somtarena::NullExpr;
    }
    std::uint64_t D = st.depth;
    std::vector<somtarena::SortId> varSorts;
    varSorts.reserve(vars.size());
    for (std::size_t i = 0; i < vars.size(); ++i) {
        st.boundDepth[vars[i].get()] = D + i;  // binder position (outermost = D)
        varSorts.push_back(mapSort(vars[i]->getSortRaw(), a, g));  // P3.a: field, not registry
    }
    st.depth = D + vars.size();
    somtarena::ExprId bodyId = buildArena(body, a, g, st);
    st.depth = D;
    for (auto& v : vars) st.boundDepth.erase(v.get());

    somtarena::ExprId q = bodyId;
    for (std::size_t i = vars.size(); i-- > 0;) {
        q = (nk == NK::NT_FORALL) ? a.mkForall(varSorts[i], q) : a.mkExists(varSorts[i], q);
    }
    return q;
}

// II-2b-3 (E5): translate a sort node from the LIVE inline arena `ia` into the fresh arena `a` — a
// cross-arena copy that mirrors mapSort's target ids exactly (well-known sorts reuse `a`'s cached
// Bool/Int/Real/... singletons; parametric sorts hash-cons, so equal structure => equal id). Used by
// the arena->arena buildCoreNode copy so a node's sort is sourced from the arena, not its DAGNode
// field. Datatype sorts are NOT handled here (their DatatypeRegistry metadata rides gaps.dtSorts,
// which needs the SOMTParser Sort object the arena doesn't carry) — the caller routes a top-level
// datatype sort to mapSort(getSortRaw); a NESTED datatype (array-of-datatype, absent from every
// NRA/NIA/NIRA-path logic) hard-gaps here -> the caller falls back to the adapter. FP likewise.
somtarena::SortId translateInlineSort(const somtarena::Arena& ia, somtarena::SortId isort,
                                      somtarena::Arena& a, GapSink& g) {
    using K = somtarena::Kind;
    switch (ia.kind(isort)) {
        case K::SortBool:         return a.boolSort();
        case K::SortInt:          return a.intSort();
        case K::SortReal:         return a.realSort();
        case K::SortString:       return a.stringSort();
        case K::SortRegex:        return a.regexSort();
        case K::SortRoundingMode: return a.roundingModeSort();
        case K::SortBitVec:       return a.bitVecSort(ia.bitVecWidth(isort));
        case K::SortArray: {
            auto ch = ia.children(isort);
            somtarena::SortId idx = translateInlineSort(ia, ch[0], a, g);
            somtarena::SortId el  = translateInlineSort(ia, ch[1], a, g);
            return a.arraySort(idx, el);
        }
        case K::SortUninterpreted: {
            std::vector<somtarena::SortId> params;
            for (auto c : ia.children(isort)) params.push_back(translateInlineSort(ia, c, a, g));
            return a.uninterpretedSort(ia.stringValue(isort),
                                       std::span<const somtarena::SortId>(params.data(), params.size()));
        }
        default:
            g.hardGap("translateInlineSort: unhandled inline sort kind=" +
                      std::to_string(static_cast<int>(ia.kind(isort))));
            return somtarena::NullExpr;
    }
}

somtarena::ExprId buildCoreNode(const SOMTParser::DAGNode& node,
                                const std::vector<somtarena::ExprId>& kids,
                                somtarena::Arena& a, GapSink& g,
                                std::unordered_map<std::string, somtarena::ExprId>& funcDecls,
                                bool flipGtGe, NK nk, const somtarena::Arena* liveInline) {
    // II-2b-3 (E5, "drop DAGNode fields" CRUX): when `liveInline` is the EXACT arena this node's
    // handle points into (a known-live inline handle we are holding alive), do an arena->arena COPY —
    // read kind/sort/payload straight from the inline arena instead of the node's kind/sort/value
    // FIELD, and re-emit over the recursively-built `kids`. This is what lets the DAGNode fields drop
    // on the NRA path WITHOUT changing var-order: traversal is still the children field (buildArena),
    // only the per-node DATA source moves. Verdict-neutral by construction — the inline node was built
    // from this same DAGNode (via this same buildCoreNode, flipGtGe=false), so its k/s/payload equal
    // the field's; the ONE transform the rewritten walk adds is the >/>= flip, applied here on top.
    // The guard is an EXACT-pointer match (not merely arenaExprId()!=0), so a node with a handle into
    // a different/freed arena falls through to the field path and can never deref foreign memory.
    if (liveInline && node.arenaPtr() == liveInline && node.arenaExprId() != somtarena::NullExpr) {
        const somtarena::Arena& ia = *liveInline;
        somtarena::ExprId iid = node.arenaExprId();
        somtarena::Kind ak = ia.kind(iid);
        // Sort: pure arena->arena translate, EXCEPT a top-level datatype sort (needs the SOMTParser
        // Sort for gaps.dtSorts) keeps the byte-identical mapSort(getSortRaw) path (verdict-neutral;
        // datatype is not the NRA field-drop target and never appears sorting a var/const on that path).
        somtarena::SortId isort = ia.sortOf(iid);
        somtarena::SortId sort = (ia.kind(isort) == somtarena::Kind::SortDatatype)
                                     ? mapSort(node.getSortRaw(), a, g)
                                     : translateInlineSort(ia, isort, a, g);
        // >/>= -> </<= child-swap flip, now keyed on the inline arena's Kind (Gt/Ge). The inline node
        // was built unflipped (hook flipGtGe=false); the rewritten walk flips it here, mirroring the
        // field path below and FrontendAdapter::importNode.
        if (flipGtGe && (ak == somtarena::Kind::Gt || ak == somtarena::Kind::Ge) && kids.size() >= 2) {
            std::vector<somtarena::ExprId> sw(kids.begin(), kids.end());
            std::swap(sw[0], sw[1]);
            somtarena::Kind fk = (ak == somtarena::Kind::Gt) ? somtarena::Kind::Lt : somtarena::Kind::Le;
            return a.mkExpr(fk, sort, std::span<const somtarena::ExprId>(sw.data(), sw.size()),
                            ia.payloadOf(iid));
        }
        if (ak == somtarena::Kind::Apply) {
            // UF apply: rebuild FuncDecl(name, [argSorts..., resultSort]) in the fresh arena over the
            // fresh kids; the func NAME is read from the inline arena's FuncDecl (not the DAGNode name).
            std::string fname(ia.funcName(ia.applyFunc(iid)));
            somtarena::ExprId fd;
            if (auto fdIt = funcDecls.find(fname); fdIt != funcDecls.end()) {
                fd = fdIt->second;
            } else {
                std::vector<somtarena::SortId> sig;
                sig.reserve(kids.size() + 1);
                for (auto kid : kids) sig.push_back(a.sortOf(kid));
                sig.push_back(sort);  // result sort last
                fd = a.mkFuncDecl(fname, std::span<const somtarena::SortId>(sig.data(), sig.size()));
                funcDecls[fname] = fd;
            }
            return a.mkApply(fd, std::span<const somtarena::ExprId>(kids.data(), kids.size()));
        }
        // Generic arena->arena copy — covers Var (payloadOf == payloadString(name)), Const (value
        // payload), DT constructor/selector/tester (payloadString(name)), and every operator.
        return a.mkExpr(ak, sort, std::span<const somtarena::ExprId>(kids.data(), kids.size()),
                        ia.payloadOf(iid));
    }
    // ---- FIELD path (unchanged): no live inline handle -> read the DAGNode's own fields ----
    // II-2b-3 (E4 step4b.1): `nk` is the THREADED candidate kind (see fwd-decl comment) — the shared
    // core builder no longer reads node.getKind(). Other field reads below (getSortRaw/getNameRaw/
    // getValueRaw) are the node's OWN sort/name/value fields, not the kind field being dropped.
    somtarena::SortId sort = mapSort(node.getSortRaw(), a, g);  // P3.a: field, not registry
    // II-2b-3 (P4.c): >/>= -> </<= child-swap flip, mirroring FrontendAdapter::importNode
    // (adapter.cpp:171-177). Swap the first two child handles and retarget the Kind so the emitted
    // native node is Lt/Le(swapped) — nativeToXolverKind(Lt/Le)=Kind::Lt/Leq, giving byte-parity with
    // the adapter's IR. Gated (flipGtGe) so only the rewritten-NRA walk flips; leaves are unaffected.
    if (flipGtGe && (nk == NK::NT_GT || nk == NK::NT_GE) && kids.size() >= 2) {
        std::vector<somtarena::ExprId> sw(kids.begin(), kids.end());
        std::swap(sw[0], sw[1]);
        somtarena::Kind fk = (nk == NK::NT_GT) ? somtarena::Kind::Lt : somtarena::Kind::Le;
        return a.mkExpr(fk, sort, std::span<const somtarena::ExprId>(sw.data(), sw.size()),
                        node.getValueRaw() ? mapValue(node, g) : somtarena::payloadNone());
    }
    if (isVarLeaf(nk)) {
        // Variable identity carried by its name (so x != y structurally).
        return a.mkExpr(somtarena::Kind::Var, sort, {}, somtarena::payloadString(node.getNameRaw()));  // P3.e: field
    }
    if (isApply(nk)) {
        // UF apply: FuncDecl(name, [argSorts..., resultSort]) + Apply(funcDecl, args).
        std::string fname = node.getNameRaw();  // P3.e: field, not registry (builder is the source)
        somtarena::ExprId fd;
        if (auto fdIt = funcDecls.find(fname); fdIt != funcDecls.end()) {
            fd = fdIt->second;
        } else {
            std::vector<somtarena::SortId> sig;
            sig.reserve(kids.size() + 1);
            for (auto kid : kids) sig.push_back(a.sortOf(kid));
            sig.push_back(sort);  // result sort last
            fd = a.mkFuncDecl(fname, std::span<const somtarena::SortId>(sig.data(), sig.size()));
            funcDecls[fname] = fd;
        }
        return a.mkApply(fd, std::span<const somtarena::ExprId>(kids.data(), kids.size()));
    }
    bool mapped = false;
    somtarena::Kind k = mapKind(nk, mapped);
    if (!mapped) {
        g.hardGap("unmapped kind=" + std::to_string(static_cast<int>(nk)) +
                  " name='" + node.getNameRaw() + "'");  // P3.e: field, not registry
        return somtarena::NullExpr;
    }
    // Datatype operators carry their operator NAME (constructor/selector/tester symbol) in the
    // payload so Xolver's DatatypeRegistry can resolve them — mirrors the adapter. These nodes have
    // no getValue(), so mapValue would drop the name.
    somtarena::Payload pl;
    if (nk == NK::NT_DT_CONSTRUCTOR || nk == NK::NT_DT_SELECTOR || nk == NK::NT_DT_TESTER) {
        pl = somtarena::payloadString(node.getNameRaw());  // P3.e: field, not registry (builder is source)
    } else {
        pl = node.getValueRaw() ? mapValue(node, g) : somtarena::payloadNone();  // P3.b: field, not registry
    }
    return a.mkExpr(k, sort, std::span<const somtarena::ExprId>(kids.data(), kids.size()), pl);
}
}  // namespace

bool checkEquivalent(const std::shared_ptr<SOMTParser::DAGNode>& node,
                     somtarena::ExprId id, somtarena::Arena& a, GapSink& g) {
    if (!node) return true;
    if (id == somtarena::NullExpr || !a.isValidHandle(id)) {
        g.hardGap("checkEquivalent: invalid built handle for '" + node->getNameRaw() + "'");  // P3.e: field
        return false;
    }
    GapSink tmp;  // re-derive the expected sort without double-counting
    if (a.sortOf(id) != mapSort(node->getSortRaw(), a, tmp)) {  // P3.a: field, not registry
        g.hardGap("checkEquivalent: sort mismatch for '" + node->getNameRaw() + "'");  // P3.e: field
        return false;
    }
    return true;
}

std::vector<somtarena::ExprId> buildAssertions(SOMTParser::Parser& parser,
                                               somtarena::Arena& arena, GapSink& g) {
    // II-2b-3 (foundation): enter the front-end phase — this walk stamps arena handles onto the parser
    // DAGNodes, so arenaKind()'s field-net must keep is*/getKind on the NODE_KIND field until the parent
    // clears the flag after import+capture. Idempotent with the inline set. Unwired this increment.
    SOMTParser::g_frontendPhase = true;
    // P3.a: fresh registry per parse (walk path) so ExprIds from a prior file can't leak. Runs once
    // before the walk populates it; never mid-read (theory getSort reads happen after import).
    SOMTParser::ArenaReadRegistry::instance().clear();
    std::vector<somtarena::ExprId> roots;
    BuildState st;
    for (const auto& a : parser.getAssertions()) {
        roots.push_back(buildArena(a, arena, g, st));
    }
    return roots;
}

void installInlineArenaBuilder(SOMTParser::NodeManager& nm, somtarena::Arena& arena,
                               std::unordered_map<std::string, somtarena::ExprId>& funcDecls,
                               GapSink& gaps, bool& aborted) {
    // II-2b-3 (foundation): enter the front-end phase BEFORE parse — the inline hook forwards let
    // scaffolding a CHILD's ExprId, so arenaKind()'s field-net must derive is*/getKind from the field
    // until the parent clears the flag after import+capture. Unwired this increment.
    SOMTParser::g_frontendPhase = true;
    // P3.a: fresh registry per parse (inline path) so ExprIds from a prior file can't leak. Runs once
    // at hook install (before parse/population); never mid-read.
    SOMTParser::ArenaReadRegistry::instance().clear();
    // Pre-build the common static singletons (true/false), inserted at NodeManager init BEFORE any
    // hook, so formulas referencing them don't bail. They are leaves -> buildCoreNode over no kids.
    // Rarely-used transcendental/infinity singletons stay unhandled (a formula using one bails).
    auto prebuild = [&](const std::shared_ptr<SOMTParser::DAGNode>& n) {
        if (n && n->arenaExprId() == somtarena::NullExpr) {
            somtarena::ExprId id =
                buildCoreNode(*n, {}, arena, gaps, funcDecls, /*flipGtGe=*/false, n->getKind(),
                              /*liveInline=*/nullptr);  // building the inline arena -> field source
            if (id != somtarena::NullExpr) {
                n->setArenaHandle(&arena, id, /*finalized=*/true);
                // P3.a: register the singleton's own (field) sort under its handle (own-handle site).
                SOMTParser::ArenaReadRegistry::instance().registerSort(&arena, id, n->getSortRaw());
                // P3.b: register the singleton's own (field) value beside the sort (authoritative field).
                SOMTParser::ArenaReadRegistry::instance().registerValue(&arena, id, n->getValueRaw());
                // P3.c: register the singleton node (leaf — children skipped, field-backed).
                registerArenaNode(arena, id, n);
            }
        }
    };
    prebuild(SOMTParser::NodeManager::getTrue());
    prebuild(SOMTParser::NodeManager::getFalse());

    nm.setArenaBuilderHook(
        [&arena, &funcDecls, &gaps, &aborted](const std::shared_ptr<SOMTParser::DAGNode>& node,
                                              SOMTParser::NODE_KIND nk_param) {
            using NK = SOMTParser::NODE_KIND;
            if (aborted || !node) return;
            // II-2b-3 (E3 step4a): use the threaded candidate kind, not node->getKind() — the candidate
            // has no arena handle yet (this hook is what sets it), so its kind field is the wrong source
            // once the field is dropped. nk_param == candidateKind == what getKind() returns today.
            NK nk = nk_param;
            // Quantifiers + bound vars: de Bruijn needs top-down binder context which bottom-up
            // inline construction can't provide -> bail; the caller falls back to the walk.
            if (nk == NK::NT_FORALL || nk == NK::NT_EXISTS || nk == NK::NT_QUANT_VAR) {
                aborted = true;
                return;
            }
            // Let scaffolding has no arena node of its own; forward the relevant child's handle
            // (mirrors buildArena's let-elimination by node identity).
            // P3.a: deliberately NOT registered here — these sites reuse a CHILD's ExprId, which its
            // structural owner already registered with the same sort (a let's sort == its forwarded
            // child's sort). Re-registering the let node's sort would risk overwriting that entry.
            // P3.c: let scaffolding reads its FIELD (Raw) children — the arena reads are being built.
            if (nk == NK::NT_LET_BIND_VAR) {
                if (node->getChildrenSizeRaw() > 0 && node->getChildRaw(0))
                    node->setArenaHandle(&arena, node->getChildRaw(0)->arenaExprId());
                return;
            }
            if (nk == NK::NT_LET || nk == NK::NT_LET_CHAIN) {
                size_t nc = node->getChildrenSizeRaw();
                if (nc > 0) {
                    size_t bi = (nk == NK::NT_LET) ? 0 : (nc - 1);
                    if (auto b = node->getChildRaw(bi)) node->setArenaHandle(&arena, b->arenaExprId());
                }
                return;
            }
            if (nk == NK::NT_LET_BIND_VAR_LIST) return;  // structural list, no handle
            // Core node: gather children's cached handles (built earlier, bottom-up), build, cache.
            std::vector<somtarena::ExprId> kids;
            kids.reserve(node->getChildrenSizeRaw());
            for (size_t i = 0; i < node->getChildrenSizeRaw(); ++i) {
                auto c = node->getChildRaw(i);
                if (!c) continue;
                somtarena::ExprId cid = c->arenaExprId();
                if (cid == somtarena::NullExpr) { aborted = true; return; }  // child unbuilt -> bail
                kids.push_back(cid);
            }
            // II-2b-3 (E4 step4b.1): pass the THREADED nk (== nk_param) — this inline path is now fully
            // field-free for the candidate kind (buildCoreNode no longer reads node.getKind()).
            // II-2b-3 (E5): liveInline=nullptr — this hook IS what builds the inline arena, so the node
            // has no inline handle yet; it must read its own field (there is no arena source to copy).
            somtarena::ExprId id =
                buildCoreNode(*node, kids, arena, gaps, funcDecls, /*flipGtGe=*/false, nk,
                              /*liveInline=*/nullptr);
            if (id == somtarena::NullExpr) { aborted = true; return; }  // unmapped kind / gap -> bail
            node->setArenaHandle(&arena, id, /*finalized=*/true);
            // P3.a: register this node's own (field) sort under its arena handle (own-handle site).
            SOMTParser::ArenaReadRegistry::instance().registerSort(&arena, id, node->getSortRaw());
            // P3.b: register this node's own (field) value beside the sort (authoritative field).
            SOMTParser::ArenaReadRegistry::instance().registerValue(&arena, id, node->getValueRaw());
            // P3.c: register the node + its (Apply-remapped) child ExprId list for arena-served reads.
            registerArenaNode(arena, id, node);
        });
}

}  // namespace xarena_cov
