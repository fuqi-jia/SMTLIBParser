// II-2a parallel build implementation.
#include "somtparser/arena/build.h"
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
somtarena::ExprId buildQuantifier(const std::shared_ptr<SOMTParser::DAGNode>& node,
                                  somtarena::Arena& a, GapSink& g, BuildState& st);
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
        return a.mkBoundVar(mapSort(node->getSort(), a, g), index);
    }

    const SOMTParser::DAGNode* key = node.get();
    if (auto it = st.memo.find(key); it != st.memo.end()) return it->second;

    // --- let elimination (by node identity, mirrors the verified adapter) ---
    if (nk == NK::NT_LET_BIND_VAR) {
        somtarena::ExprId id = node->getChildrenSize() > 0
                                   ? buildArena(node->getChild(0), a, g, st)
                                   : somtarena::NullExpr;
        st.memo[key] = id;
        return id;
    }
    if (nk == NK::NT_LET || nk == NK::NT_LET_CHAIN) {
        size_t nc = node->getChildrenSize();
        somtarena::ExprId id = somtarena::NullExpr;
        if (nc > 0) {
            size_t bi = (nk == NK::NT_LET) ? 0 : (nc - 1);
            id = buildArena(node->getChild(bi), a, g, st);
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
        st.memo[key] = id;
        return id;
    }

    // --- build children first (post-order) ---
    std::vector<somtarena::ExprId> kids;
    kids.reserve(node->getChildrenSize());
    for (size_t i = 0; i < node->getChildrenSize(); ++i) {
        if (auto c = node->getChild(i)) kids.push_back(buildArena(c, a, g, st));
    }

    somtarena::SortId sort = mapSort(node->getSort(), a, g);
    somtarena::ExprId id = somtarena::NullExpr;

    if (isVarLeaf(nk)) {
        // Variable identity carried by its name (so x != y structurally).
        id = a.mkExpr(somtarena::Kind::Var, sort, {}, somtarena::payloadString(node->getName()));
    } else if (isApply(nk)) {
        // UF apply: FuncDecl(name, [argSorts..., resultSort]) + Apply(funcDecl, args).
        std::string fname = node->getName();
        somtarena::ExprId fd;
        if (auto fdIt = st.funcDecls.find(fname); fdIt != st.funcDecls.end()) {
            fd = fdIt->second;
        } else {
            std::vector<somtarena::SortId> sig;
            sig.reserve(kids.size() + 1);
            for (auto kid : kids) sig.push_back(a.sortOf(kid));
            sig.push_back(sort);  // result sort last
            fd = a.mkFuncDecl(fname, std::span<const somtarena::SortId>(sig.data(), sig.size()));
            st.funcDecls[fname] = fd;
        }
        id = a.mkApply(fd, std::span<const somtarena::ExprId>(kids.data(), kids.size()));
    } else {
        bool mapped = false;
        somtarena::Kind k = mapKind(nk, mapped);
        if (!mapped) {
            g.hardGap("unmapped kind=" + std::to_string(static_cast<int>(nk)) +
                      " name='" + node->getName() + "'");
            st.memo[key] = somtarena::NullExpr;
            return somtarena::NullExpr;
        }
        // Datatype operators carry their operator NAME (constructor/selector/tester symbol) in
        // the payload so Xolver's DatatypeRegistry can resolve them — mirrors the adapter. These
        // nodes have no getValue(), so mapValue would drop the name.
        somtarena::Payload pl;
        if (nk == NK::NT_DT_CONSTRUCTOR || nk == NK::NT_DT_SELECTOR || nk == NK::NT_DT_TESTER) {
            pl = somtarena::payloadString(node->getName());
        } else {
            pl = node->getValue() ? mapValue(*node, g) : somtarena::payloadNone();
        }
        id = a.mkExpr(k, sort, std::span<const somtarena::ExprId>(kids.data(), kids.size()), pl);
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
        varSorts.push_back(mapSort(vars[i]->getSort(), a, g));
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
}  // namespace

bool checkEquivalent(const std::shared_ptr<SOMTParser::DAGNode>& node,
                     somtarena::ExprId id, somtarena::Arena& a, GapSink& g) {
    if (!node) return true;
    if (id == somtarena::NullExpr || !a.isValidHandle(id)) {
        g.hardGap("checkEquivalent: invalid built handle for '" + node->getName() + "'");
        return false;
    }
    GapSink tmp;  // re-derive the expected sort without double-counting
    if (a.sortOf(id) != mapSort(node->getSort(), a, tmp)) {
        g.hardGap("checkEquivalent: sort mismatch for '" + node->getName() + "'");
        return false;
    }
    return true;
}

std::vector<somtarena::ExprId> buildAssertions(SOMTParser::Parser& parser,
                                               somtarena::Arena& arena, GapSink& g) {
    std::vector<somtarena::ExprId> roots;
    BuildState st;
    for (const auto& a : parser.getAssertions()) {
        roots.push_back(buildArena(a, arena, g, st));
    }
    return roots;
}

}  // namespace xarena_cov
