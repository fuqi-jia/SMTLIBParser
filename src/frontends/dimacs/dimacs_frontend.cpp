/* -*- C++ -*-
 *
 * DIMACS Frontend — Stub implementation.
 */

#include "somtparser/frontends/dimacs/dimacs_frontend.h"

namespace SOMTParser::Frontend {

Unified::UnifiedModel DimacsFrontend::parseFile(const std::string& /*filename*/) {
    // TODO: Parse DIMACS CNF format
    // p cnf <vars> <clauses>
    // <lit> <lit> ... 0
    return Unified::UnifiedModel{};
}

Unified::UnifiedModel DimacsFrontend::parseString(const std::string& /*source*/,
                                                   const std::string& /*filename*/) {
    // TODO: Parse DIMACS CNF from string
    return Unified::UnifiedModel{};
}

} // namespace SOMTParser::Frontend
