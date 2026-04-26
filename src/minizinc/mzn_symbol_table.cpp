/* -*- C++ -*-
 *
 * MiniZinc Frontend — Symbol Table Implementation
 */

#include "somtparser/minizinc/mzn_symbol_table.h"
#include "somtparser/minizinc/mzn_ast.h"

namespace SOMTParser::MiniZinc {

// ── Constructor ──────────────────────────────────────────────────
MznSymbolTable::MznSymbolTable() {
    scopes.push_back(ScopeFrame{}); // global scope
}

// ── Scope management ─────────────────────────────────────────────
void MznSymbolTable::pushScope() {
    scopes.push_back(ScopeFrame{});
}

void MznSymbolTable::popScope() {
    if (scopes.size() > 1) {
        scopes.pop_back();
    }
}

size_t MznSymbolTable::scopeDepth() const {
    return scopes.size();
}

// ── Registration ─────────────────────────────────────────────────
void MznSymbolTable::registerVar(const std::shared_ptr<VarDeclItem>& decl) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::VAR, decl->name, decl->loc);
    entry->payload = decl;
    scopes.back().entries[decl->name].push_back(entry);
    scopes.back().ordered_names.push_back(decl->name);
}

void MznSymbolTable::registerFunction(const std::shared_ptr<FunctionItem>& func) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::FUNCTION, func->name, func->loc);
    entry->payload = func;
    scopes.back().entries[func->name].push_back(entry);
}

void MznSymbolTable::registerPredicate(const std::shared_ptr<PredicateItem>& pred) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::PREDICATE, pred->name, pred->loc);
    entry->payload = pred;
    scopes.back().entries[pred->name].push_back(entry);
}

void MznSymbolTable::registerEnum(const std::shared_ptr<EnumDeclItem>& enm) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::ENUM, enm->name, enm->loc);
    entry->payload = enm;
    scopes.back().entries[enm->name].push_back(entry);
}

void MznSymbolTable::registerAnnotation(const std::shared_ptr<AnnotationItem>& ann) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::ANNOTATION, ann->name, ann->loc);
    entry->payload = ann;
    scopes.back().entries[ann->name].push_back(entry);
}

void MznSymbolTable::registerTypeAlias(const std::string& name,
                                       std::shared_ptr<TypeInst> type) {
    auto entry = std::make_shared<SymbolEntry>(
        SymbolEntry::Kind::TYPE_ALIAS, name, type->loc);
    entry->payload = type;
    scopes.back().entries[name].push_back(entry);
}

// ── Lookup ───────────────────────────────────────────────────────
std::shared_ptr<SymbolEntry> MznSymbolTable::lookup(const std::string& name) const {
    return lookupInScopes(name);
}

std::vector<std::shared_ptr<SymbolEntry>> MznSymbolTable::lookupAll(
    const std::string& name) const {
    std::vector<std::shared_ptr<SymbolEntry>> result;
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->entries.find(name);
        if (f != it->entries.end()) {
            result.insert(result.end(), f->second.begin(), f->second.end());
        }
    }
    return result;
}

std::shared_ptr<VarDeclItem> MznSymbolTable::lookupVar(const std::string& name) const {
    auto e = lookupInScopes(name);
    if (e && e->kind == SymbolEntry::Kind::VAR) {
        return std::static_pointer_cast<VarDeclItem>(e->payload);
    }
    return nullptr;
}

std::shared_ptr<EnumDeclItem> MznSymbolTable::lookupEnum(const std::string& name) const {
    auto e = lookupInScopes(name);
    if (e && e->kind == SymbolEntry::Kind::ENUM) {
        return std::static_pointer_cast<EnumDeclItem>(e->payload);
    }
    return nullptr;
}

std::shared_ptr<TypeInst> MznSymbolTable::lookupTypeAlias(const std::string& name) const {
    auto all = lookupAll(name);
    for (auto& e : all) {
        if (e->kind == SymbolEntry::Kind::TYPE_ALIAS) {
            return std::static_pointer_cast<TypeInst>(e->payload);
        }
    }
    return nullptr;
}

std::shared_ptr<FunctionItem> MznSymbolTable::resolveFunction(
    const std::string& name,
    const std::vector<std::shared_ptr<TypeInst>>& arg_types) const {
    auto all = lookupAll(name);
    for (auto& e : all) {
        if (e->kind != SymbolEntry::Kind::FUNCTION) continue;
        auto f = std::static_pointer_cast<FunctionItem>(e->payload);
        if (f->params.size() != arg_types.size()) continue;
        // TODO: stricter type matching
        return f;
    }
    return nullptr;
}

std::shared_ptr<PredicateItem> MznSymbolTable::resolvePredicate(
    const std::string& name,
    const std::vector<std::shared_ptr<TypeInst>>& arg_types) const {
    auto all = lookupAll(name);
    for (auto& e : all) {
        if (e->kind != SymbolEntry::Kind::PREDICATE) continue;
        auto p = std::static_pointer_cast<PredicateItem>(e->payload);
        if (p->params.size() != arg_types.size()) continue;
        // TODO: stricter type matching
        return p;
    }
    return nullptr;
}

// ── Queries ──────────────────────────────────────────────────────
bool MznSymbolTable::isDefined(const std::string& name) const {
    return lookupInScopes(name) != nullptr;
}

bool MznSymbolTable::isVar(const std::string& name) const {
    auto e = lookupInScopes(name);
    return e && e->kind == SymbolEntry::Kind::VAR;
}

bool MznSymbolTable::isPar(const std::string& name) const {
    auto vd = lookupVar(name);
    return vd && vd->type->isPar();
}

bool MznSymbolTable::isEnum(const std::string& name) const {
    auto e = lookupInScopes(name);
    return e && e->kind == SymbolEntry::Kind::ENUM;
}

void MznSymbolTable::clear() {
    scopes.clear();
    scopes.push_back(ScopeFrame{});
}

// ── Private helpers ──────────────────────────────────────────────
std::shared_ptr<SymbolEntry> MznSymbolTable::lookupInScopes(
    const std::string& name) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->entries.find(name);
        if (f != it->entries.end() && !f->second.empty()) {
            return f->second.back();
        }
    }
    return nullptr;
}

} // namespace SOMTParser::MiniZinc
