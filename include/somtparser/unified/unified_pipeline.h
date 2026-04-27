/* -*- Header -*-
 *
 * UnifiedPipeline — End-to-end pipeline: Plan / UnifiedModel → SMT-LIB2.
 *
 * This is the library's primary high-level API for producing SMT-LIB2
 * output from any Unified IR representation.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef SOMTPARSER_UNIFIED_PIPELINE_H
#define SOMTPARSER_UNIFIED_PIPELINE_H

#include "somtparser/unified/plan.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"
#include "somtparser/frontend/parser.h"

#include <string>
#include <vector>

namespace SOMTParser::Unified {

/** End-to-end pipeline: any input format → SMT-LIB2.
 *  This is the library's primary high-level API.
 */
class UnifiedPipeline {
public:
    explicit UnifiedPipeline(const UnifiedOpRegistry& registry);

    // Plan (already validated/emitted) → SMT-LIB2
    std::string planToSmt2(const Plan& plan);

    // UnifiedModel (already emitted) → SMT-LIB2
    std::string modelToSmt2(const UnifiedModel& model,
                            const std::string& logic_hint = "");

    const std::vector<std::string>& errors() const { return errors_; }

private:
    const UnifiedOpRegistry& registry_;
    Parser parser_;
    std::vector<std::string> errors_;

    static std::string mapLogicHint(const std::string& hint);
    void addError(const std::string& msg);
};

} // namespace SOMTParser::Unified

#endif // SOMTPARSER_UNIFIED_PIPELINE_H
