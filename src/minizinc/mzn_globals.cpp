/* -*- C++ -*-
 *
 * MiniZinc Frontend — Global Constraint Decomposer Library Implementation
 */

#include "somtparser/minizinc/mzn_globals.h"
#include "somtparser/minizinc/mzn_lower_smt.h"
#include "somtparser/frontend/parser.h"

namespace SOMTParser::MiniZinc {

// ── Constructor ──────────────────────────────────────────────────
GlobalConstraintDecomposer::GlobalConstraintDecomposer(SmtLoweringBackend& backend)
    : backend_(backend) {}

// ── Main entry point ─────────────────────────────────────────────
std::vector<NodePtr> GlobalConstraintDecomposer::decompose(
    const std::string& name,
    const std::vector<ExprPtr>& args) {

    if (name == "all_different") return decompose_all_different(args);
    if (name == "all_equal") return decompose_all_equal(args);
    if (name == "all_different_except_0") return decompose_all_different_except_0(args);
    if (name == "count") return decompose_count(args);
    if (name == "count_eq") return decompose_count_eq(args);
    if (name == "global_cardinality") return decompose_global_cardinality(args);
    if (name == "global_cardinality_closed") return decompose_global_cardinality_closed(args);
    if (name == "cumulative") return decompose_cumulative(args);
    if (name == "disjunctive") return decompose_disjunctive(args);
    if (name == "element") return decompose_element(args);
    if (name == "increasing") return decompose_increasing(args);
    if (name == "decreasing") return decompose_decreasing(args);
    if (name == "strictly_increasing") return decompose_strictly_increasing(args);
    if (name == "strictly_decreasing") return decompose_strictly_decreasing(args);
    if (name == "sort") return decompose_sort(args);
    if (name == "arg_min") return decompose_arg_min(args);
    if (name == "arg_max") return decompose_arg_max(args);
    if (name == "circuit") return decompose_circuit(args);
    if (name == "table") return decompose_table(args);
    if (name == "regular") return decompose_regular(args);
    if (name == "lex_less") return decompose_lex_less(args);
    if (name == "lex_lesseq") return decompose_lex_lesseq(args);
    if (name == "nvalue") return decompose_nvalue(args);
    if (name == "diffn") return decompose_diffn(args);
    if (name == "at_least") return decompose_at_least(args);
    if (name == "at_most") return decompose_at_most(args);
    if (name == "exactly") return decompose_exactly(args);
    if (name == "among") return decompose_among(args);
    if (name == "inverse") return decompose_inverse(args);
    if (name == "member") return decompose_member(args);
    if (name == "bin_packing") return decompose_bin_packing(args);
    if (name == "bin_packing_capa") return decompose_bin_packing_capa(args);
    if (name == "bin_packing_load") return decompose_bin_packing_load(args);
    if (name == "soft_all_different") return decompose_soft_all_different(args);
    if (name == "value_precede") return decompose_value_precede(args);

    if (name == "redundant_constraint") return ignore_redundant(args);
    if (name == "symmetry_breaking_constraint") return ignore_symmetry_breaking(args);
    if (name == "implied_constraint") return pass_through_implied(args);

    // Fallback: mark as unsupported
    return {};
}

// ── Helpers ──────────────────────────────────────────────────────
std::vector<NodePtr> GlobalConstraintDecomposer::flattenArray(const ExprPtr& arr_expr) {
    std::vector<NodePtr> result;
    auto lowered = backend_.lowerExpr(arr_expr);
    if (!lowered) return result;
    // TODO: if lowered is an array variable, we need to know its length
    // For now, assume arr_expr is an array literal
    auto* arr_lit = arr_expr->as<ArrayLit>();
    if (arr_lit) {
        for (auto& e : arr_lit->elements) {
            auto le = backend_.lowerExpr(e);
            if (le) result.push_back(le);
        }
    }
    return result;
}

NodePtr GlobalConstraintDecomposer::freshBoolVar(const std::string& hint) {
    static int counter = 0;
    return backend_.getParser().mkVarBool(hint + "_" + std::to_string(counter++));
}

NodePtr GlobalConstraintDecomposer::freshIntVar(const std::string& hint,
                                                 NodePtr lb, NodePtr ub) {
    static int counter = 0;
    auto v = backend_.getParser().mkVarInt(hint + "_" + std::to_string(counter++));
    // Add bounds as assertions via backend_ ... but we can't easily add assertions here
    // For now, return the variable without bounds
    (void)lb; (void)ub;
    return v;
}

// ── Core decomposers ─────────────────────────────────────────────
std::vector<NodePtr> GlobalConstraintDecomposer::decompose_all_different(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 0; i < elems.size(); ++i) {
        for (size_t j = i + 1; j < elems.size(); ++j) {
            result.push_back(backend_.getParser().mkDistinct(elems[i], elems[j]));
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_all_equal(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
        result.push_back(backend_.getParser().mkEq(elems[0], elems[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_all_different_except_0(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    auto zero = backend_.getParser().mkConstInt(0);
    for (size_t i = 0; i < elems.size(); ++i) {
        for (size_t j = i + 1; j < elems.size(); ++j) {
            auto ei_zero = backend_.getParser().mkEq(elems[i], zero);
            auto ej_zero = backend_.getParser().mkEq(elems[j], zero);
            auto diff = backend_.getParser().mkDistinct(elems[i], elems[j]);
            auto disj = backend_.getParser().mkOr(
                backend_.getParser().mkOr(ei_zero, ej_zero), diff);
            result.push_back(disj);
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_count(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto elems = flattenArray(args[0]);
    auto target = backend_.lowerExpr(args[1]);
    if (!target) return result;
    std::vector<NodePtr> bools;
    for (auto& e : elems) {
        bools.push_back(backend_.getParser().mkEq(e, target));
    }
    if (args.size() >= 3) {
        auto count_var = backend_.lowerExpr(args[2]);
        if (count_var) {
            // sum(bool2int(eq)) = count_var
            std::vector<NodePtr> ints;
            for (auto& b : bools) {
                ints.push_back(backend_.getParser().mkIte(
                    b, backend_.getParser().mkConstInt(1),
                    backend_.getParser().mkConstInt(0)));
            }
            auto sum = backend_.getParser().mkAdd(ints);
            result.push_back(backend_.getParser().mkEq(sum, count_var));
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_count_eq(
    const std::vector<ExprPtr>& args) {
    return decompose_count(args);
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_global_cardinality(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 3) return result;
    auto elems = flattenArray(args[0]);
    auto cover = flattenArray(args[1]);
    auto counts = flattenArray(args[2]);
    if (cover.size() != counts.size()) return result;
    for (size_t i = 0; i < cover.size(); ++i) {
        std::vector<NodePtr> bools;
        for (auto& e : elems) {
            bools.push_back(backend_.getParser().mkEq(e, cover[i]));
        }
        std::vector<NodePtr> ints;
        for (auto& b : bools) {
            ints.push_back(backend_.getParser().mkIte(
                b, backend_.getParser().mkConstInt(1),
                backend_.getParser().mkConstInt(0)));
        }
        auto sum = backend_.getParser().mkAdd(ints);
        result.push_back(backend_.getParser().mkEq(sum, counts[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_global_cardinality_closed(
    const std::vector<ExprPtr>& args) {
    auto result = decompose_global_cardinality(args);
    if (args.size() < 2) return result;
    auto elems = flattenArray(args[0]);
    auto cover = flattenArray(args[1]);
    for (auto& e : elems) {
        std::vector<NodePtr> in_cover;
        for (auto& c : cover) {
            in_cover.push_back(backend_.getParser().mkEq(e, c));
        }
        result.push_back(backend_.getParser().mkOr(in_cover));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_cumulative(
    const std::vector<ExprPtr>& args) {
    (void)args;
    // Phase 2+: resource scheduling constraints
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_disjunctive(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto starts = flattenArray(args[0]);
    auto durations = flattenArray(args[1]);
    if (starts.size() != durations.size()) return result;
    for (size_t i = 0; i < starts.size(); ++i) {
        for (size_t j = i + 1; j < starts.size(); ++j) {
            auto si_plus_di = backend_.getParser().mkAdd(starts[i], durations[i]);
            auto sj_plus_dj = backend_.getParser().mkAdd(starts[j], durations[j]);
            auto left = backend_.getParser().mkLe(si_plus_di, starts[j]);
            auto right = backend_.getParser().mkLe(sj_plus_dj, starts[i]);
            result.push_back(backend_.getParser().mkOr(left, right));
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_element(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 3) return result;
    auto idx = backend_.lowerExpr(args[0]);
    auto arr = backend_.lowerExpr(args[1]);
    auto val = backend_.lowerExpr(args[2]);
    if (idx && arr && val) {
        auto selected = backend_.getParser().mkSelect(arr, idx);
        result.push_back(backend_.getParser().mkEq(selected, val));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_increasing(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
        result.push_back(backend_.getParser().mkLe(elems[i - 1], elems[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_decreasing(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
        result.push_back(backend_.getParser().mkGe(elems[i - 1], elems[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_strictly_increasing(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
        result.push_back(backend_.getParser().mkLt(elems[i - 1], elems[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_strictly_decreasing(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto elems = flattenArray(args[0]);
    for (size_t i = 1; i < elems.size(); ++i) {
        result.push_back(backend_.getParser().mkGt(elems[i - 1], elems[i]));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_sort(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto sorted = flattenArray(args[1]);
    // all_different(sorted) + increasing(sorted)
    auto ad = decompose_all_different({args[1]});
    result.insert(result.end(), ad.begin(), ad.end());
    auto inc = decompose_increasing({args[1]});
    result.insert(result.end(), inc.begin(), inc.end());
    // count equality: forall(v) count(arr,v) = count(sorted,v)
    // Simplified: omitted for Phase 1
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_arg_min(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_arg_max(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_circuit(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.empty()) return result;
    auto next = flattenArray(args[0]);
    // all_different(next)
    auto ad = decompose_all_different(args);
    result.insert(result.end(), ad.begin(), ad.end());
    // MTZ sub-tour elimination (simplified)
    // TODO: add u[i] variables and ordering constraints
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_table(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto vars = flattenArray(args[0]);
    auto* tuples = args[1]->as<ArrayLit>();
    if (!tuples) return result;
    std::vector<NodePtr> or_terms;
    for (auto& t_expr : tuples->elements) {
        auto* tuple = t_expr->as<ArrayLit>();
        if (!tuple || tuple->elements.size() != vars.size()) continue;
        std::vector<NodePtr> and_terms;
        for (size_t i = 0; i < vars.size(); ++i) {
            auto val = backend_.lowerExpr(tuple->elements[i]);
            if (val) and_terms.push_back(backend_.getParser().mkEq(vars[i], val));
        }
        if (!and_terms.empty()) {
            or_terms.push_back(backend_.getParser().mkAnd(and_terms));
        }
    }
    if (!or_terms.empty()) {
        result.push_back(backend_.getParser().mkOr(or_terms));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_regular(
    const std::vector<ExprPtr>& args) {
    (void)args;
    // Phase 2+: DFA state-machine encoding
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_lex_less(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto a = flattenArray(args[0]);
    auto b = flattenArray(args[1]);
    size_t n = std::min(a.size(), b.size());
    // a < b  <=>  exists(i) (a[0..i-1]=b[0..i-1] && a[i] < b[i])
    std::vector<NodePtr> disjuncts;
    for (size_t i = 0; i < n; ++i) {
        std::vector<NodePtr> prefix_eq;
        for (size_t j = 0; j < i; ++j) {
            prefix_eq.push_back(backend_.getParser().mkEq(a[j], b[j]));
        }
        prefix_eq.push_back(backend_.getParser().mkLt(a[i], b[i]));
        disjuncts.push_back(backend_.getParser().mkAnd(prefix_eq));
    }
    if (!disjuncts.empty()) {
        result.push_back(backend_.getParser().mkOr(disjuncts));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_lex_lesseq(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto a = flattenArray(args[0]);
    auto b = flattenArray(args[1]);
    size_t n = std::min(a.size(), b.size());
    // a <= b  <=>  a=b \/ a<b
    std::vector<NodePtr> all_eq;
    for (size_t i = 0; i < n; ++i) {
        all_eq.push_back(backend_.getParser().mkEq(a[i], b[i]));
    }
    auto eq_case = backend_.getParser().mkAnd(all_eq);
    auto less = decompose_lex_less(args);
    if (!less.empty()) {
        result.push_back(backend_.getParser().mkOr(eq_case, less[0]));
    } else {
        result.push_back(eq_case);
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_nvalue(
    const std::vector<ExprPtr>& args) {
    (void)args;
    // n = card(unique_values) - complex encoding
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_diffn(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 4) return result;
    auto x = flattenArray(args[0]);
    auto y = flattenArray(args[1]);
    auto dx = flattenArray(args[2]);
    auto dy = flattenArray(args[3]);
    size_t n = std::min({x.size(), y.size(), dx.size(), dy.size()});
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = i + 1; j < n; ++j) {
            auto xi_end = backend_.getParser().mkAdd(x[i], dx[i]);
            auto xj_end = backend_.getParser().mkAdd(x[j], dx[j]);
            auto yi_end = backend_.getParser().mkAdd(y[i], dy[i]);
            auto yj_end = backend_.getParser().mkAdd(y[j], dy[j]);
            auto left = backend_.getParser().mkLe(xi_end, x[j]);
            auto right = backend_.getParser().mkLe(xj_end, x[i]);
            auto above = backend_.getParser().mkLe(yi_end, y[j]);
            auto below = backend_.getParser().mkLe(yj_end, y[i]);
            result.push_back(backend_.getParser().mkOr(
                backend_.getParser().mkOr(left, right),
                backend_.getParser().mkOr(above, below)));
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_at_least(
    const std::vector<ExprPtr>& args) {
    if (args.size() < 3) return {};
    auto n = backend_.lowerExpr(args[0]);
    auto arr = args[1];
    auto v = backend_.lowerExpr(args[2]);
    if (!n || !v) return {};
    auto elems = flattenArray(arr);
    std::vector<NodePtr> bools;
    for (auto& e : elems) {
        bools.push_back(backend_.getParser().mkEq(e, v));
    }
    std::vector<NodePtr> ints;
    for (auto& b : bools) {
        ints.push_back(backend_.getParser().mkIte(
            b, backend_.getParser().mkConstInt(1),
            backend_.getParser().mkConstInt(0)));
    }
    auto sum = backend_.getParser().mkAdd(ints);
    return {backend_.getParser().mkGe(sum, n)};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_at_most(
    const std::vector<ExprPtr>& args) {
    if (args.size() < 3) return {};
    auto n = backend_.lowerExpr(args[0]);
    auto arr = args[1];
    auto v = backend_.lowerExpr(args[2]);
    if (!n || !v) return {};
    auto elems = flattenArray(arr);
    std::vector<NodePtr> bools;
    for (auto& e : elems) {
        bools.push_back(backend_.getParser().mkEq(e, v));
    }
    std::vector<NodePtr> ints;
    for (auto& b : bools) {
        ints.push_back(backend_.getParser().mkIte(
            b, backend_.getParser().mkConstInt(1),
            backend_.getParser().mkConstInt(0)));
    }
    auto sum = backend_.getParser().mkAdd(ints);
    return {backend_.getParser().mkLe(sum, n)};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_exactly(
    const std::vector<ExprPtr>& args) {
    if (args.size() < 3) return {};
    auto n = backend_.lowerExpr(args[0]);
    auto arr = args[1];
    auto v = backend_.lowerExpr(args[2]);
    if (!n || !v) return {};
    auto elems = flattenArray(arr);
    std::vector<NodePtr> bools;
    for (auto& e : elems) {
        bools.push_back(backend_.getParser().mkEq(e, v));
    }
    std::vector<NodePtr> ints;
    for (auto& b : bools) {
        ints.push_back(backend_.getParser().mkIte(
            b, backend_.getParser().mkConstInt(1),
            backend_.getParser().mkConstInt(0)));
    }
    auto sum = backend_.getParser().mkAdd(ints);
    return {backend_.getParser().mkEq(sum, n)};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_among(
    const std::vector<ExprPtr>& args) {
    if (args.size() < 3) return {};
    auto n = backend_.lowerExpr(args[0]);
    auto arr = args[1];
    auto set_vals = flattenArray(args[2]);
    if (!n) return {};
    auto elems = flattenArray(arr);
    std::vector<NodePtr> bools;
    for (auto& e : elems) {
        std::vector<NodePtr> in_set;
        for (auto& s : set_vals) {
            in_set.push_back(backend_.getParser().mkEq(e, s));
        }
        bools.push_back(backend_.getParser().mkOr(in_set));
    }
    std::vector<NodePtr> ints;
    for (auto& b : bools) {
        ints.push_back(backend_.getParser().mkIte(
            b, backend_.getParser().mkConstInt(1),
            backend_.getParser().mkConstInt(0)));
    }
    auto sum = backend_.getParser().mkAdd(ints);
    return {backend_.getParser().mkEq(sum, n)};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_inverse(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto f = flattenArray(args[0]);
    auto invf = flattenArray(args[1]);
    if (f.size() != invf.size()) return result;
    size_t n = f.size();
    auto one = backend_.getParser().mkConstInt(1);
    for (size_t i = 0; i < n; ++i) {
        auto idx_i = backend_.getParser().mkConstInt(static_cast<int>(i));
        for (size_t j = 0; j < n; ++j) {
            auto idx_j = backend_.getParser().mkConstInt(static_cast<int>(j));
            auto fi_eq_j = backend_.getParser().mkEq(f[i], idx_j);
            auto invfj_eq_i = backend_.getParser().mkEq(invf[j], idx_i);
            result.push_back(backend_.getParser().mkEq(fi_eq_j, invfj_eq_i));
        }
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_member(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    if (args.size() < 2) return result;
    auto elems = flattenArray(args[0]);
    auto val = backend_.lowerExpr(args[1]);
    if (!val) return result;
    std::vector<NodePtr> disjuncts;
    for (auto& e : elems) {
        disjuncts.push_back(backend_.getParser().mkEq(e, val));
    }
    if (!disjuncts.empty()) {
        result.push_back(backend_.getParser().mkOr(disjuncts));
    }
    return result;
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_bin_packing(
    const std::vector<ExprPtr>& args) {
    if (args.size() < 3) return {};
    auto capacity = backend_.lowerExpr(args[0]);
    auto bins = flattenArray(args[1]);
    auto weights = flattenArray(args[2]);
    if (!capacity || bins.size() != weights.size()) return {};
    // Simplified: not fully decomposed without knowing number of bins
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_bin_packing_capa(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_bin_packing_load(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_soft_all_different(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::decompose_value_precede(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

// ── Ignored / pass-through ───────────────────────────────────────
std::vector<NodePtr> GlobalConstraintDecomposer::ignore_redundant(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::ignore_symmetry_breaking(
    const std::vector<ExprPtr>& args) {
    (void)args;
    return {};
}

std::vector<NodePtr> GlobalConstraintDecomposer::pass_through_implied(
    const std::vector<ExprPtr>& args) {
    std::vector<NodePtr> result;
    for (auto& a : args) {
        auto lowered = backend_.lowerExpr(a);
        if (lowered) result.push_back(lowered);
    }
    return result;
}

} // namespace SOMTParser::MiniZinc
