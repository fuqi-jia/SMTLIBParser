/* -*- C++ -*-
 * ParserContext implementations
 */

#include "somtparser/frontend/parser_context.h"

namespace SOMTParser {

std::vector<std::shared_ptr<DAGNode>> ParserContext::getAssertions() const {
    return assertions;
}

std::unordered_map<std::string, std::unordered_set<size_t>> ParserContext::getGroupedAssertions() const {
    return assertion_groups;
}

std::unordered_map<std::string, std::shared_ptr<DAGNode>> ParserContext::getNamedAssertions() const {
    return named_assertions;
}

std::vector<std::vector<std::shared_ptr<DAGNode>>> ParserContext::getAssumptions() const {
    return assumptions;
}

std::vector<std::shared_ptr<DAGNode>> ParserContext::getSoftAssertions() const {
    return soft_assertions;
}

std::vector<std::shared_ptr<DAGNode>> ParserContext::getSoftWeights() const {
    return soft_weights;
}

std::unordered_map<std::string, std::unordered_set<size_t>> ParserContext::getGroupedSoftAssertions() const {
    return soft_assertion_groups;
}

std::vector<std::shared_ptr<Objective>> ParserContext::getObjectives() const {
    auto om = getObjectiveManager();
    return om ? om->getObjectives() : std::vector<std::shared_ptr<Objective>>{};
}

std::vector<std::shared_ptr<DAGNode>> ParserContext::getSplitLemmas() const {
    return split_lemmas;
}

// --- Incremental push/pop scope stack ---

void ParserContext::pushScope(size_t n) {
    for (size_t i = 0; i < n; ++i) {
        ScopeFrame frame;
        frame.assertions_size = assertions.size();
        frame.assumptions_size = assumptions.size();
        frame.soft_assertions_size = soft_assertions.size();
        frame.soft_weights_size = soft_weights.size();
        frame.objectives_size = getObjectiveManager() ? getObjectiveManager()->getObjectives().size() : 0;
        frame.split_lemmas_size = split_lemmas.size();
        scope_stack_.push_back(std::move(frame));
    }
}

void ParserContext::popScope(size_t n) {
    if (n > scope_stack_.size()) {
        // SMT-LIB error: pop underflow. For now, pop all available levels.
        n = scope_stack_.size();
    }
    for (size_t i = 0; i < n; ++i) {
        const ScopeFrame& frame = scope_stack_.back();

        // Truncate vectors to recorded sizes
        assertions.resize(frame.assertions_size);
        assumptions.resize(frame.assumptions_size);
        soft_assertions.resize(frame.soft_assertions_size);
        soft_weights.resize(frame.soft_weights_size);
        split_lemmas.resize(frame.split_lemmas_size);

        // Remove added symbols from SymbolManager
        if (auto sm = getSymbolManager()) {
            for (const auto& name : frame.added_vars) sm->removeVar(name);
            for (const auto& name : frame.added_funs) sm->removeFun(name);
            for (const auto& name : frame.added_sorts) sm->removeSort(name);
        }

        // Remove added named assertions
        for (const auto& key : frame.added_named_assertion_keys) {
            named_assertions.erase(key);
        }

        // Remove added assertion group indices
        for (const auto& key : frame.added_assertion_group_keys) {
            auto it = assertion_groups.find(key);
            if (it != assertion_groups.end()) {
                // Remove indices >= frame.assertions_size from this group
                std::unordered_set<size_t> new_set;
                for (size_t idx : it->second) {
                    if (idx < frame.assertions_size) new_set.insert(idx);
                }
                if (new_set.empty()) assertion_groups.erase(it);
                else it->second = std::move(new_set);
            }
        }

        // Remove added soft assertion group indices
        for (const auto& key : frame.added_soft_assertion_group_keys) {
            auto it = soft_assertion_groups.find(key);
            if (it != soft_assertion_groups.end()) {
                std::unordered_set<size_t> new_set;
                for (size_t idx : it->second) {
                    if (idx < frame.soft_assertions_size) new_set.insert(idx);
                }
                if (new_set.empty()) soft_assertion_groups.erase(it);
                else it->second = std::move(new_set);
            }
        }

        // Truncate objectives
        if (auto om = getObjectiveManager()) {
            om->popToSize(frame.objectives_size);
        }

        scope_stack_.pop_back();
    }
}

void ParserContext::resetAssertions() {
    assertions.clear();
    assumptions.clear();
    soft_assertions.clear();
    soft_weights.clear();
    assertion_groups.clear();
    soft_assertion_groups.clear();
    named_assertions.clear();
    split_lemmas.clear();
    // Note: scope_stack_ is NOT cleared; declarations and options are kept.
}

void ParserContext::resetAll() {
    resetAssertions();
    scope_stack_.clear();
    if (auto sm = getSymbolManager()) {
        // SymbolManager has no clear() method; we rely on the shared_ptr being reset
        // by the Parser's init or by creating a new SymbolManager.
        // For a full reset, we clear all maps manually if accessible.
        // Since SymbolManager internals are private, the Parser should recreate managers.
    }
    if (auto om = getObjectiveManager()) {
        om->clear();
    }
}

// --- Scope recording helpers (no-op if no active scope) ---

void ParserContext::registerVarInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_vars.push_back(name);
}

void ParserContext::registerFunInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_funs.push_back(name);
}

void ParserContext::registerSortInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_sorts.push_back(name);
}

void ParserContext::registerNamedAssertionInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_named_assertion_keys.push_back(name);
}

void ParserContext::registerAssertionGroupInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_assertion_group_keys.push_back(name);
}

void ParserContext::registerSoftAssertionGroupInScope(const std::string& name) {
    if (!scope_stack_.empty()) scope_stack_.back().added_soft_assertion_group_keys.push_back(name);
}

} // namespace SOMTParser
