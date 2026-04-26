/* -*- Header -*-
 *
 * MiniZinc Frontend — Abstract Syntax Tree
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef MZN_AST_H
#define MZN_AST_H

#include "somtparser/minizinc/mzn_common.h"

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

// Forward declarations
struct Expr;
struct TypeInst;
struct Item;
struct Generator;

using ExprPtr  = std::shared_ptr<Expr>;
using TypePtr  = std::shared_ptr<TypeInst>;
using ItemPtr  = std::shared_ptr<Item>;

// ── 字面量载荷 ─────────────────────────────────────────────────────
struct BoolLit    { bool value = false; };
struct IntLit     { int64_t value = 0; };
struct FloatLit   { double value = 0.0; };
struct StringLit  { std::string value; };
struct ArrayLit   { std::vector<ExprPtr> elements; };
struct SetLit     { std::vector<ExprPtr> elements; };
struct TupleLit   { std::vector<ExprPtr> elements; };
struct RecordLit  {
    std::vector<std::pair<std::string, ExprPtr>> fields;
};

// ── 标识符 ─────────────────────────────────────────────────────────
struct Ident      { std::string name; };
struct AnonVar    {};

// ── 操作符 ─────────────────────────────────────────────────────────
struct UnaryOp {
    enum class Op { NOT, PLUS, MINUS };
    Op op;
    ExprPtr operand;
};

struct BinaryOp {
    enum class Op {
        // Boolean
        AND, OR, IMPLIES, IMPLIED_BY, IFF, XOR,
        // Comparison
        EQ, NEQ, LT, LE, GT, GE,
        // Set
        IN, SUBSET, SUPERSET, UNION, DIFF, SYMDIFF, INTERSECT,
        // Range
        RANGE, RANGE_HALF_OPEN_L, RANGE_HALF_OPEN_R, RANGE_OPEN,
        // Array
        CONCAT,
        // Arithmetic
        ADD, SUB, MUL, DIV, DIV_INT, MOD, POW
    };
    Op op;
    ExprPtr left;
    ExprPtr right;
};

// ── 调用与访问 ─────────────────────────────────────────────────────
struct Generator {
    std::vector<std::string> vars;
    ExprPtr set_expr;
};

struct CallExpr {
    std::string name;
    std::vector<ExprPtr> args;
    std::vector<Generator> generators;  // for comprehension-style calls: sum(i in S)(expr)
    bool is_comprehension_call = false;
};

struct ArrayAccess {
    ExprPtr array;
    std::vector<ExprPtr> indices;
};

struct ArraySlice {
    ExprPtr array;
    ExprPtr low;
    ExprPtr high;
    bool low_open = false;   // true for <..
    bool high_open = false;  // true for ..<
};

struct FieldAccess {
    ExprPtr record;
    std::string field;
};

struct TupleAccess {
    ExprPtr tuple;
    size_t index = 0;
};

// ── 推导式 ─────────────────────────────────────────────────────────
struct ArrayComp {
    ExprPtr body;
    std::vector<Generator> generators;
    ExprPtr where;  // nullptr if no where clause
};

struct SetComp {
    ExprPtr body;
    std::vector<Generator> generators;
    ExprPtr where;
};

// ── 控制流 ─────────────────────────────────────────────────────────
struct IfThenElse {
    std::vector<std::pair<ExprPtr, ExprPtr>> branches; // (cond, then_expr)
    ExprPtr else_branch;
};

struct LetExpr {
    std::vector<ItemPtr> items;
    ExprPtr body;
};

// ── 注解 ───────────────────────────────────────────────────────────
struct Annotated {
    ExprPtr expr;
    std::vector<ExprPtr> annotations;
};

// ── 表达式变体 ─────────────────────────────────────────────────────
struct Expr {
    enum class Kind {
        BOOL_LIT, INT_LIT, FLOAT_LIT, STRING_LIT,
        ARRAY_LIT, SET_LIT, TUPLE_LIT, RECORD_LIT,
        IDENT, ANON_VAR,
        UNARY_OP, BINARY_OP,
        IF_THEN_ELSE, LET,
        CALL, ARRAY_ACCESS, ARRAY_SLICE, FIELD_ACCESS, TUPLE_ACCESS,
        ARRAY_COMP, SET_COMP,
        ANNOTATED
    };

    Kind kind;
    SourceLoc loc;
    std::variant<
        BoolLit, IntLit, FloatLit, StringLit,
        ArrayLit, SetLit, TupleLit, RecordLit,
        Ident, AnonVar,
        UnaryOp, BinaryOp,
        IfThenElse, LetExpr,
        CallExpr, ArrayAccess, ArraySlice, FieldAccess, TupleAccess,
        ArrayComp, SetComp,
        Annotated
    > data;

    Expr() = default;
    Expr(Kind k, const SourceLoc& loc) : kind(k), loc(loc) {}

    template<typename T>
    T* as() { return std::get_if<T>(&data); }
    template<typename T>
    const T* as() const { return std::get_if<T>(&data); }

    std::string toString() const;
};

// ── 类型实例 ───────────────────────────────────────────────────────
struct TypeInst {
    enum class ParVar { OMIT, PAR, VAR };
    ParVar par_var = ParVar::OMIT;
    bool is_opt = false;
    bool is_set = false;

    enum class BaseKind {
        BOOL, INT, FLOAT, STRING, ANN, ANY, TOP,
        ENUM, TYPE_VAR, ALIAS, TUPLE, RECORD, UNKNOWN
    };
    BaseKind base = BaseKind::UNKNOWN;
    std::string name;                     // for enum, alias, type var
    std::vector<ExprPtr> array_dims;      // for array[dim1,..,dimN] of elem
    std::shared_ptr<TypeInst> elem_type;  // element type for array/set
    std::vector<std::shared_ptr<TypeInst>> tuple_elems;
    std::vector<std::pair<std::string, std::shared_ptr<TypeInst>>> record_fields;
    ExprPtr domain_expr;                  // e.g., 1..10, {1,3,5}
    SourceLoc loc;

    std::string toString() const;
    bool isVar() const { return par_var == ParVar::VAR; }
    bool isPar() const { return par_var == ParVar::PAR || par_var == ParVar::OMIT; }
    bool isArray() const { return base == BaseKind::UNKNOWN && !array_dims.empty(); }
};

// ── 顶层 Item ──────────────────────────────────────────────────────
struct Item {
    enum class Kind {
        INCLUDE, VAR_DECL, ASSIGN,
        CONSTRAINT, SOLVE, OUTPUT,
        PREDICATE, FUNCTION, TEST, ANNOTATION,
        ENUM_DECL
    };
    Kind kind;
    SourceLoc loc;

    Item(Kind k, const SourceLoc& loc) : kind(k), loc(loc) {}
    virtual ~Item() = default;
};

struct IncludeItem : public Item {
    std::string filename;
    IncludeItem(const SourceLoc& loc, const std::string& filename)
        : Item(Kind::INCLUDE, loc), filename(filename) {}
};

struct VarDeclItem : public Item {
    std::shared_ptr<TypeInst> type;
    std::string name;
    ExprPtr init;  // nullptr if no initializer
    std::vector<ExprPtr> anns;
    VarDeclItem(const SourceLoc& loc, std::shared_ptr<TypeInst> type,
                const std::string& name, ExprPtr init = nullptr)
        : Item(Kind::VAR_DECL, loc), type(std::move(type)), name(name), init(std::move(init)) {}
};

struct AssignItem : public Item {
    std::string name;
    ExprPtr expr;
    AssignItem(const SourceLoc& loc, const std::string& name, ExprPtr expr)
        : Item(Kind::ASSIGN, loc), name(name), expr(std::move(expr)) {}
};

struct ConstraintItem : public Item {
    ExprPtr expr;
    std::vector<ExprPtr> anns;
    ConstraintItem(const SourceLoc& loc, ExprPtr expr)
        : Item(Kind::CONSTRAINT, loc), expr(std::move(expr)) {}
};

struct SolveItem : public Item {
    enum class Mode { SATISFY, MINIMIZE, MAXIMIZE };
    Mode mode;
    ExprPtr objective;  // nullptr for SATISFY
    std::vector<ExprPtr> anns;
    SolveItem(const SourceLoc& loc, Mode mode, ExprPtr objective = nullptr)
        : Item(Kind::SOLVE, loc), mode(mode), objective(std::move(objective)) {}
};

struct OutputItem : public Item {
    ExprPtr expr;
    OutputItem(const SourceLoc& loc, ExprPtr expr)
        : Item(Kind::OUTPUT, loc), expr(std::move(expr)) {}
};

struct PredicateItem : public Item {
    std::string name;
    std::vector<std::shared_ptr<VarDeclItem>> params;
    ExprPtr body;  // nullptr if no body (forward decl)
    PredicateItem(const SourceLoc& loc, const std::string& name)
        : Item(Kind::PREDICATE, loc), name(name) {}
};

struct FunctionItem : public Item {
    std::shared_ptr<TypeInst> ret_type;
    std::string name;
    std::vector<std::shared_ptr<VarDeclItem>> params;
    ExprPtr body;
    FunctionItem(const SourceLoc& loc, std::shared_ptr<TypeInst> ret_type,
                 const std::string& name)
        : Item(Kind::FUNCTION, loc), ret_type(std::move(ret_type)), name(name) {}
};

struct TestItem : public Item {
    std::string name;
    std::vector<std::shared_ptr<VarDeclItem>> params;
    ExprPtr body;
    TestItem(const SourceLoc& loc, const std::string& name)
        : Item(Kind::TEST, loc), name(name) {}
};

struct AnnotationItem : public Item {
    std::string name;
    std::vector<std::shared_ptr<VarDeclItem>> params;
    AnnotationItem(const SourceLoc& loc, const std::string& name)
        : Item(Kind::ANNOTATION, loc), name(name) {}
};

struct EnumDeclItem : public Item {
    std::string name;
    std::vector<std::string> constructors;
    EnumDeclItem(const SourceLoc& loc, const std::string& name)
        : Item(Kind::ENUM_DECL, loc), name(name) {}
};

// ── 模型 ───────────────────────────────────────────────────────────
struct Model {
    std::vector<ItemPtr> items;
    std::string filename;
    std::unordered_map<std::string, ItemPtr> top_level_map;

    void addItem(ItemPtr item);
    ItemPtr lookup(const std::string& name) const;
    std::string toString() const;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_AST_H
