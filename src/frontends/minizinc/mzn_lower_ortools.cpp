/* -*- C++ -*-
 *
 * MiniZinc Frontend — OR-Tools CP-SAT Lowering Backend (Reserved)
 *
 * Phase 7 extension stub.
 */

#include "somtparser/frontends/minizinc/mzn_lower_ortools.h"

namespace SOMTParser::MiniZinc {

#ifdef SOMTPARSER_ENABLE_ORTOOLS

// PImpl pattern to hide OR-Tools dependency from this translation unit.
class OrToolsLoweringBackend::Impl {
public:
    std::vector<std::string> errors;
};

OrToolsLoweringBackend::OrToolsLoweringBackend()
    : pImpl(std::make_unique<Impl>()) {}

OrToolsLoweringBackend::~OrToolsLoweringBackend() = default;

std::shared_ptr<Sort> OrToolsLoweringBackend::lowerType(const TypeInst& ti) {
    (void)ti;
    return nullptr;
}

std::shared_ptr<DAGNode> OrToolsLoweringBackend::lowerExpr(const ExprPtr& expr) {
    (void)expr;
    return nullptr;
}

std::shared_ptr<DAGNode> OrToolsLoweringBackend::lowerVarDecl(
    const VarDeclItem& decl,
    std::vector<std::shared_ptr<DAGNode>>& out_assertions) {
    (void)decl; (void)out_assertions;
    return nullptr;
}

std::shared_ptr<DAGNode> OrToolsLoweringBackend::lowerConstraint(const ConstraintItem& ci) {
    (void)ci;
    return nullptr;
}

void OrToolsLoweringBackend::lowerSolveItem(const SolveItem& si) {
    (void)si;
}

void OrToolsLoweringBackend::lowerOutputItem(const OutputItem& oi) {
    (void)oi;
}

std::vector<std::shared_ptr<DAGNode>> OrToolsLoweringBackend::decomposeGlobal(
    const std::string& name,
    const std::vector<ExprPtr>& args) {
    (void)name; (void)args;
    return {};
}

void OrToolsLoweringBackend::finalize() {}

bool OrToolsLoweringBackend::hadErrors() const {
    return !pImpl->errors.empty();
}

std::vector<std::string> OrToolsLoweringBackend::getErrors() const {
    return pImpl->errors;
}

#endif // SOMTPARSER_ENABLE_ORTOOLS

} // namespace SOMTParser::MiniZinc
