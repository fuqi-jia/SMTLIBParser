/* -*- C++ -*-
 *
 * FrontendRegistry implementation
 */

#include "somtparser/frontends/frontend.h"

#include <algorithm>

namespace SOMTParser::Frontend {

void FrontendRegistry::registerFrontend(std::unique_ptr<Frontend> f) {
    if (f) frontends_.push_back(std::move(f));
}

Frontend* FrontendRegistry::detect(const std::string& filename) const {
    for (auto& f : frontends_) {
        for (auto& ext : f->fileExtensions()) {
            if (filename.size() >= ext.size() &&
                filename.compare(filename.size() - ext.size(), ext.size(), ext) == 0) {
                return f.get();
            }
        }
    }
    return nullptr;
}

Frontend* FrontendRegistry::get(const std::string& name) const {
    for (auto& f : frontends_) {
        if (f->name() == name) return f.get();
    }
    return nullptr;
}

std::vector<std::string> FrontendRegistry::list() const {
    std::vector<std::string> names;
    for (auto& f : frontends_) {
        names.push_back(f->name());
    }
    return names;
}

} // namespace SOMTParser::Frontend
