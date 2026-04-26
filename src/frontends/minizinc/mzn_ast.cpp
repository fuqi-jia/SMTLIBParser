/* -*- C++ -*-
 *
 * MiniZinc Frontend — AST Utilities
 */

#include "somtparser/frontends/minizinc/mzn_ast.h"
#include <sstream>

namespace SOMTParser::MiniZinc {

// ── Expr::toString ─────────────────────────────────────────────────
std::string Expr::toString() const {
    std::ostringstream oss;
    switch (kind) {
        case Kind::BOOL_LIT:
            oss << (as<BoolLit>()->value ? "true" : "false");
            break;
        case Kind::INT_LIT:
            oss << as<IntLit>()->value;
            break;
        case Kind::FLOAT_LIT:
            oss << as<FloatLit>()->value;
            break;
        case Kind::STRING_LIT:
            oss << "\"" << as<StringLit>()->value << "\"";
            break;
        case Kind::ARRAY_LIT: {
            oss << "[";
            const auto& elems = as<ArrayLit>()->elements;
            for (size_t i = 0; i < elems.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << elems[i]->toString();
            }
            oss << "]";
            break;
        }
        case Kind::SET_LIT: {
            oss << "{";
            const auto& elems = as<SetLit>()->elements;
            for (size_t i = 0; i < elems.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << elems[i]->toString();
            }
            oss << "}";
            break;
        }
        case Kind::TUPLE_LIT: {
            oss << "(";
            const auto& elems = as<TupleLit>()->elements;
            for (size_t i = 0; i < elems.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << elems[i]->toString();
            }
            oss << ")";
            break;
        }
        case Kind::RECORD_LIT: {
            oss << "(";
            const auto& fields = as<RecordLit>()->fields;
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << fields[i].first << ": " << fields[i].second->toString();
            }
            oss << ")";
            break;
        }
        case Kind::IDENT:
            oss << as<Ident>()->name;
            break;
        case Kind::ANON_VAR:
            oss << "_";
            break;
        case Kind::UNARY_OP: {
            const auto* u = as<UnaryOp>();
            switch (u->op) {
                case UnaryOp::Op::NOT: oss << "not "; break;
                case UnaryOp::Op::PLUS: oss << "+"; break;
                case UnaryOp::Op::MINUS: oss << "-"; break;
            }
            oss << u->operand->toString();
            break;
        }
        case Kind::BINARY_OP: {
            const auto* b = as<BinaryOp>();
            oss << "(" << b->left->toString() << " ";
            switch (b->op) {
                case BinaryOp::Op::AND: oss << "/\\"; break;
                case BinaryOp::Op::OR: oss << "\\/"; break;
                case BinaryOp::Op::IMPLIES: oss << "->"; break;
                case BinaryOp::Op::IMPLIED_BY: oss << "<-"; break;
                case BinaryOp::Op::IFF: oss << "<->"; break;
                case BinaryOp::Op::XOR: oss << "xor"; break;
                case BinaryOp::Op::EQ: oss << "="; break;
                case BinaryOp::Op::NEQ: oss << "!="; break;
                case BinaryOp::Op::LT: oss << "<"; break;
                case BinaryOp::Op::LE: oss << "<="; break;
                case BinaryOp::Op::GT: oss << ">"; break;
                case BinaryOp::Op::GE: oss << ">="; break;
                case BinaryOp::Op::IN: oss << "in"; break;
                case BinaryOp::Op::SUBSET: oss << "subset"; break;
                case BinaryOp::Op::SUPERSET: oss << "superset"; break;
                case BinaryOp::Op::UNION: oss << "union"; break;
                case BinaryOp::Op::DIFF: oss << "diff"; break;
                case BinaryOp::Op::SYMDIFF: oss << "symdiff"; break;
                case BinaryOp::Op::INTERSECT: oss << "intersect"; break;
                case BinaryOp::Op::RANGE: oss << ".."; break;
                case BinaryOp::Op::RANGE_HALF_OPEN_L: oss << "..<"; break;
                case BinaryOp::Op::RANGE_HALF_OPEN_R: oss << "<.."; break;
                case BinaryOp::Op::RANGE_OPEN: oss << "<..<"; break;
                case BinaryOp::Op::CONCAT: oss << "++"; break;
                case BinaryOp::Op::ADD: oss << "+"; break;
                case BinaryOp::Op::SUB: oss << "-"; break;
                case BinaryOp::Op::MUL: oss << "*"; break;
                case BinaryOp::Op::DIV: oss << "/"; break;
                case BinaryOp::Op::DIV_INT: oss << "div"; break;
                case BinaryOp::Op::MOD: oss << "mod"; break;
                case BinaryOp::Op::POW: oss << "^"; break;
            }
            oss << " " << b->right->toString() << ")";
            break;
        }
        case Kind::CALL: {
            const auto* c = as<CallExpr>();
            oss << c->name << "(";
            for (size_t i = 0; i < c->args.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << c->args[i]->toString();
            }
            oss << ")";
            break;
        }
        case Kind::ARRAY_ACCESS: {
            const auto* a = as<ArrayAccess>();
            oss << a->array->toString() << "[";
            for (size_t i = 0; i < a->indices.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << a->indices[i]->toString();
            }
            oss << "]";
            break;
        }
        case Kind::IF_THEN_ELSE: {
            const auto* ite = as<IfThenElse>();
            oss << "if " << ite->branches[0].first->toString()
                << " then " << ite->branches[0].second->toString();
            for (size_t i = 1; i < ite->branches.size(); ++i) {
                oss << " elseif " << ite->branches[i].first->toString()
                    << " then " << ite->branches[i].second->toString();
            }
            oss << " else " << ite->else_branch->toString() << " endif";
            break;
        }
        case Kind::LET: {
            const auto* l = as<LetExpr>();
            oss << "let { ... } in " << l->body->toString();
            break;
        }
        case Kind::ANNOTATED: {
            const auto* a = as<Annotated>();
            oss << a->expr->toString() << " :: ...";
            break;
        }
        default:
            oss << "<expr>";
            break;
    }
    return oss.str();
}

