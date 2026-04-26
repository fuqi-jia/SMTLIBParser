/* -*- Header -*-
 *
 * MiniZinc Frontend — Global Constraint Decomposer Library
 *
 * Decomposes MiniZinc global constraints into primitive SMT assertions.
 */

#ifndef MZN_GLOBALS_H
#define MZN_GLOBALS_H

#include "somtparser/minizinc/mzn_ast.h"
#include <memory>
#include <vector>
#include <string>

namespace SOMTParser {
    struct DAGNode;
}

namespace SOMTParser::MiniZinc {

// Forward declaration
class SmtLoweringBackend;
using NodePtr = std::shared_ptr<DAGNode>;

/**
 * @brief Decomposes global constraints for the SMT backend.
 */
class GlobalConstraintDecomposer {
public:
    explicit GlobalConstraintDecomposer(SmtLoweringBackend& backend);

    // ── Core decomposition entry point ─────────────────────────
    std::vector<NodePtr> decompose(const std::string& name,
                                   const std::vector<ExprPtr>& args);

    // ── Individual decomposers (public for testing) ────────────
    std::vector<NodePtr> decompose_all_different(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_all_equal(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_all_different_except_0(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_count(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_count_eq(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_global_cardinality(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_global_cardinality_closed(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_cumulative(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_disjunctive(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_element(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_increasing(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_decreasing(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_strictly_increasing(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_strictly_decreasing(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_sort(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_arg_min(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_arg_max(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_circuit(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_table(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_regular(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_lex_less(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_lex_lesseq(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_nvalue(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_diffn(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_at_least(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_at_most(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_exactly(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_among(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_inverse(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_member(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_bin_packing(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_bin_packing_capa(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_bin_packing_load(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_soft_all_different(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> decompose_value_precede(const std::vector<ExprPtr>& args);

    // ── Ignored / pass-through constraints ─────────────────────
    std::vector<NodePtr> ignore_redundant(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> ignore_symmetry_breaking(const std::vector<ExprPtr>& args);
    std::vector<NodePtr> pass_through_implied(const std::vector<ExprPtr>& args);

private:
    SmtLoweringBackend& backend_;

    // Helper: flatten an array Expr to a vector of lowered DAG nodes
    std::vector<NodePtr> flattenArray(const ExprPtr& arr_expr);
    // Helper: create a fresh bool/int variable
    NodePtr freshBoolVar(const std::string& hint);
    NodePtr freshIntVar(const std::string& hint, NodePtr lb, NodePtr ub);
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_GLOBALS_H
