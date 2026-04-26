/* -*- Header -*-
 *
 * Unified IR — Language-agnostic intermediate representation.
 *
 * The Unified IR is a superset of SMT-LIB, MiniZinc, and other
 * constraint-modeling languages. It uses UnifiedOpRef (runtime-resolved
 * via UnifiedOpRegistry) instead of hardcoded op enums.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef UNIFIED_IR_H
#define UNIFIED_IR_H

#include "somtparser/unified/unified_op_registry.h"

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <optional>

namespace SOMTParser::Unified {

// ── Source location (lightweight) ──────────────────────────────────

struct SourceLoc {
    std::string filename;
    int line = 0;
    int column = 0;

    SourceLoc() = default;
    SourceLoc(std::string fn, int l, int c)
        : filename(std::move(fn)), line(l), column(c) {}
};

// ── Forward declarations ───────────────────────────────────────────

struct UnifiedExpr;
struct UnifiedType;
struct UnifiedVarDecl;

using ExprPtr = std::shared_ptr<UnifiedExpr>;
using TypePtr = std::shared_ptr<UnifiedType>;

// ── Unified type system ────────────────────────────────────────────

struct UnifiedType {
    enum class ParVar { PAR, VAR };
    ParVar par_var = ParVar::VAR;
    bool is_optional = false;

    UnifiedSort sort;                     // base sort (or ANY if inferred)
    std::vector<ExprPtr> array_dims;      // for array types: dimension expressions
    std::vector<std::pair<std::string, UnifiedType>> record_fields;
    ExprPtr domain;                       // e.g., range 1..10, set literal {1,3,5}

    UnifiedType() = default;
    explicit UnifiedType(UnifiedSort s) : sort(std::move(s)) {}

    bool isVar() const { return par_var == ParVar::VAR; }
    bool isPar() const { return par_var == ParVar::PAR; }
    bool isArray() const { return sort.kind == UnifiedSort::Kind::ARRAY; }
};

// ── Expression node ────────────────────────────────────────────────

struct UnifiedExpr {
    enum class Kind {
        LITERAL,    // bool, int, float, string
        IDENT,      // named variable / parameter
        OP,         // operator call via registry
        ARRAY_LIT,  // [e1, e2, ...]
        SET_LIT,    // {e1, e2, ...}
        TUPLE_LIT,  // (e1, e2, ...)
        RECORD_LIT, // (f1: e1, f2: e2, ...)
        LET,        // let {decls} in body
        ITE,        // if cond then then_expr else else_expr
        FORALL,     // universal quantifier
        EXISTS,     // existential quantifier
    };

    Kind kind;
    SourceLoc loc;

    // ── Sub-structures ─────────────────────────────────────────────

    struct Literal {
        enum class LitKind { BOOL, INT, FLOAT, STRING };
        LitKind lit_kind;
        std::variant<bool, int64_t, double, std::string> value;

        static Literal mkBool(bool v)     { return {LitKind::BOOL, v}; }
        static Literal mkInt(int64_t v)   { return {LitKind::INT, v}; }
        static Literal mkFloat(double v)  { return {LitKind::FLOAT, v}; }
        static Literal mkString(std::string v) { return {LitKind::STRING, std::move(v)}; }
    };

    struct Ident { std::string name; };

    struct OpNode {
        UnifiedOpRef op;
        std::vector<ExprPtr> args;
        // For comprehension-style ops (e.g., sum(i in S)(expr))
        std::vector<std::pair<std::string, ExprPtr>> generators;
    };

    struct ArrayLit { std::vector<ExprPtr> elems; };
    struct SetLit   { std::vector<ExprPtr> elems; };
    struct TupleLit { std::vector<ExprPtr> elems; };
    struct RecordLit {
        std::vector<std::pair<std::string, ExprPtr>> fields;
    };

    struct LetExpr {
        std::vector<UnifiedVarDecl> locals;
        ExprPtr body;
    };

    struct IteExpr {
        ExprPtr cond;
        ExprPtr then_expr;
        ExprPtr else_expr;
    };

    struct QuantExpr {
        std::vector<std::pair<std::string, ExprPtr>> generators; // var -> set_expr
        ExprPtr body;
    };

    // ── Variant storage ────────────────────────────────────────────

    std::variant<Literal, Ident, OpNode, ArrayLit, SetLit, TupleLit,
                 RecordLit, LetExpr, IteExpr, QuantExpr> data;

    // Constructors
    UnifiedExpr() = default;
    UnifiedExpr(Kind k, const SourceLoc& loc) : kind(k), loc(loc) {}

    template<typename T>
    T* as() { return std::get_if<T>(&data); }
    template<typename T>
    const T* as() const { return std::get_if<T>(&data); }

    // Convenience accessors
    Literal* asLiteral()       { return as<Literal>(); }
    const Literal* asLiteral() const { return as<Literal>(); }
    Ident* asIdent()           { return as<Ident>(); }
    const Ident* asIdent() const     { return as<Ident>(); }
    OpNode* asOp()             { return as<OpNode>(); }
    const OpNode* asOp() const       { return as<OpNode>(); }
    ArrayLit* asArray()        { return as<ArrayLit>(); }
    const ArrayLit* asArray() const  { return as<ArrayLit>(); }
    SetLit* asSet()            { return as<SetLit>(); }
    const SetLit* asSet() const      { return as<SetLit>(); }
    LetExpr* asLet()           { return as<LetExpr>(); }
    const LetExpr* asLet() const     { return as<LetExpr>(); }
    IteExpr* asIte()           { return as<IteExpr>(); }
    const IteExpr* asIte() const     { return as<IteExpr>(); }
    QuantExpr* asQuant()       { return as<QuantExpr>(); }
    const QuantExpr* asQuant() const { return as<QuantExpr>(); }
};

// ── Variable declaration ───────────────────────────────────────────

struct UnifiedVarDecl {
    std::string name;
    UnifiedType type;
    ExprPtr init;                    // nullptr if no initializer
    std::vector<ExprPtr> anns;       // annotations

    UnifiedVarDecl() = default;
    UnifiedVarDecl(std::string name, UnifiedType type, ExprPtr init = nullptr)
        : name(std::move(name)), type(std::move(type)), init(std::move(init)) {}
};

// ── Constraint ─────────────────────────────────────────────────────

struct UnifiedConstraint {
    ExprPtr expr;                    // boolean expression
    std::vector<ExprPtr> anns;       // annotations (e.g., :: domain, :: bounds)

    UnifiedConstraint() = default;
    explicit UnifiedConstraint(ExprPtr expr) : expr(std::move(expr)) {}
};

// ── Objective ──────────────────────────────────────────────────────

struct UnifiedObjective {
    enum class Mode { SATISFY, MINIMIZE, MAXIMIZE };
    Mode mode = Mode::SATISFY;
    ExprPtr expr;                    // nullptr for SATISFY
    std::vector<ExprPtr> anns;

    UnifiedObjective() = default;
    explicit UnifiedObjective(Mode m, ExprPtr e = nullptr)
        : mode(m), expr(std::move(e)) {}
};

// ── Unified Model ──────────────────────────────────────────────────

struct UnifiedModel {
    std::vector<UnifiedVarDecl> vars;
    std::vector<UnifiedConstraint> constraints;
    std::vector<UnifiedObjective> objectives;
    std::vector<ExprPtr> outputs;    // output expressions (MiniZinc-style)
    std::vector<UnifiedVarDecl> parameters; // par declarations (can be promoted to vars)

    void addVar(UnifiedVarDecl var) { vars.push_back(std::move(var)); }
    void addConstraint(UnifiedConstraint c) { constraints.push_back(std::move(c)); }
    void addObjective(UnifiedObjective o) { objectives.push_back(std::move(o)); }
};

} // namespace SOMTParser::Unified

#endif // UNIFIED_IR_H
