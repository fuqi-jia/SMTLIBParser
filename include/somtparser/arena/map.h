#pragma once
// II-2a native mapping: SOMTParser IR -> SOMTArena. Pure, no live-path dependency.
#include "somtparser/parser.h"
#include "somtarena/Arena.h"

#include <memory>
#include <string>
#include <vector>

namespace xarena_cov {

// Records mapping shortfalls without aborting, so one corpus run enumerates them all.
struct GapSink {
    std::vector<std::string> hard;   // unmapped kind/sort/value, equivalence mismatch
    std::vector<std::string> soft;   // expected/classified notes (e.g. MPFR real)
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

// SOMTParser constant/value -> native SOMTArena Payload (exact GMP: int->payloadInt(mpz),
// rational->payloadRational(mpq), bool->payloadBool, string->payloadString, BV->payloadBitVec).
// An MPFR REAL value (SOMTParser's approximate channel) is a SOFT gap (Xolver is exact-only,
// so it should never carry an exact-path value) -> payloadNone.
somtarena::Payload mapValue(const SOMTParser::DAGNode& n, GapSink& g);

}  // namespace xarena_cov
