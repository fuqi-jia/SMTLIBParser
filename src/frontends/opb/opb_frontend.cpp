/* -*- C++ -*-
 *
 * OPB Frontend — Stub implementation.
 */

#include "somtparser/frontends/opb/opb_frontend.h"

namespace SOMTParser::Frontend {

Unified::UnifiedModel OpbFrontend::parseFile(const std::string& /*filename*/) {
    // TODO: Parse OPB / WCNF format
    // * #variable= ... #constraint= ...
    // +1 x1 +1 x2 >= 1 ;
    return Unified::UnifiedModel{};
}

Unified::UnifiedModel OpbFrontend::parseString(const std::string& /*source*/,
                                                const std::string& /*filename*/) {
    // TODO: Parse OPB from string
    return Unified::UnifiedModel{};
}

} // namespace SOMTParser::Frontend
