#pragma once
// II-2a native mapping: SOMTParser IR -> SOMTArena. Pure, no live-path dependency.
#include "somtparser/parser.h"
#include "somtarena/Arena.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace xarena_cov {

// Records mapping shortfalls without aborting, so one corpus run enumerates them all.
struct GapSink {
    std::vector<std::string> hard;   // unmapped kind/sort/value, equivalence mismatch
    std::vector<std::string> soft;   // expected/classified notes (e.g. MPFR real)
    // II-2b-2: native datatype SortId -> its SOMTParser Sort. The arena datatype sort carries
    // only the NAME; the Sort carries constructors/selectors. mapSort records every datatype
    // sort it creates here so the Xolver import can populate its DatatypeRegistry.
    std::unordered_map<somtarena::SortId, std::shared_ptr<SOMTParser::Sort>> dtSorts;
    void hardGap(std::string s) { hard.push_back(std::move(s)); }
    void softGap(std::string s) { soft.push_back(std::move(s)); }
};

// SOMTParser Sort -> native SOMTArena sort (recursive for arrays). Records a hard gap +
// returns somtarena::NullExpr for an unmapped SORT_KIND.
somtarena::SortId mapSort(const std::shared_ptr<SOMTParser::Sort>& s,
                          somtarena::Arena& a, GapSink& g);

// SOMTParser NODE_KIND -> native SOMTArena Kind. Sets mapped=false (caller records a gap)
// for kinds with no arena equivalent — INCLUDING the parse-scaffolding kinds (NT_LET*,
// NT_QUANT_VAR, NT_TEMP_VAR, NT_FUNC_DEF/REC/PARAM, NT_ERROR/NULL/UNKNOWN), which the build
// walk resolves specially (let-elim / de Bruijn) rather than mapping directly.
somtarena::Kind mapKind(SOMTParser::NODE_KIND k, bool& mapped);

// Inverse of mapKind (II-2b-3 P4.a): reconstruct the DAGNode NODE_KIND from an arena node.
// For the 3 many-to-one collapse families, disambiguate from arena context:
//   K::Apply    -> NT_UF_APPLY        (uniform — the UF/defined/rec split is parser-internal &
//                                       lossless for the solver; NT_FUNC_APPLY is inlined,
//                                       NT_FUNC_REC_APPLY is served as a plain UF apply
//                                       everywhere downstream)
//   K::Eq       -> NT_EQ_BOOL       if the first operand's sort is Bool, else NT_EQ_OTHER
//   K::Distinct -> NT_DISTINCT_BOOL if the first operand's sort is Bool, else NT_DISTINCT_OTHER
// Every other arena Kind in mapKind's image maps 1:1 back to its NODE_KIND. Any arena Kind
// OUTSIDE mapKind's image (e.g. FuncDecl, or arena-internal kinds a DAGNode never carries)
// returns NT_UNKNOWN — getKind() is only ever called on nodes that came through mapKind, so
// this is a defensive default, not an expected path. Pure: reads only kind/children/sortOf.
SOMTParser::NODE_KIND arenaKindToNodeKind(const somtarena::Arena& a, somtarena::ExprId id);

// SOMTParser constant/value -> native SOMTArena Payload (exact GMP: int->payloadInt(mpz),
// rational->payloadRational(mpq), bool->payloadBool, string->payloadString, BV->payloadBitVec).
// An MPFR REAL value (SOMTParser's approximate channel) is a SOFT gap (Xolver is exact-only,
// so it should never carry an exact-path value) -> payloadNone.
somtarena::Payload mapValue(const SOMTParser::DAGNode& n, GapSink& g);

// II-2b-3 (endgame step3): value+name overload — maps a THREADED value (the node's `name` recovers
// only the BV literal) so the inline arena builder emits a Const payload WITHOUT reading the DAGNode
// value/name fields. The DAGNode-taking overload delegates here. Verdict-neutral (threaded == field).
somtarena::Payload mapValue(const std::shared_ptr<SOMTParser::Value>& val,
                            const std::string& name, GapSink& g);

}  // namespace xarena_cov
