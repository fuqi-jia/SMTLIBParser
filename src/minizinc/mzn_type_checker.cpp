/* -*- C++ -*-
 *
 * MiniZinc Frontend — Type Checker & Inference Engine Implementation
 */

#include "somtparser/minizinc/mzn_type_checker.h"
#include "somtparser/minizinc/mzn_builtins.h"

namespace SOMTParser::MiniZinc {

// ── Constructor ──────────────────────────────────────────────────
MznTypeChecker::MznTypeChecker(MznSymbolTable* sym_table)
    : sym_table(sym_table) {}

// ── Main entry ───────────────────────────────────────────────────
void MznTypeChecker::checkModel(const Model& model) {
    // First pass: register all declarations
    for (auto& item : model.items) {
        if (auto* vd = dynamic_cast<VarDeclItem*>(item.get())) {
            sym_table->registerVar(
                std::static_pointer_cast<VarDeclItem>(item));
        } else if (auto* ed = dynamic_cast<EnumDeclItem*>(item.get())) {
            sym_table->registerEnum(
                std::static_pointer_cast<EnumDeclItem>(item));
        } else if (auto* pd = dynamic_cast<PredicateItem*>(item.get())) {
            sym_table->registerPredicate(
                std::static_pointer_cast<PredicateItem>(item));
        } else if (auto* fd = dynamic_cast<FunctionItem*>(item.get())) {
            sym_table->registerFunction(
                std::static_pointer_cast<FunctionItem>(item));
        }
    }

    // Second pass: type-check each item
    for (auto& item : model.items) {
        checkItem(item);
    }
}

// ── Per-item checking ────────────────────────────────────────────
void MznTypeChecker::checkItem(const ItemPtr& item) {
    switch (item->kind) {
        case Item::Kind::VAR_DECL:
            checkVarDecl(*static_cast<VarDeclItem*>(item.get()));
            break;
        case Item::Kind::CONSTRAINT:
            checkConstraint(*static_cast<ConstraintItem*>(item.get()));
            break;
        case Item::Kind::SOLVE:
            checkSolveItem(*static_cast<SolveItem*>(item.get()));
            break;
        case Item::Kind::PREDICATE:
            checkPredicate(*static_cast<PredicateItem*>(item.get()));
            break;
        case Item::Kind::FUNCTION:
            checkFunction(*static_cast<FunctionItem*>(item.get()));
            break;
        case Item::Kind::ENUM_DECL:
            checkEnumDecl(*static_cast<EnumDeclItem*>(item.get()));
            break;
        default:
            break;
    }
}

void MznTypeChecker::checkVarDecl(const VarDeclItem& decl) {
    if (decl.init) {
        auto init_type = inferType(decl.init);
        if (!init_type) {
            addError(decl.init->loc, "Cannot infer type of initializer");
            return;
        }
        if (!isAssignable(*decl.type, *init_type)) {
            addError(decl.init->loc,
                "Type mismatch: cannot assign " + init_type->toString() +
                " to " + decl.type->toString());
        }
    }
}

void MznTypeChecker::checkConstraint(const ConstraintItem& ci) {
    auto t = inferType(ci.expr);
    if (!t) {
        addError(ci.expr->loc, "Cannot infer type of constraint expression");
        return;
    }
    if (t->base != TypeInst::BaseKind::BOOL) {
        addError(ci.expr->loc, "Constraint must be a bool expression");
    }
}

void MznTypeChecker::checkSolveItem(const SolveItem& si) {
    if (si.mode != SolveItem::Mode::SATISFY && si.objective) {
        auto t = inferType(si.objective);
        if (!t) {
            addError(si.objective->loc, "Cannot infer type of objective");
            return;
        }
        if (t->base != TypeInst::BaseKind::INT &&
            t->base != TypeInst::BaseKind::FLOAT) {
            addError(si.objective->loc, "Objective must be numeric");
        }
    }
}

void MznTypeChecker::checkPredicate(const PredicateItem& pi) {
    sym_table->pushScope();
    for (auto& p : pi.params) {
        sym_table->registerVar(p);
    }
    if (pi.body) {
        auto t = inferType(pi.body);
        if (!t || t->base != TypeInst::BaseKind::BOOL) {
            addError(pi.body->loc, "Predicate body must be bool");
        }
    }
    sym_table->popScope();
}

void MznTypeChecker::checkFunction(const FunctionItem& fi) {
    sym_table->pushScope();
    for (auto& p : fi.params) {
        sym_table->registerVar(p);
    }
    if (fi.body) {
        auto t = inferType(fi.body);
        if (!t) {
            addError(fi.body->loc, "Cannot infer function body type");
        }
        // TODO: check return type matches
    }
    sym_table->popScope();
}

void MznTypeChecker::checkEnumDecl(const EnumDeclItem& edi) {
    (void)edi; // no type checking needed for enum declarations
}

// ── Expression type inference ────────────────────────────────────
std::shared_ptr<TypeInst> MznTypeChecker::inferType(const ExprPtr& expr) {
    if (!expr) return nullptr;

    switch (expr->kind) {
        case Expr::Kind::BOOL_LIT: {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::BOOL;
            t->par_var = TypeInst::ParVar::PAR;
            return t;
        }
        case Expr::Kind::INT_LIT: {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::INT;
            t->par_var = TypeInst::ParVar::PAR;
            return t;
        }
        case Expr::Kind::FLOAT_LIT: {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::FLOAT;
            t->par_var = TypeInst::ParVar::PAR;
            return t;
        }
        case Expr::Kind::STRING_LIT: {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::STRING;
            t->par_var = TypeInst::ParVar::PAR;
            return t;
        }
        case Expr::Kind::IDENT: {
            auto* id = expr->as<Ident>();
            if (!id) return nullptr;
            auto vd = sym_table->lookupVar(id->name);
            if (vd) return vd->type;
            // Check if it's a par enum constructor
            return nullptr;
        }
        case Expr::Kind::ANON_VAR: {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::ANY;
            return t;
        }
        case Expr::Kind::UNARY_OP:
            return inferUnaryOp(*expr->as<UnaryOp>());
        case Expr::Kind::BINARY_OP:
            return inferBinaryOp(*expr->as<BinaryOp>());
        case Expr::Kind::CALL:
            return inferCall(*expr->as<CallExpr>());
        case Expr::Kind::IF_THEN_ELSE:
            return inferIfThenElse(*expr->as<IfThenElse>());
        case Expr::Kind::ARRAY_ACCESS:
            return inferArrayAccess(*expr->as<ArrayAccess>());
        case Expr::Kind::ARRAY_LIT:
            return inferArrayLit(*expr->as<ArrayLit>());
        case Expr::Kind::SET_LIT:
            return inferSetLit(*expr->as<SetLit>());
        case Expr::Kind::LET:
            return inferLet(*expr->as<LetExpr>());
        case Expr::Kind::TUPLE_LIT:
        case Expr::Kind::RECORD_LIT:
            // TODO
            return nullptr;
        default:
            return nullptr;
    }
}

std::shared_ptr<TypeInst> MznTypeChecker::inferBinaryOp(const BinaryOp& op) {
    auto lt = inferType(op.left);
    auto rt = inferType(op.right);
    if (!lt || !rt) return nullptr;

    auto result = std::make_shared<TypeInst>();

    switch (op.op) {
        // Boolean ops -> bool
        case BinaryOp::Op::AND:
        case BinaryOp::Op::OR:
        case BinaryOp::Op::IMPLIES:
        case BinaryOp::Op::IMPLIED_BY:
        case BinaryOp::Op::IFF:
        case BinaryOp::Op::XOR:
            result->base = TypeInst::BaseKind::BOOL;
            return result;

        // Comparisons -> bool
        case BinaryOp::Op::EQ:
        case BinaryOp::Op::NEQ:
        case BinaryOp::Op::LT:
        case BinaryOp::Op::LE:
        case BinaryOp::Op::GT:
        case BinaryOp::Op::GE:
            result->base = TypeInst::BaseKind::BOOL;
            return result;

        // Set ops -> set
        case BinaryOp::Op::UNION:
        case BinaryOp::Op::DIFF:
        case BinaryOp::Op::SYMDIFF:
        case BinaryOp::Op::INTERSECT:
            result->base = TypeInst::BaseKind::INT; // set of int
            result->is_set = true;
            return result;

        // Range -> set of int
        case BinaryOp::Op::RANGE:
        case BinaryOp::Op::RANGE_HALF_OPEN_L:
        case BinaryOp::Op::RANGE_HALF_OPEN_R:
        case BinaryOp::Op::RANGE_OPEN:
            result->base = TypeInst::BaseKind::INT;
            result->is_set = true;
            return result;

        // Arithmetic -> numeric
        case BinaryOp::Op::ADD:
        case BinaryOp::Op::SUB:
        case BinaryOp::Op::MUL:
        case BinaryOp::Op::DIV:
        case BinaryOp::Op::DIV_INT:
        case BinaryOp::Op::MOD:
        case BinaryOp::Op::POW:
            if (lt->base == TypeInst::BaseKind::FLOAT ||
                rt->base == TypeInst::BaseKind::FLOAT) {
                result->base = TypeInst::BaseKind::FLOAT;
            } else {
                result->base = TypeInst::BaseKind::INT;
            }
            return result;

        case BinaryOp::Op::CONCAT:
            // array or string concat
            if (lt->base == TypeInst::BaseKind::STRING) {
                result->base = TypeInst::BaseKind::STRING;
            } else {
                result->base = TypeInst::BaseKind::UNKNOWN;
                result->array_dims = lt->array_dims;
                result->elem_type = lt->elem_type;
            }
            return result;

        default:
            return nullptr;
    }
}

std::shared_ptr<TypeInst> MznTypeChecker::inferUnaryOp(const UnaryOp& op) {
    auto t = inferType(op.operand);
    if (!t) return nullptr;
    auto result = std::make_shared<TypeInst>();

    switch (op.op) {
        case UnaryOp::Op::NOT:
            result->base = TypeInst::BaseKind::BOOL;
            return result;
        case UnaryOp::Op::PLUS:
        case UnaryOp::Op::MINUS:
            result->base = t->base;
            return result;
        default:
            return nullptr;
    }
}

std::shared_ptr<TypeInst> MznTypeChecker::inferCall(const CallExpr& call) {
    // Built-in aggregation / function
    if (MznBuiltins::get().isAggregation(call.name)) {
        if (call.name == "forall" || call.name == "exists" ||
            call.name == "clause" || call.name == "iffall" || call.name == "xorall") {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::BOOL;
            return t;
        }
        if (call.name == "sum" || call.name == "product") {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::INT;
            return t;
        }
        if (call.name == "min" || call.name == "max") {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::INT;
            return t;
        }
        if (call.name == "count" || call.name == "card" || call.name == "length") {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::INT;
            return t;
        }
    }

    if (MznBuiltins::get().isConversion(call.name)) {
        auto t = std::make_shared<TypeInst>();
        if (call.name == "bool2int") t->base = TypeInst::BaseKind::INT;
        else if (call.name == "int2float") t->base = TypeInst::BaseKind::FLOAT;
        else if (call.name == "ceil" || call.name == "floor" || call.name == "round") t->base = TypeInst::BaseKind::INT;
        else t->base = TypeInst::BaseKind::INT;
        return t;
    }

    if (MznBuiltins::get().isTranscendental(call.name)) {
        auto t = std::make_shared<TypeInst>();
        t->base = TypeInst::BaseKind::FLOAT;
        return t;
    }

    // User-defined function
    if (sym_table) {
        auto func = sym_table->resolveFunction(call.name, {});
        if (func) return func->ret_type;
        auto pred = sym_table->resolvePredicate(call.name, {});
        if (pred) {
            auto t = std::make_shared<TypeInst>();
            t->base = TypeInst::BaseKind::BOOL;
            return t;
        }
    }

    return nullptr;
}

std::shared_ptr<TypeInst> MznTypeChecker::inferIfThenElse(const IfThenElse& ite) {
    // Return type is the common type of then/else branches
    auto t = inferType(ite.else_branch);
    for (auto& branch : ite.branches) {
        auto bt = inferType(branch.second);
        if (!bt) continue;
        t = bt; // simplified: take the last known type
    }
    return t;
}

std::shared_ptr<TypeInst> MznTypeChecker::inferArrayAccess(const ArrayAccess& acc) {
    auto arr_t = inferType(acc.array);
    if (!arr_t || !arr_t->elem_type) return nullptr;
    return arr_t->elem_type;
}

std::shared_ptr<TypeInst> MznTypeChecker::inferArrayLit(const ArrayLit& arr) {
    if (arr.elements.empty()) {
        auto t = std::make_shared<TypeInst>();
        t->base = TypeInst::BaseKind::UNKNOWN;
        return t;
    }
    auto elem_t = inferType(arr.elements[0]);
    auto result = std::make_shared<TypeInst>();
    result->base = TypeInst::BaseKind::UNKNOWN;
    result->elem_type = elem_t;
    // 1D array with index set 1..n
    auto dim = std::make_shared<Expr>(Expr::Kind::INT_LIT, SourceLoc{});
    dim->data = IntLit{static_cast<int64_t>(arr.elements.size())};
    result->array_dims.push_back(dim);
    return result;
}

std::shared_ptr<TypeInst> MznTypeChecker::inferSetLit(const SetLit& set) {
    (void)set;
    auto t = std::make_shared<TypeInst>();
    t->base = TypeInst::BaseKind::INT;
    t->is_set = true;
    return t;
}

std::shared_ptr<TypeInst> MznTypeChecker::inferLet(const LetExpr& let) {
    sym_table->pushScope();
    for (auto& item : let.items) {
        if (auto* vd = dynamic_cast<VarDeclItem*>(item.get())) {
            sym_table->registerVar(
                std::static_pointer_cast<VarDeclItem>(item));
        }
    }
    auto t = inferType(let.body);
    sym_table->popScope();
    return t;
}

// ── Type compatibility ───────────────────────────────────────────
bool MznTypeChecker::isAssignable(const TypeInst& target,
                                  const TypeInst& source) const {
    if (target.base != source.base && target.base != TypeInst::BaseKind::ANY
        && source.base != TypeInst::BaseKind::ANY) {
        // Allow int -> float coercion
        if (!(target.base == TypeInst::BaseKind::FLOAT &&
              source.base == TypeInst::BaseKind::INT)) {
            return false;
        }
    }
    if (target.is_set != source.is_set) return false;
    if (target.is_opt != source.is_opt) return false;
    // TODO: deeper array/enum compatibility
    return true;
}

bool MznTypeChecker::isComparable(const TypeInst& a, const TypeInst& b) const {
    if (a.base == b.base) return true;
    if ((a.base == TypeInst::BaseKind::INT && b.base == TypeInst::BaseKind::FLOAT) ||
        (a.base == TypeInst::BaseKind::FLOAT && b.base == TypeInst::BaseKind::INT)) {
        return true;
    }
    return false;
}

bool MznTypeChecker::isCoercible(const TypeInst& from, const TypeInst& to) const {
    return isAssignable(to, from);
}

// ── Error helpers ────────────────────────────────────────────────
void MznTypeChecker::addError(const SourceLoc& loc, const std::string& msg) {
    errors.push_back("[" + std::to_string(loc.line) + ":" + std::to_string(loc.col) + "] " + msg);
}

} // namespace SOMTParser::MiniZinc
