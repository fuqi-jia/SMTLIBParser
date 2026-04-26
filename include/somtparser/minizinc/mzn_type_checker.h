/* -*- Header -*-
 *
 * MiniZinc Frontend — Type Checker & Inference Engine
 *
 * Performs bottom-up type inference and top-down type checking
 * on a fully parsed MiniZinc AST.
 */

#ifndef MZN_TYPE_CHECKER_H
#define MZN_TYPE_CHECKER_H

#include "somtparser/minizinc/mzn_ast.h"
#include "somtparser/minizinc/mzn_symbol_table.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

/**
 * @brief Type checker for MiniZinc AST.
 *
 * After parsing, run checkModel() to infer types and validate constraints.
 */
class MznTypeChecker {
public:
    explicit MznTypeChecker(MznSymbolTable* sym_table);

    // ── Main entry ─────────────────────────────────────────────
    void checkModel(const Model& model);

    // ── Per-item checking ──────────────────────────────────────
    void checkItem(const ItemPtr& item);
    void checkVarDecl(const VarDeclItem& decl);
    void checkConstraint(const ConstraintItem& ci);
    void checkSolveItem(const SolveItem& si);
    void checkPredicate(const PredicateItem& pi);
    void checkFunction(const FunctionItem& fi);
    void checkEnumDecl(const EnumDeclItem& edi);

    // ── Expression type inference ──────────────────────────────
    std::shared_ptr<TypeInst> inferType(const ExprPtr& expr);

    // ── Type compatibility ─────────────────────────────────────
    bool isAssignable(const TypeInst& target, const TypeInst& source) const;
    bool isComparable(const TypeInst& a, const TypeInst& b) const;
    bool isCoercible(const TypeInst& from, const TypeInst& to) const;

    // ── Errors ─────────────────────────────────────────────────
    bool hasErrors() const { return !errors.empty(); }
    const std::vector<std::string>& getErrors() const { return errors; }

private:
    MznSymbolTable* sym_table;
    std::vector<std::string> errors;

    std::shared_ptr<TypeInst> inferBinaryOp(const BinaryOp& op);
    std::shared_ptr<TypeInst> inferUnaryOp(const UnaryOp& op);
    std::shared_ptr<TypeInst> inferCall(const CallExpr& call);
    std::shared_ptr<TypeInst> inferIfThenElse(const IfThenElse& ite);
    std::shared_ptr<TypeInst> inferArrayAccess(const ArrayAccess& acc);
    std::shared_ptr<TypeInst> inferArrayLit(const ArrayLit& arr);
    std::shared_ptr<TypeInst> inferSetLit(const SetLit& set);
    std::shared_ptr<TypeInst> inferLet(const LetExpr& let);

    void addError(const SourceLoc& loc, const std::string& msg);
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_TYPE_CHECKER_H
