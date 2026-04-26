/* -*- Header -*-
 *
 * FlatZinc Frontend — Stub.
 *
 * FlatZinc is a low-level subset of MiniZinc with simpler syntax.
 * Most of the infrastructure is shared with the MiniZinc frontend.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef FZN_FRONTEND_H
#define FZN_FRONTEND_H

#include "somtparser/frontends/frontend.h"

namespace SOMTParser::Frontend {

class FlatZincFrontend : public Frontend {
public:
    std::string name() const override { return "flatzinc"; }
    std::vector<std::string> fileExtensions() const override { return {".fzn"}; }
    Unified::UnifiedModel parseFile(const std::string& filename) override;
    Unified::UnifiedModel parseString(const std::string& source,
                                       const std::string& filename = "<string>") override;
};

} // namespace SOMTParser::Frontend

#endif // FZN_FRONTEND_H
