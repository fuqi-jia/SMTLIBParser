/* -*- Header -*-
 *
 * Unified Op Registry — Extensible, runtime-loadable operator catalog.
 *
 * Every supported language maps its operators to canonical Unified ops.
 * Each Unified op carries SMT lowering rules and backend hints.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef UNIFIED_OP_REGISTRY_H
#define UNIFIED_OP_REGISTRY_H

#include <nlohmann/json.hpp>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>

namespace SOMTParser::Unified {

// ── Sort system for op signatures ──────────────────────────────────

struct UnifiedSort {
    enum class Kind {
        UNKNOWN, ANY,
        BOOL, INT, REAL, FLOAT, STRING,
        ARRAY, SET, ENUM, TUPLE, RECORD
    };
    Kind kind = Kind::UNKNOWN;
    std::string name;                          // ENUM name or type variable
    std::vector<UnifiedSort> params;           // ARRAY(elem), SET(elem), TUPLE(elems...)

    static UnifiedSort mkBool()   { return {Kind::BOOL, "", {}}; }
    static UnifiedSort mkInt()    { return {Kind::INT, "", {}}; }
    static UnifiedSort mkReal()   { return {Kind::REAL, "", {}}; }
    static UnifiedSort mkFloat()  { return {Kind::FLOAT, "", {}}; }
    static UnifiedSort mkString() { return {Kind::STRING, "", {}}; }
    static UnifiedSort mkArray(const UnifiedSort& elem) { return {Kind::ARRAY, "", {elem}}; }
    static UnifiedSort mkSet(const UnifiedSort& elem)   { return {Kind::SET, "", {elem}}; }
    static UnifiedSort mkEnum(const std::string& n)     { return {Kind::ENUM, n, {}}; }
    static UnifiedSort mkAny()    { return {Kind::ANY, "", {}}; }
};

// Custom JSON serialization for UnifiedSort::Kind (string <-> enum)
inline void to_json(nlohmann::json& j, const UnifiedSort::Kind& k) {
    switch (k) {
        case UnifiedSort::Kind::UNKNOWN: j = "unknown"; break;
        case UnifiedSort::Kind::BOOL:    j = "bool"; break;
        case UnifiedSort::Kind::INT:     j = "int"; break;
        case UnifiedSort::Kind::REAL:    j = "real"; break;
        case UnifiedSort::Kind::FLOAT:   j = "float"; break;
        case UnifiedSort::Kind::STRING:  j = "string"; break;
        case UnifiedSort::Kind::ARRAY:   j = "array"; break;
        case UnifiedSort::Kind::SET:     j = "set"; break;
        case UnifiedSort::Kind::ENUM:    j = "enum"; break;
        case UnifiedSort::Kind::TUPLE:   j = "tuple"; break;
        case UnifiedSort::Kind::RECORD:  j = "record"; break;
        case UnifiedSort::Kind::ANY:     j = "any"; break;
        default: j = "unknown"; break;
    }
}

inline void from_json(const nlohmann::json& j, UnifiedSort::Kind& k) {
    static const std::unordered_map<std::string, UnifiedSort::Kind> map = {
        {"unknown", UnifiedSort::Kind::UNKNOWN},
        {"bool",    UnifiedSort::Kind::BOOL},
        {"int",     UnifiedSort::Kind::INT},
        {"real",    UnifiedSort::Kind::REAL},
        {"float",   UnifiedSort::Kind::FLOAT},
        {"string",  UnifiedSort::Kind::STRING},
        {"array",   UnifiedSort::Kind::ARRAY},
        {"set",     UnifiedSort::Kind::SET},
        {"enum",    UnifiedSort::Kind::ENUM},
        {"tuple",   UnifiedSort::Kind::TUPLE},
        {"record",  UnifiedSort::Kind::RECORD},
        {"any",     UnifiedSort::Kind::ANY}
    };
    auto it = map.find(j.get<std::string>());
    k = (it != map.end()) ? it->second : UnifiedSort::Kind::UNKNOWN;
}

// Custom JSON serialization for UnifiedSort (with optional fields)
inline void to_json(nlohmann::json& j, const UnifiedSort& s) {
    j = nlohmann::json{{"kind", s.kind}};
    if (!s.name.empty()) j["name"] = s.name;
    if (!s.params.empty()) j["params"] = s.params;
}
inline void from_json(const nlohmann::json& j, UnifiedSort& s) {
    j.at("kind").get_to(s.kind);
    if (j.contains("name")) j.at("name").get_to(s.name); else s.name.clear();
    if (j.contains("params")) j.at("params").get_to(s.params); else s.params.clear();
}

// ── SMT lowering descriptor ────────────────────────────────────────

struct SmtLoweringDef {
    enum class Strategy { NATIVE, DECOMPOSE, AXIOMATIZE, EXTERNAL, UNSUPPORTED };
    Strategy strategy = Strategy::UNSUPPORTED;
    std::string decomposition_template;   // valid when strategy == DECOMPOSE
    std::string native_smt_name;          // valid when strategy == NATIVE
    std::vector<std::string> dependencies; // other unified ops required
};

// Custom JSON serialization for SmtLoweringDef::Strategy
inline void to_json(nlohmann::json& j, const SmtLoweringDef::Strategy& s) {
    switch (s) {
        case SmtLoweringDef::Strategy::NATIVE:       j = "native"; break;
        case SmtLoweringDef::Strategy::DECOMPOSE:    j = "decompose"; break;
        case SmtLoweringDef::Strategy::AXIOMATIZE:   j = "axiomatize"; break;
        case SmtLoweringDef::Strategy::EXTERNAL:     j = "external"; break;
        case SmtLoweringDef::Strategy::UNSUPPORTED:  j = "unsupported"; break;
        default: j = "unsupported"; break;
    }
}

inline void from_json(const nlohmann::json& j, SmtLoweringDef::Strategy& s) {
    static const std::unordered_map<std::string, SmtLoweringDef::Strategy> map = {
        {"native",      SmtLoweringDef::Strategy::NATIVE},
        {"decompose",   SmtLoweringDef::Strategy::DECOMPOSE},
        {"axiomatize",  SmtLoweringDef::Strategy::AXIOMATIZE},
        {"external",    SmtLoweringDef::Strategy::EXTERNAL},
        {"unsupported", SmtLoweringDef::Strategy::UNSUPPORTED}
    };
    auto it = map.find(j.get<std::string>());
    s = (it != map.end()) ? it->second : SmtLoweringDef::Strategy::UNSUPPORTED;
}

// Custom JSON serialization for SmtLoweringDef (with optional fields)
inline void to_json(nlohmann::json& j, const SmtLoweringDef& d) {
    j = nlohmann::json{{"strategy", d.strategy}};
    if (!d.decomposition_template.empty()) j["decomposition_template"] = d.decomposition_template;
    if (!d.native_smt_name.empty()) j["native_smt_name"] = d.native_smt_name;
    if (!d.dependencies.empty()) j["dependencies"] = d.dependencies;
}
inline void from_json(const nlohmann::json& j, SmtLoweringDef& d) {
    j.at("strategy").get_to(d.strategy);
    if (j.contains("decomposition_template")) j.at("decomposition_template").get_to(d.decomposition_template); else d.decomposition_template.clear();
    if (j.contains("native_smt_name")) j.at("native_smt_name").get_to(d.native_smt_name); else d.native_smt_name.clear();
    if (j.contains("dependencies")) j.at("dependencies").get_to(d.dependencies); else d.dependencies.clear();
}

// ── Backend hints (FlatZinc, DIMACS, MIP) ─────────────────────────

struct BackendHintsDef {
    std::string flatzinc_name;
    std::string dimacs_encoding;
    std::string mip_formulation;
};

// Custom JSON serialization for BackendHintsDef (all fields optional)
inline void to_json(nlohmann::json& j, const BackendHintsDef& h) {
    j = nlohmann::json::object();
    if (!h.flatzinc_name.empty()) j["flatzinc_name"] = h.flatzinc_name;
    if (!h.dimacs_encoding.empty()) j["dimacs_encoding"] = h.dimacs_encoding;
    if (!h.mip_formulation.empty()) j["mip_formulation"] = h.mip_formulation;
}
inline void from_json(const nlohmann::json& j, BackendHintsDef& h) {
    if (j.contains("flatzinc_name")) j.at("flatzinc_name").get_to(h.flatzinc_name); else h.flatzinc_name.clear();
    if (j.contains("dimacs_encoding")) j.at("dimacs_encoding").get_to(h.dimacs_encoding); else h.dimacs_encoding.clear();
    if (j.contains("mip_formulation")) j.at("mip_formulation").get_to(h.mip_formulation); else h.mip_formulation.clear();
}

// ── Canonical operator definition ──────────────────────────────────

struct UnifiedOpDef {
    std::string unified_name;        // canonical name, e.g. "all_different"
    std::string category;            // "arith", "bool", "global_cp", "array", "set", ...
    int arity = 0;                   // -1 = variadic
    bool is_commutative = false;

    std::vector<UnifiedSort> param_sorts;
    UnifiedSort return_sort;

    // language_id → local_name (e.g. "minizinc" → "all_different", "smtlib" → "all-different")
    std::unordered_map<std::string, std::string> lang_names;

    SmtLoweringDef smt_lowering;
    BackendHintsDef backend_hints;

    // Metadata
    std::string description;
    std::string doc_url;
    std::vector<std::string> tags;
};

// Custom JSON serialization for UnifiedOpDef (with optional fields)
inline void to_json(nlohmann::json& j, const UnifiedOpDef& d) {
    j = nlohmann::json{
        {"unified_name", d.unified_name},
        {"category", d.category},
        {"arity", d.arity},
        {"is_commutative", d.is_commutative},
        {"param_sorts", d.param_sorts},
        {"return_sort", d.return_sort},
        {"lang_names", d.lang_names},
        {"smt_lowering", d.smt_lowering},
        {"backend_hints", d.backend_hints},
        {"description", d.description},
        {"doc_url", d.doc_url},
        {"tags", d.tags}
    };
}
inline void from_json(const nlohmann::json& j, UnifiedOpDef& d) {
    j.at("unified_name").get_to(d.unified_name);
    j.at("category").get_to(d.category);
    if (j.contains("arity")) j.at("arity").get_to(d.arity); else d.arity = 0;
    if (j.contains("is_commutative")) j.at("is_commutative").get_to(d.is_commutative); else d.is_commutative = false;
    if (j.contains("param_sorts")) j.at("param_sorts").get_to(d.param_sorts); else d.param_sorts.clear();
    if (j.contains("return_sort")) j.at("return_sort").get_to(d.return_sort); else d.return_sort = UnifiedSort{};
    if (j.contains("lang_names")) j.at("lang_names").get_to(d.lang_names); else d.lang_names.clear();
    if (j.contains("smt_lowering")) j.at("smt_lowering").get_to(d.smt_lowering); else d.smt_lowering = SmtLoweringDef{};
    if (j.contains("backend_hints")) j.at("backend_hints").get_to(d.backend_hints); else d.backend_hints = BackendHintsDef{};
    if (j.contains("description")) j.at("description").get_to(d.description); else d.description.clear();
    if (j.contains("doc_url")) j.at("doc_url").get_to(d.doc_url); else d.doc_url.clear();
    if (j.contains("tags")) j.at("tags").get_to(d.tags); else d.tags.clear();
}

// ── Runtime reference to an op in the registry ─────────────────────

struct UnifiedOpRef {
    size_t id = 0;   // 0 = reserved for "unknown / invalid"
    bool valid() const { return id != 0; }
};

// ── Registry ───────────────────────────────────────────────────────

class UnifiedOpRegistry {
public:
    UnifiedOpRegistry();

    // Load / save JSON registry
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path) const;

    // Fetch remote JSON and merge (additive only)
    bool syncFromUrl(const std::string& url);

    // Register a single op (returns the assigned id)
    size_t registerOp(UnifiedOpDef def);

    // Lookups
    UnifiedOpRef lookupByUnifiedName(const std::string& name) const;
    UnifiedOpRef lookupByLangName(const std::string& lang, const std::string& name) const;

    const UnifiedOpDef* getDef(size_t id) const;
    const UnifiedOpDef* getDef(UnifiedOpRef ref) const;

    // Listing
    std::vector<std::string> listOps() const;
    std::vector<std::string> listOpsByCategory(const std::string& cat) const;
    std::vector<std::string> listOpsByTag(const std::string& tag) const;
    std::vector<std::string> categories() const;
    std::vector<std::string> tags() const;

    // Query lowering support
    bool hasSmtLowering(const std::string& unified_name) const;

    size_t size() const { return ops_.size() - 1; }  // exclude id 0

private:
    // id 0 reserved for "unknown"
    std::vector<UnifiedOpDef> ops_{UnifiedOpDef{}};

    std::unordered_map<std::string, size_t> unified_name_to_id_;
    // outer key = language id, inner key = local name
    std::unordered_map<std::string, std::unordered_map<std::string, size_t>> lang_to_name_to_id_;

    void rebuildIndices();
};

} // namespace SOMTParser::Unified

#endif // UNIFIED_OP_REGISTRY_H
