/* -*- Source -*-
 * SymbolManager implementation
 */

#include "somtparser/frontend/symbol_manager.h"
#include <algorithm>

namespace SOMTParser {

SymbolManager::SymbolManager() = default;

void SymbolManager::reserve(size_t capacity) {
    let_key_map_.reserve(capacity);
    preserving_let_key_map_.reserve(capacity);
    fun_key_map_.reserve(capacity);
    fun_var_map_.reserve(capacity);
    sort_key_map_.reserve(capacity);
    quant_var_map_.reserve(capacity);
    var_names_.reserve(capacity);
    temp_var_names_.reserve(capacity);
    placeholder_var_names_.reserve(capacity);
    function_names_.reserve(capacity);
    static_functions_.reserve(capacity);
}

std::shared_ptr<DAGNode> SymbolManager::resolveSymbol(const std::string& name, const ResolveScope& scope) const {
    auto it_ph = placeholder_var_names_.find(name);
    if (it_ph != placeholder_var_names_.end())
        return it_ph->second;

    if (scope.check_preserving_let && !scope.preserving_let_name.empty()) {
        auto it = preserving_let_key_map_.find(scope.preserving_let_name);
        if (it != preserving_let_key_map_.end())
            return it->second;
    }
    if (scope.check_let) {
        auto it = let_key_map_.find(name);
        if (it != let_key_map_.end())
            return it->second;
    }

    auto it_fun = fun_key_map_.find(name);
    if (it_fun != fun_key_map_.end())
        return it_fun->second;
    auto it_fv = fun_var_map_.find(name);
    if (it_fv != fun_var_map_.end())
        return it_fv->second;
    if (scope.in_quantifier_scope) {
        auto it_q = quant_var_map_.find(name);
        if (it_q != quant_var_map_.end())
            return it_q->second;
    }

    auto it_var = var_names_.find(name);
    if (it_var != var_names_.end())
        return it_var->second;
    if (name.size() > 2 && name[0] == '|' && name[name.size() - 1] == '|') {
        std::string inner = name.substr(1, name.size() - 2);
        auto it = var_names_.find(inner);
        if (it != var_names_.end())
            return it->second;
    }
    std::string bar_name = '|' + name + '|';
    auto it_bar = var_names_.find(bar_name);
    if (it_bar != var_names_.end())
        return it_bar->second;
    return nullptr;
}

std::shared_ptr<DAGNode> SymbolManager::resolveTerm(const std::string& name, const ResolveScope& scope) const {
    auto it_ph = placeholder_var_names_.find(name);
    if (it_ph != placeholder_var_names_.end())
        return it_ph->second;

    if (scope.check_preserving_let && !scope.preserving_let_name.empty()) {
        auto it = preserving_let_key_map_.find(scope.preserving_let_name);
        if (it != preserving_let_key_map_.end())
            return it->second;
    }
    if (scope.check_let) {
        auto it = let_key_map_.find(name);
        if (it != let_key_map_.end())
            return it->second;
    }

    // Function parameters (fun_var) are term bindings inside function bodies
    auto it_fv = fun_var_map_.find(name);
    if (it_fv != fun_var_map_.end())
        return it_fv->second;

    if (scope.in_quantifier_scope) {
        auto it_q = quant_var_map_.find(name);
        if (it_q != quant_var_map_.end())
            return it_q->second;
    }

    auto it_var = var_names_.find(name);
    if (it_var != var_names_.end())
        return it_var->second;
    if (name.size() > 2 && name[0] == '|' && name[name.size() - 1] == '|') {
        std::string inner = name.substr(1, name.size() - 2);
        auto it = var_names_.find(inner);
        if (it != var_names_.end())
            return it->second;
    }
    std::string bar_name = '|' + name + '|';
    auto it_bar = var_names_.find(bar_name);
    if (it_bar != var_names_.end())
        return it_bar->second;
    return nullptr;
}

std::shared_ptr<DAGNode> SymbolManager::resolveFun(const std::string& name) const {
    auto it_fun = fun_key_map_.find(name);
    if (it_fun != fun_key_map_.end())
        return it_fun->second;
    auto it_fv = fun_var_map_.find(name);
    if (it_fv != fun_var_map_.end())
        return it_fv->second;
    return nullptr;
}

std::shared_ptr<Sort> SymbolManager::resolveSort(const std::string& name) const {
    auto it = sort_key_map_.find(name);
    return it != sort_key_map_.end() ? it->second : nullptr;
}

void SymbolManager::registerLet(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    let_key_map_[name] = node;
}

void SymbolManager::popLetScope(const std::vector<std::string>& keys) {
    for (const auto& k : keys) let_key_map_.erase(k);
}

bool SymbolManager::hasLet(const std::string& name) const {
    return let_key_map_.find(name) != let_key_map_.end();
}

void SymbolManager::registerPreservingLet(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    preserving_let_key_map_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getPreservingLet(const std::string& name) const {
    auto it = preserving_let_key_map_.find(name);
    return it != preserving_let_key_map_.end() ? it->second : nullptr;
}

void SymbolManager::erasePreservingLet(const std::string& key) {
    preserving_let_key_map_.erase(key);
}

void SymbolManager::erasePreservingLetKeys(const std::vector<std::string>& keys) {
    for (const auto& k : keys) preserving_let_key_map_.erase(k);
}

bool SymbolManager::hasPreservingLet(const std::string& name) const {
    return preserving_let_key_map_.find(name) != preserving_let_key_map_.end();
}

void SymbolManager::registerFun(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    fun_key_map_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getFun(const std::string& name) const {
    auto it = fun_key_map_.find(name);
    return it != fun_key_map_.end() ? it->second : nullptr;
}

bool SymbolManager::hasFun(const std::string& name) const {
    return fun_key_map_.find(name) != fun_key_map_.end();
}

void SymbolManager::registerFunVar(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    fun_var_map_[name] = node;
}

void SymbolManager::eraseFunVar(const std::string& key) {
    fun_var_map_.erase(key);
}

bool SymbolManager::hasFunVar(const std::string& name) const {
    return fun_var_map_.find(name) != fun_var_map_.end();
}

void SymbolManager::registerSort(const std::string& name, const std::shared_ptr<Sort>& sort) {
    sort_key_map_[name] = sort;
}

bool SymbolManager::hasSort(const std::string& name) const {
    return sort_key_map_.find(name) != sort_key_map_.end();
}

void SymbolManager::removeSort(const std::string& name) {
    sort_key_map_.erase(name);
}

void SymbolManager::registerQuantVar(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    quant_var_map_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getQuantVar(const std::string& name) const {
    auto it = quant_var_map_.find(name);
    return it != quant_var_map_.end() ? it->second : nullptr;
}

void SymbolManager::popQuantScope(const std::vector<std::string>& keys) {
    for (const auto& k : keys) quant_var_map_.erase(k);
}

bool SymbolManager::hasQuantVar(const std::string& name) const {
    return quant_var_map_.find(name) != quant_var_map_.end();
}

void SymbolManager::registerVar(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    var_names_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getVar(const std::string& name) const {
    auto it = var_names_.find(name);
    return it != var_names_.end() ? it->second : nullptr;
}

bool SymbolManager::hasVar(const std::string& name) const {
    return var_names_.find(name) != var_names_.end();
}

void SymbolManager::removeVar(const std::string& name) {
    var_names_.erase(name);
}

void SymbolManager::renameVar(const std::string& old_name, const std::string& new_name) {
    auto it = var_names_.find(old_name);
    if (it != var_names_.end()) {
        auto node = it->second;
        var_names_.erase(it);
        var_names_[new_name] = node;
    }
}

const std::unordered_map<std::string, std::shared_ptr<DAGNode>>& SymbolManager::getVarNames() const {
    return var_names_;
}

size_t SymbolManager::nextTempVarCounter() {
    return temp_var_counter_++;
}

void SymbolManager::registerTempVar(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    temp_var_names_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getTempVar(const std::string& name) const {
    auto it = temp_var_names_.find(name);
    return it != temp_var_names_.end() ? it->second : nullptr;
}

bool SymbolManager::hasTempVar(const std::string& name) const {
    return temp_var_names_.find(name) != temp_var_names_.end();
}

void SymbolManager::renameTempVar(const std::string& old_name, const std::string& new_name) {
    auto it = temp_var_names_.find(old_name);
    if (it != temp_var_names_.end()) {
        auto node = it->second;
        temp_var_names_.erase(it);
        temp_var_names_[new_name] = node;
    }
}

const std::unordered_map<std::string, std::shared_ptr<DAGNode>>& SymbolManager::getTempVarNames() const {
    return temp_var_names_;
}

void SymbolManager::registerPlaceholderVar(const std::string& name, const std::shared_ptr<DAGNode>& node) {
    placeholder_var_names_[name] = node;
}

std::shared_ptr<DAGNode> SymbolManager::getPlaceholderVar(const std::string& name) const {
    auto it = placeholder_var_names_.find(name);
    return it != placeholder_var_names_.end() ? it->second : nullptr;
}

bool SymbolManager::hasPlaceholderVar(const std::string& name) const {
    return placeholder_var_names_.find(name) != placeholder_var_names_.end();
}

void SymbolManager::addFunctionName(const std::string& name) {
    function_names_.emplace_back(name);
}

const std::vector<std::string>& SymbolManager::getFunctionNames() const {
    return function_names_;
}

std::vector<std::shared_ptr<DAGNode>> SymbolManager::getFunctions() const {
    std::vector<std::shared_ptr<DAGNode>> result;
    result.reserve(function_names_.size());
    for (const auto& name : function_names_) {
        auto it = fun_key_map_.find(name);
        result.push_back(it != fun_key_map_.end() ? it->second : nullptr);
    }
    return result;
}

bool SymbolManager::hasFunctionName(const std::string& name) const {
    return std::find(function_names_.begin(), function_names_.end(), name) != function_names_.end();
}

void SymbolManager::removeFunctionName(const std::string& name) {
    auto it = std::find(function_names_.begin(), function_names_.end(), name);
    if (it != function_names_.end()) function_names_.erase(it);
}

void SymbolManager::addStaticFunction(const std::shared_ptr<DAGNode>& node) {
    static_functions_.emplace_back(node);
}

const std::vector<std::shared_ptr<DAGNode>>& SymbolManager::getStaticFunctions() const {
    return static_functions_;
}

const std::unordered_map<std::string, std::shared_ptr<Sort>>& SymbolManager::getSortKeyMap() const {
    return sort_key_map_;
}

void SymbolManager::removeFun(const std::string& name) {
    fun_key_map_.erase(name);
    removeFunctionName(name);
}

} // namespace SOMTParser
