/* -*- Header -*-
 *
 * DIMACS CNF Frontend — Stub.
 *
 * Parses DIMACS CNF / SAT format into Unified IR.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef DIMACS_FRONTEND_H
#define DIMACS_FRONTEND_H

#include "somtparser/frontends/frontend.h"

namespace SOMTParser::Frontend {

class DimacsFrontend : public Frontend {
public:
    std::string name() const override { return "dimacs"; }
    std::vector<std::string> fileExtensions() const override { return {".cnf", ".dimacs"}; }
    Unified::UnifiedModel parseFile(const std::string& filename) override;
    Unified::UnifiedModel parseString(const std::string& source,
                                       const std::string& filename = "<string>") override;
};

} // namespace SOMTParser::Frontend

#endif // DIMACS_FRONTEND_H
