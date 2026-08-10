/* -*- Header -*-
 *
 * SymbolManager: name resolution and scope (string -> Node/Sort).
 * Frontend layer; does not create nodes/sorts; Parser registers after creating via NodeManager/SortManager.
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef SYMBOL_MANAGER_HEADER
#define SYMBOL_MANAGER_HEADER

#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SOMTParser {

/** Scope flags for resolve (Parser decides from let/quant state). */
struct ResolveScope {
    bool check_let = false;
    bool in_quantifier_scope = false;
};

/**
 * Manager for SMT-LIB name -> IR object (symbol lookup + scope).
 * Holds let_key_map, preserving_let_key_map, fun_key_map, fun_var_map, sort_key_map,
 * quant_var_map, var_names, temp_var_*, placeholder_var_names, function_names, static_functions.
 */
class SymbolManager {
public:
    SymbolManager();

    void reserve(size_t capacity);

    // --- Resolve (lookup only; returns stored node/sort, does not use NodeManager/SortManager) ---
    /** Full resolution: placeholder -> preserving_let -> let -> fun_key -> fun_var -> quant_var -> var_names -> |x|. Returns nullptr if not found. */
    std::shared_ptr<DAGNode> resolveSymbol(const std::string& name, const ResolveScope& scope) const;
    /** Term-only: placeholder -> preserving_let -> let -> fun_var (params) -> quant_var -> var_names -> |x| (no fun_key). */
    std::shared_ptr<DAGNode> resolveTerm(const std::string& name, const ResolveScope& scope) const;
    /** Function-only: fun_key_map then fun_var_map. Returns nullptr if not found. */
    std::shared_ptr<DAGNode> resolveFun(const std::string& name) const;
    std::shared_ptr<Sort> resolveSort(const std::string& name) const;

    // --- Let scope (unified: both normal and preserving let) ---
    void pushLetScope();
    void popLetScope();
    void popLetScope(const std::vector<std::string>& keys);
    void registerLet(const std::string& name, const std::shared_ptr<DAGNode>& node);
    bool hasLet(const std::string& name) const;

    // --- Function / fun var ---
    void registerFun(const std::string& name, const std::shared_ptr<DAGNode>& node);
    std::shared_ptr<DAGNode> getFun(const std::string& name) const;
    bool hasFun(const std::string& name) const;
    void registerFunVar(const std::string& name, const std::shared_ptr<DAGNode>& node);
    void eraseFunVar(const std::string& key);
    bool hasFunVar(const std::string& name) const;

    // --- Sort (user-defined sort names) ---
    void registerSort(const std::string& name, const std::shared_ptr<Sort>& sort);
    bool hasSort(const std::string& name) const;
    void removeSort(const std::string& name);

    // --- Quantifier scope ---
    void registerQuantVar(const std::string& name, const std::shared_ptr<DAGNode>& node);
    std::shared_ptr<DAGNode> getQuantVar(const std::string& name) const;
    void popQuantScope(const std::vector<std::string>& keys);
    bool hasQuantVar(const std::string& name) const;

    // --- Global var (name -> node) ---
    void registerVar(const std::string& name, const std::shared_ptr<DAGNode>& node);
    std::shared_ptr<DAGNode> getVar(const std::string& name) const;
    bool hasVar(const std::string& name) const;
    void removeVar(const std::string& name);
    void renameVar(const std::string& old_name, const std::string& new_name);
    const std::unordered_map<std::string, std::shared_ptr<DAGNode>>& getVarNames() const;

    // --- Temp var ---
    size_t nextTempVarCounter();
    void registerTempVar(const std::string& name, const std::shared_ptr<DAGNode>& node);
    std::shared_ptr<DAGNode> getTempVar(const std::string& name) const;
    bool hasTempVar(const std::string& name) const;
    void renameTempVar(const std::string& old_name, const std::string& new_name);
    const std::unordered_map<std::string, std::shared_ptr<DAGNode>>& getTempVarNames() const;

    // --- Placeholder var ---
    void registerPlaceholderVar(const std::string& name, const std::shared_ptr<DAGNode>& node);
    std::shared_ptr<DAGNode> getPlaceholderVar(const std::string& name) const;
    bool hasPlaceholderVar(const std::string& name) const;

    // --- Function names list ---
    void addFunctionName(const std::string& name);
    const std::vector<std::string>& getFunctionNames() const;
    /** Returns function nodes in the same order as getFunctionNames(). Requires no NodeManager. */
    std::vector<std::shared_ptr<DAGNode>> getFunctions() const;
    bool hasFunctionName(const std::string& name) const;
    void removeFunctionName(const std::string& name);

    // --- Static functions ---
    void addStaticFunction(const std::shared_ptr<DAGNode>& node);
    const std::vector<std::shared_ptr<DAGNode>>& getStaticFunctions() const;

    // --- Iteration / removal (e.g. for reset or get-model) ---
    const std::unordered_map<std::string, std::shared_ptr<Sort>>& getSortKeyMap() const;
    void removeFun(const std::string& name);

    // Declaration order. The maps above are unordered, so iterating them gives
    // an order unrelated to the input -- which makes anything derived from it
    // (notably dumpSMT2) irreproducible. These record first-registration order,
    // mirroring how function_names_ already tracks functions. Names removed
    // from the corresponding map are left in place and must be skipped by the
    // caller via the map lookup.
    const std::vector<std::string>& getSortOrder() const;
    const std::vector<std::string>& getVarOrder() const;
    const std::vector<std::string>& getTempVarOrder() const;

private:
    struct LetBackup {
        std::string name;
        std::shared_ptr<DAGNode> oldNode;
        bool hadOld;
    };
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> let_key_map_;
    std::vector<LetBackup> let_scope_backup_;
    std::vector<size_t> let_scope_checkpoints_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> fun_key_map_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> fun_var_map_;
    std::unordered_map<std::string, std::shared_ptr<Sort>> sort_key_map_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> quant_var_map_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> var_names_;
    size_t temp_var_counter_ = 0;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> temp_var_names_;
    std::unordered_map<std::string, std::shared_ptr<DAGNode>> placeholder_var_names_;
    std::vector<std::string> function_names_;
    std::vector<std::shared_ptr<DAGNode>> static_functions_;
    std::vector<std::string> sort_order_;
    std::vector<std::string> var_order_;
    std::vector<std::string> temp_var_order_;
};

} // namespace SOMTParser

#endif
