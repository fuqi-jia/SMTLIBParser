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

} // namespace SOMTParser
