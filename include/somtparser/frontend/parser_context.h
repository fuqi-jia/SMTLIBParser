/* -*- Header -*-
 *
 * ParserContext: frontend context with SymbolManager, ObjectiveManager and parser state.
 *
 * Extends Context with symbol_manager_, objective_manager_, and assertions/assumptions.
 * Used by Parser only; passes (Rewriter, etc.) depend only on Context.
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef PARSER_CONTEXT_HEADER
#define PARSER_CONTEXT_HEADER

#include "somtparser/context/context.h"
#include "somtparser/ir/dag.h"
#include "somtparser/frontend/objective.h"
#include "somtparser/frontend/symbol_manager.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace SOMTParser {

/**
 * Context implementation that holds parser data (symbols, objectives, assertions, etc.).
 * Inherits NodeManager, SortManager, Options from Context; adds SymbolManager, ObjectiveManager.
 */
class ParserContext : public Context {
protected:
    std::shared_ptr<SymbolManager>   symbol_manager_;
    std::shared_ptr<ObjectiveManager> objective_manager_;

public:
    void setSymbolManager(std::shared_ptr<SymbolManager> sm) { symbol_manager_ = std::move(sm); }
    void setObjectiveManager(std::shared_ptr<ObjectiveManager> om) { objective_manager_ = std::move(om); }

    std::shared_ptr<SymbolManager> getSymbolManager() { return symbol_manager_; }
    std::shared_ptr<SymbolManager> getSymbolManager() const { return symbol_manager_; }
    std::shared_ptr<ObjectiveManager> getObjectiveManager() { return objective_manager_; }
    std::shared_ptr<ObjectiveManager> getObjectiveManager() const { return objective_manager_; }

public:
    std::vector<std::shared_ptr<DAGNode>> assertions;
    std::unordered_map<std::string, std::unordered_set<size_t>> assertion_groups;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> named_assertions;
    std::vector<std::vector<std::shared_ptr<DAGNode>>> assumptions;
    std::vector<std::shared_ptr<DAGNode>> soft_assertions;
    std::vector<std::shared_ptr<DAGNode>> soft_weights;
    std::unordered_map<std::string, std::unordered_set<size_t>> soft_assertion_groups;
    std::vector<std::shared_ptr<DAGNode>> split_lemmas;

    std::vector<std::shared_ptr<DAGNode>> getAssertions() const;
    std::unordered_map<std::string, std::unordered_set<size_t>> getGroupedAssertions() const;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> getNamedAssertions() const;
    std::vector<std::vector<std::shared_ptr<DAGNode>>> getAssumptions() const;
    std::vector<std::shared_ptr<DAGNode>> getSoftAssertions() const;
    std::vector<std::shared_ptr<DAGNode>> getSoftWeights() const;
    std::unordered_map<std::string, std::unordered_set<size_t>> getGroupedSoftAssertions() const;
    std::vector<std::shared_ptr<Objective>> getObjectives() const;
    std::vector<std::shared_ptr<DAGNode>> getSplitLemmas() const;
};

} // namespace SOMTParser

#endif
