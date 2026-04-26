/* -*- C++ -*-
 *
 * FlatZinc Frontend — Stub implementation.
 */

#include "somtparser/frontends/flatzinc/fzn_frontend.h"

namespace SOMTParser::Frontend {

Unified::UnifiedModel FlatZincFrontend::parseFile(const std::string& /*filename*/) {
    // TODO: Implement FlatZinc parser (reuses MiniZinc lexer with different token rules)
    return Unified::UnifiedModel{};
}

Unified::UnifiedModel FlatZincFrontend::parseString(const std::string& /*source*/,
                                                     const std::string& /*filename*/) {
    // TODO: Implement FlatZinc string parser
    return Unified::UnifiedModel{};
}

} // namespace SOMTParser::Frontend