// ── TypeInst::toString ─────────────────────────────────────────────
std::string TypeInst::toString() const {
    std::ostringstream oss;
    if (par_var == ParVar::VAR) oss << "var ";
    else if (par_var == ParVar::PAR) oss << "par ";
    if (is_opt) oss << "opt ";
    if (is_set) oss << "set of ";

    switch (base) {
        case BaseKind::BOOL: oss << "bool"; break;
        case BaseKind::INT: oss << "int"; break;
        case BaseKind::FLOAT: oss << "float"; break;
        case BaseKind::STRING: oss << "string"; break;
        case BaseKind::ANN: oss << "ann"; break;
        case BaseKind::ANY: oss << "any"; break;
        case BaseKind::TOP: oss << "top"; break;
        case BaseKind::ENUM: oss << name; break;
        case BaseKind::TYPE_VAR: oss << "$" << name; break;
        case BaseKind::ALIAS: oss << name; break;
        default: {
            if (!array_dims.empty()) {
                oss << "array[";
                for (size_t i = 0; i < array_dims.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << array_dims[i]->toString();
                }
                oss << "] of " << (elem_type ? elem_type->toString() : "?");
            } else if (elem_type) {
                oss << elem_type->toString();
            } else {
                oss << "?";
            }
        }
    }

    if (domain_expr) {
        oss << "(" << domain_expr->toString() << ")";
    }
    return oss.str();
}

// ── Model helpers ──────────────────────────────────────────────────
void Model::addItem(ItemPtr item) {
    items.push_back(item);
}

ItemPtr Model::lookup(const std::string& name) const {
    auto it = top_level_map.find(name);
    if (it != top_level_map.end()) {
        return it->second;
    }
    return nullptr;
}

std::string Model::toString() const {
    std::ostringstream oss;
    oss << "Model(" << filename << "): " << items.size() << " items\n";
    for (const auto& item : items) {
        switch (item->kind) {
            case Item::Kind::INCLUDE: oss << "  include\n"; break;
            case Item::Kind::VAR_DECL: {
                auto* vd = static_cast<VarDeclItem*>(item.get());
                oss << "  var_decl: " << vd->name << " : " << vd->type->toString() << "\n";
                break;
            }
            case Item::Kind::ASSIGN: oss << "  assign\n"; break;
            case Item::Kind::CONSTRAINT: oss << "  constraint\n"; break;
            case Item::Kind::SOLVE: oss << "  solve\n"; break;
            case Item::Kind::OUTPUT: oss << "  output\n"; break;
            case Item::Kind::PREDICATE: oss << "  predicate\n"; break;
            case Item::Kind::FUNCTION: oss << "  function\n"; break;
            case Item::Kind::TEST: oss << "  test\n"; break;
            case Item::Kind::ANNOTATION: oss << "  annotation\n"; break;
            case Item::Kind::ENUM_DECL: oss << "  enum\n"; break;
        }
    }
    return oss.str();
}

} // namespace SOMTParser::MiniZinc
