/* -*- C++ -*-
 *
 * Unified Op Registry Implementation
 */

#include "somtparser/unified/unified_op_registry.h"

#include <fstream>
#include <sstream>
#include <algorithm>

namespace SOMTParser::Unified {

// ── Registry ───────────────────────────────────────────────────────

UnifiedOpRegistry::UnifiedOpRegistry() {
    // id 0 is the reserved "unknown" op
    ops_.reserve(512);
}

bool UnifiedOpRegistry::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    try {
        nlohmann::json j;
        f >> j;

        if (j.contains("ops") && j["ops"].is_array()) {
            for (const auto& op_j : j["ops"]) {
                UnifiedOpDef def = op_j.get<UnifiedOpDef>();
                registerOp(std::move(def));
            }
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool UnifiedOpRegistry::saveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    nlohmann::json j;
    j["version"] = "1.0.0";
    nlohmann::json ops_j = nlohmann::json::array();
    for (size_t i = 1; i < ops_.size(); ++i) {
        ops_j.push_back(ops_[i]);
    }
    j["ops"] = ops_j;

    f << j.dump(2);
    return true;
}

bool UnifiedOpRegistry::syncFromUrl(const std::string& url) {
    // Stub: remote sync requires libcurl or equivalent.
    // Return false so caller falls back to local file.
    (void)url;
    return false;
}

size_t UnifiedOpRegistry::registerOp(UnifiedOpDef def) {
    // Check for duplicate unified name
    auto it = unified_name_to_id_.find(def.unified_name);
    if (it != unified_name_to_id_.end()) {
        // Overwrite existing definition (allows hot-updates)
        size_t id = it->second;
        ops_[id] = std::move(def);
        // Rebuild lang indices for this op
        for (const auto& [lang, local_name] : ops_[id].lang_names) {
            lang_to_name_to_id_[lang][local_name] = id;
        }
        return id;
    }

    size_t id = ops_.size();
    ops_.push_back(std::move(def));
    unified_name_to_id_[ops_[id].unified_name] = id;

    for (const auto& [lang, local_name] : ops_[id].lang_names) {
        lang_to_name_to_id_[lang][local_name] = id;
    }
    return id;
}

UnifiedOpRef UnifiedOpRegistry::lookupByUnifiedName(const std::string& name) const {
    auto it = unified_name_to_id_.find(name);
    if (it != unified_name_to_id_.end()) return {it->second};
    return {0};
}

UnifiedOpRef UnifiedOpRegistry::lookupByLangName(const std::string& lang,
                                                   const std::string& name) const {
    auto lit = lang_to_name_to_id_.find(lang);
    if (lit == lang_to_name_to_id_.end()) return {0};
    auto nit = lit->second.find(name);
    if (nit == lit->second.end()) return {0};
    return {nit->second};
}

const UnifiedOpDef* UnifiedOpRegistry::getDef(size_t id) const {
    if (id == 0 || id >= ops_.size()) return nullptr;
    return &ops_[id];
}

const UnifiedOpDef* UnifiedOpRegistry::getDef(UnifiedOpRef ref) const {
    return getDef(ref.id);
}

std::vector<std::string> UnifiedOpRegistry::listOps() const {
    std::vector<std::string> names;
    names.reserve(ops_.size() - 1);
    for (size_t i = 1; i < ops_.size(); ++i) {
        names.push_back(ops_[i].unified_name);
    }
    return names;
}

std::vector<std::string> UnifiedOpRegistry::listOpsByCategory(const std::string& cat) const {
    std::vector<std::string> names;
    for (size_t i = 1; i < ops_.size(); ++i) {
        if (ops_[i].category == cat) names.push_back(ops_[i].unified_name);
    }
    return names;
}

std::vector<std::string> UnifiedOpRegistry::listOpsByTag(const std::string& tag) const {
    std::vector<std::string> names;
    for (size_t i = 1; i < ops_.size(); ++i) {
        const auto& tags = ops_[i].tags;
        if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
            names.push_back(ops_[i].unified_name);
        }
    }
    return names;
}

std::vector<std::string> UnifiedOpRegistry::categories() const {
    std::unordered_map<std::string, bool> seen;
    std::vector<std::string> cats;
    for (size_t i = 1; i < ops_.size(); ++i) {
        if (seen.insert({ops_[i].category, true}).second) {
            cats.push_back(ops_[i].category);
        }
    }
    std::sort(cats.begin(), cats.end());
    return cats;
}

std::vector<std::string> UnifiedOpRegistry::tags() const {
    std::unordered_map<std::string, bool> seen;
    std::vector<std::string> all_tags;
    for (size_t i = 1; i < ops_.size(); ++i) {
        for (const auto& t : ops_[i].tags) {
            if (seen.insert({t, true}).second) {
                all_tags.push_back(t);
            }
        }
    }
    std::sort(all_tags.begin(), all_tags.end());
    return all_tags;
}

bool UnifiedOpRegistry::hasSmtLowering(const std::string& unified_name) const {
    auto ref = lookupByUnifiedName(unified_name);
    if (!ref.valid()) return false;
    const auto* def = getDef(ref);
    return def && def->smt_lowering.strategy != SmtLoweringDef::Strategy::UNSUPPORTED;
}

void UnifiedOpRegistry::rebuildIndices() {
    unified_name_to_id_.clear();
    lang_to_name_to_id_.clear();
    for (size_t i = 1; i < ops_.size(); ++i) {
        unified_name_to_id_[ops_[i].unified_name] = i;
        for (const auto& [lang, local_name] : ops_[i].lang_names) {
            lang_to_name_to_id_[lang][local_name] = i;
        }
    }
}

} // namespace SOMTParser::Unified
