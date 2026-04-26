/* -*- Header -*-
 *
 * OPB / WCNF Frontend — Stub.
 *
 * Parses Pseudo-Boolean (OPB) and Weighted CNF (WCNF) formats.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef OPB_FRONTEND_H
#define OPB_FRONTEND_H

#include "somtparser/frontends/frontend.h"

namespace SOMTParser::Frontend {

class OpbFrontend : public Frontend {
public:
    std::string name() const override { return "opb"; }
    std::vector<std::string> fileExtensions() const override { return {".opb", ".wcnf", ".pb"}; }
    Unified::UnifiedModel parseFile(const std::string& filename) override;
    Unified::UnifiedModel parseString(const std::string& source,
                                       const std::string& filename = "<string>") override;
};

} // namespace SOMTParser::Frontend

#endif // OPB_FRONTEND_H
