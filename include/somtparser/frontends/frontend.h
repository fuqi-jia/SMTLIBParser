/* -*- Header -*-
 *
 * Frontend — Common interface for all language frontends.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef FRONTEND_H
#define FRONTEND_H

#include "somtparser/unified/unified_ir.h"

#include <memory>
#include <string>
#include <vector>

namespace SOMTParser::Frontend {

/** Base interface for all language frontends. */
class Frontend {
public:
    virtual ~Frontend() = default;

    /** Human-readable frontend name. */
    virtual std::string name() const = 0;

    /** Supported file extensions (e.g., ".mzn", ".smt2", ".cnf"). */
    virtual std::vector<std::string> fileExtensions() const = 0;

    /** Parse a file into Unified IR. */
    virtual Unified::UnifiedModel parseFile(const std::string& filename) = 0;

    /** Parse a source string into Unified IR. */
    virtual Unified::UnifiedModel parseString(const std::string& source,
                                               const std::string& filename = "<string>") = 0;
};

/** Registry of available frontends. */
class FrontendRegistry {
public:
    void registerFrontend(std::unique_ptr<Frontend> f);
    Frontend* detect(const std::string& filename) const;
    Frontend* get(const std::string& name) const;
    std::vector<std::string> list() const;

private:
    std::vector<std::unique_ptr<Frontend>> frontends_;
};

} // namespace SOMTParser::Frontend

#endif // FRONTEND_H
