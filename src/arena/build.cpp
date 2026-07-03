// II-2a parallel build implementation.
#include "somtparser/arena/build.h"
#include "somtparser/arena/read_registry.h"  // II-2b-3 (P3.a): ExprId -> Sort read registry
#include "somtarena/Payload.h"

#include <span>
#include <string>
#include <vector>

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
somtarena::ExprId buildCoreNode(const SOMTParser::DAGNode& node,
                                const std::vector<somtarena::ExprId>& kids,
                                somtarena::Arena& a, GapSink& g,
                                std::unordered_map<std::string, somtarena::ExprId>& funcDecls);
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
            g.hardGap("unbound quant var: '" + node->getName() + "'");
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
            node->setArenaHandle(&a, id);  // II-2b-3 (P0): core node
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

    somtarena::ExprId id = buildCoreNode(*node, kids, a, g, st.funcDecls);

    // II-2b-3 (P0): record this core node's arena handle on the DAGNode. The inline hook (P1.1) does
    // the same via buildCoreNode; populating it is verdict-neutral (cmp_native.sh is the gate).
    if (id != somtarena::NullExpr) {
        node->setArenaHandle(&a, id);
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

somtarena::ExprId buildCoreNode(const SOMTParser::DAGNode& node,
                                const std::vector<somtarena::ExprId>& kids,
                                somtarena::Arena& a, GapSink& g,
                                std::unordered_map<std::string, somtarena::ExprId>& funcDecls) {
    NK nk = node.getKind();
    somtarena::SortId sort = mapSort(node.getSortRaw(), a, g);  // P3.a: field, not registry
    if (isVarLeaf(nk)) {
        // Variable identity carried by its name (so x != y structurally).
        return a.mkExpr(somtarena::Kind::Var, sort, {}, somtarena::payloadString(node.getName()));
    }
    if (isApply(nk)) {
        // UF apply: FuncDecl(name, [argSorts..., resultSort]) + Apply(funcDecl, args).
        std::string fname = node.getName();
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
                  " name='" + node.getName() + "'");
        return somtarena::NullExpr;
    }
    // Datatype operators carry their operator NAME (constructor/selector/tester symbol) in the
    // payload so Xolver's DatatypeRegistry can resolve them — mirrors the adapter. These nodes have
    // no getValue(), so mapValue would drop the name.
    somtarena::Payload pl;
    if (nk == NK::NT_DT_CONSTRUCTOR || nk == NK::NT_DT_SELECTOR || nk == NK::NT_DT_TESTER) {
        pl = somtarena::payloadString(node.getName());
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
        g.hardGap("checkEquivalent: invalid built handle for '" + node->getName() + "'");
        return false;
    }
    GapSink tmp;  // re-derive the expected sort without double-counting
    if (a.sortOf(id) != mapSort(node->getSortRaw(), a, tmp)) {  // P3.a: field, not registry
        g.hardGap("checkEquivalent: sort mismatch for '" + node->getName() + "'");
        return false;
    }
    return true;
}

std::vector<somtarena::ExprId> buildAssertions(SOMTParser::Parser& parser,
                                               somtarena::Arena& arena, GapSink& g) {
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
    // P3.a: fresh registry per parse (inline path) so ExprIds from a prior file can't leak. Runs once
    // at hook install (before parse/population); never mid-read.
    SOMTParser::ArenaReadRegistry::instance().clear();
    // Pre-build the common static singletons (true/false), inserted at NodeManager init BEFORE any
    // hook, so formulas referencing them don't bail. They are leaves -> buildCoreNode over no kids.
    // Rarely-used transcendental/infinity singletons stay unhandled (a formula using one bails).
    auto prebuild = [&](const std::shared_ptr<SOMTParser::DAGNode>& n) {
        if (n && n->arenaExprId() == somtarena::NullExpr) {
            somtarena::ExprId id = buildCoreNode(*n, {}, arena, gaps, funcDecls);
            if (id != somtarena::NullExpr) {
                n->setArenaHandle(&arena, id);
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
        [&arena, &funcDecls, &gaps, &aborted](const std::shared_ptr<SOMTParser::DAGNode>& node) {
            using NK = SOMTParser::NODE_KIND;
            if (aborted || !node) return;
            NK nk = node->getKind();
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
            somtarena::ExprId id = buildCoreNode(*node, kids, arena, gaps, funcDecls);
            if (id == somtarena::NullExpr) { aborted = true; return; }  // unmapped kind / gap -> bail
            node->setArenaHandle(&arena, id);
            // P3.a: register this node's own (field) sort under its arena handle (own-handle site).
            SOMTParser::ArenaReadRegistry::instance().registerSort(&arena, id, node->getSortRaw());
            // P3.b: register this node's own (field) value beside the sort (authoritative field).
            SOMTParser::ArenaReadRegistry::instance().registerValue(&arena, id, node->getValueRaw());
            // P3.c: register the node + its (Apply-remapped) child ExprId list for arena-served reads.
            registerArenaNode(arena, id, node);
        });
}

}  // namespace xarena_cov
