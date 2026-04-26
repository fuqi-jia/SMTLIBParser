/* -*- Header -*-
 *
 * MiniZinc Frontend — Symbol Table
 *
 * Scoped symbol table supporting overloads, enums, functions, predicates.
 */

#ifndef MZN_SYMBOL_TABLE_H
#define MZN_SYMBOL_TABLE_H

#include "somtparser/minizinc/mzn_ast.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

// Forward declarations
struct TypeInst;
struct VarDeclItem;
struct FunctionItem;
struct PredicateItem;
struct EnumDeclItem;

/**
 * @brief A symbol table entry.
 */
struct SymbolEntry {
    enum class Kind {
        VAR,        // Variable / parameter declaration
        FUNCTION,   // function item
        PREDICATE,  // predicate item
        ENUM,       // enum declaration
        ANNOTATION, // annotation declaration
        TYPE_ALIAS  // type alias
    };
    Kind kind;
    std::string name;
    SourceLoc loc;
    std::shared_ptr<void> payload; // type-erased pointer to the actual item

    SymbolEntry(Kind k, const std::string& n, const SourceLoc& l)
        : kind(k), name(n), loc(l) {}
};

/**
 * @brief A scope frame in the symbol table.
 */
struct ScopeFrame {
    std::unordered_map<std::string, std::vector<std::shared_ptr<SymbolEntry>>> entries;
    std::vector<std::string> ordered_names; // insertion order
};

/**
 * @brief MiniZinc symbol table with lexical scoping.
 *
 * Supports function/predicate overloading by arity and parameter types.
 */
class MznSymbolTable {
public:
    MznSymbolTable();

    // ── Scope management ───────────────────────────────────────
    void pushScope();
    void popScope();
    size_t scopeDepth() const;

    // ── Registration ───────────────────────────────────────────
    void registerVar(const std::shared_ptr<VarDeclItem>& decl);
    void registerFunction(const std::shared_ptr<FunctionItem>& func);
    void registerPredicate(const std::shared_ptr<PredicateItem>& pred);
    void registerEnum(const std::shared_ptr<EnumDeclItem>& enm);
    void registerAnnotation(const std::shared_ptr<AnnotationItem>& ann);
    void registerTypeAlias(const std::string& name, std::shared_ptr<TypeInst> type);

    // ── Lookup ─────────────────────────────────────────────────
    std::shared_ptr<SymbolEntry> lookup(const std::string& name) const;
    std::vector<std::shared_ptr<SymbolEntry>> lookupAll(const std::string& name) const;
    std::shared_ptr<VarDeclItem> lookupVar(const std::string& name) const;
    std::shared_ptr<EnumDeclItem> lookupEnum(const std::string& name) const;
    std::shared_ptr<TypeInst> lookupTypeAlias(const std::string& name) const;

    // ── Overload resolution ────────────────────────────────────
    std::shared_ptr<FunctionItem> resolveFunction(
        const std::string& name,
        const std::vector<std::shared_ptr<TypeInst>>& arg_types) const;
    std::shared_ptr<PredicateItem> resolvePredicate(
        const std::string& name,
        const std::vector<std::shared_ptr<TypeInst>>& arg_types) const;

    // ── Queries ────────────────────────────────────────────────
    bool isDefined(const std::string& name) const;
    bool isVar(const std::string& name) const;
    bool isPar(const std::string& name) const;
    bool isEnum(const std::string& name) const;

    void clear();

private:
    std::vector<ScopeFrame> scopes;

    std::shared_ptr<SymbolEntry> lookupInScopes(const std::string& name) const;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_SYMBOL_TABLE_H
