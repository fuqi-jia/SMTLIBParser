/* -*- C++ -*-
 *
 * UnifiedPrinter implementation
 */

#include "somtparser/unified/unified_printer.h"

#include <iomanip>
#include <unordered_set>

namespace SOMTParser::Unified {

// ── Helpers ────────────────────────────────────────────────────────

std::string UnifiedPrinter::escapeString(const std::string& s) const {
    std::ostringstream oss;
    oss << '"';
    for (char c : s) {
        switch (c) {
            case '"': oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\n': oss << "\\n"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    oss << '"';
    return oss.str();
}

std::string UnifiedPrinter::mangleIdent(const std::string& name) const {
    // SMT-LIB identifiers: pipe-quote if contains special chars
    bool needsQuote = false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '~' && c != '!' && c != '@' && c != '$' && c != '%' && c != '^' && c != '&' && c != '*' && c != '-' && c != '+' && c != '=' && c != '<' && c != '>' && c != '.' && c != '?' && c != '/') {
            needsQuote = true;
            break;
        }
    }
    if (!needsQuote) return name;
    std::string result = "|" + name + "|";
    return result;
}

void UnifiedPrinter::printLiteral(std::ostream& os, const UnifiedExpr::Literal& lit) const {
    switch (lit.lit_kind) {
        case UnifiedExpr::Literal::LitKind::BOOL:
            os << (std::get<bool>(lit.value) ? "true" : "false");
            break;
        case UnifiedExpr::Literal::LitKind::INT:
            os << std::get<int64_t>(lit.value);
            break;
        case UnifiedExpr::Literal::LitKind::FLOAT:
            os << std::setprecision(17) << std::get<double>(lit.value);
            break;
        case UnifiedExpr::Literal::LitKind::STRING:
            os << escapeString(std::get<std::string>(lit.value));
            break;
    }
}

// ── SMT-LIB expression printer ─────────────────────────────────────

void UnifiedPrinter::printExprSmtLibImpl(std::ostream& os, ExprPtr expr) const {
    if (!expr) { os << "null"; return; }

    switch (expr->kind) {
        case UnifiedExpr::Kind::LITERAL: {
            printLiteral(os, *expr->asLiteral());
            break;
        }

        case UnifiedExpr::Kind::IDENT: {
            os << mangleIdent(expr->asIdent()->name);
            break;
        }

        case UnifiedExpr::Kind::OP: {
            auto* op = expr->asOp();
            if (!op) { os << "null_op"; break; }
            const auto* def = registry_.getDef(op->op);
            std::string op_name;
            if (def && def->smt_lowering.strategy == SmtLoweringDef::Strategy::NATIVE && !def->smt_lowering.native_smt_name.empty()) {
                op_name = def->smt_lowering.native_smt_name;
            } else if (def) {
                op_name = def->unified_name;
            } else {
                op_name = "unknown_op";
            }
            os << "(" << mangleIdent(op_name);
            for (auto& arg : op->args) {
                os << " ";
                printExprSmtLibImpl(os, arg);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::ARRAY_LIT: {
            auto* arr = expr->asArray();
            os << "(";
            for (size_t i = 0; i < arr->elems.size(); ++i) {
                if (i > 0) os << " ";
                printExprSmtLibImpl(os, arr->elems[i]);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::SET_LIT: {
            auto* s = expr->asSet();
            // SMT-LIB set literals: not standard, use a placeholder
            os << "(as-set";
            for (auto& e : s->elems) {
                os << " ";
                printExprSmtLibImpl(os, e);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::TUPLE_LIT: {
            auto* t = expr->as<UnifiedExpr::TupleLit>();
            os << "(mkTuple";
            for (auto& e : t->elems) {
                os << " ";
                printExprSmtLibImpl(os, e);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::RECORD_LIT: {
            auto* r = expr->as<UnifiedExpr::RecordLit>();
            os << "(mkRecord";
            for (auto& [name, val] : r->fields) {
                os << " (:" << name << " ";
                printExprSmtLibImpl(os, val);
                os << ")";
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::LET: {
            auto* let = expr->asLet();
            os << "(let (";
            for (auto& local : let->locals) {
                os << "(" << mangleIdent(local.name) << " ";
                if (local.init) printExprSmtLibImpl(os, local.init);
                else os << "null";
                os << ")";
            }
            os << ") ";
            printExprSmtLibImpl(os, let->body);
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::ITE: {
            auto* ite = expr->asIte();
            os << "(ite ";
            printExprSmtLibImpl(os, ite->cond);
            os << " ";
            printExprSmtLibImpl(os, ite->then_expr);
            os << " ";
            printExprSmtLibImpl(os, ite->else_expr);
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::FORALL: {
            auto* q = expr->asQuant();
            os << "(forall (";
            for (auto& [var, set_expr] : q->generators) {
                os << "(" << mangleIdent(var) << " ";
                if (set_expr) printExprSmtLibImpl(os, set_expr);
                else os << "Int"; // fallback
                os << ")";
            }
            os << ") ";
            printExprSmtLibImpl(os, q->body);
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::EXISTS: {
            auto* q = expr->asQuant();
            os << "(exists (";
            for (auto& [var, set_expr] : q->generators) {
                os << "(" << mangleIdent(var) << " ";
                if (set_expr) printExprSmtLibImpl(os, set_expr);
                else os << "Int";
                os << ")";
            }
            os << ") ";
            printExprSmtLibImpl(os, q->body);
            os << ")";
            break;
        }
    }
}

// ── MiniZinc expression printer ────────────────────────────────────

void UnifiedPrinter::printExprMiniZincImpl(std::ostream& os, ExprPtr expr) const {
    if (!expr) { os << "null"; return; }

    switch (expr->kind) {
        case UnifiedExpr::Kind::LITERAL: {
            printLiteral(os, *expr->asLiteral());
            break;
        }

        case UnifiedExpr::Kind::IDENT: {
            os << expr->asIdent()->name;
            break;
        }

        case UnifiedExpr::Kind::OP: {
            auto* op = expr->asOp();
            if (!op) { os << "null_op"; break; }
            const auto* def = registry_.getDef(op->op);
            std::string op_name;
            if (def) {
                auto it = def->lang_names.find("minizinc");
                if (it != def->lang_names.end()) op_name = it->second;
                else op_name = def->unified_name;
            } else {
                op_name = "unknown_op";
            }

            // Infix operators (MiniZinc syntax)
            static const std::unordered_set<std::string> infix = {
                "+", "-", "*", "/", "div", "mod", "pow",
                "<", ">", "<=", ">=", "=", "!=",
                "/\\", "\\/", "->", "<->", "xor"
            };
            static const std::unordered_set<std::string> prefix = {
                "not", "-"
            };

            if (prefix.count(op_name) && op->args.size() == 1) {
                os << op_name;
                // Add space if needed
                if (op_name == "not") os << "(";
                printExprMiniZincImpl(os, op->args[0]);
                if (op_name == "not") os << ")";
            } else if (infix.count(op_name) && op->args.size() == 2) {
                os << "(";
                printExprMiniZincImpl(os, op->args[0]);
                os << " " << op_name << " ";
                printExprMiniZincImpl(os, op->args[1]);
                os << ")";
            } else {
                // Function call style
                os << op_name << "(";
                for (size_t i = 0; i < op->args.size(); ++i) {
                    if (i > 0) os << ", ";
                    printExprMiniZincImpl(os, op->args[i]);
                }
                os << ")";
            }
            break;
        }

        case UnifiedExpr::Kind::ARRAY_LIT: {
            auto* arr = expr->asArray();
            os << "[";
            for (size_t i = 0; i < arr->elems.size(); ++i) {
                if (i > 0) os << ", ";
                printExprMiniZincImpl(os, arr->elems[i]);
            }
            os << "]";
            break;
        }

        case UnifiedExpr::Kind::SET_LIT: {
            auto* s = expr->asSet();
            os << "{";
            for (size_t i = 0; i < s->elems.size(); ++i) {
                if (i > 0) os << ", ";
                printExprMiniZincImpl(os, s->elems[i]);
            }
            os << "}";
            break;
        }

        case UnifiedExpr::Kind::TUPLE_LIT: {
            auto* t = expr->as<UnifiedExpr::TupleLit>();
            os << "(";
            for (size_t i = 0; i < t->elems.size(); ++i) {
                if (i > 0) os << ", ";
                printExprMiniZincImpl(os, t->elems[i]);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::RECORD_LIT: {
            auto* r = expr->as<UnifiedExpr::RecordLit>();
            os << "(";
            for (size_t i = 0; i < r->fields.size(); ++i) {
                if (i > 0) os << ", ";
                os << r->fields[i].first << ": ";
                printExprMiniZincImpl(os, r->fields[i].second);
            }
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::LET: {
            auto* let = expr->asLet();
            os << "let {\n";
            for (auto& local : let->locals) {
                os << "  ";
                printVarDeclMiniZinc(os, local);
                os << ";\n";
            }
            os << "} in ";
            printExprMiniZincImpl(os, let->body);
            break;
        }

        case UnifiedExpr::Kind::ITE: {
            auto* ite = expr->asIte();
            os << "if ";
            printExprMiniZincImpl(os, ite->cond);
            os << " then ";
            printExprMiniZincImpl(os, ite->then_expr);
            os << " else ";
            printExprMiniZincImpl(os, ite->else_expr);
            os << " endif";
            break;
        }

        case UnifiedExpr::Kind::FORALL: {
            auto* q = expr->asQuant();
            os << "forall(";
            for (size_t i = 0; i < q->generators.size(); ++i) {
                if (i > 0) os << ", ";
                os << q->generators[i].first << " in ";
                printExprMiniZincImpl(os, q->generators[i].second);
            }
            os << ")(";
            printExprMiniZincImpl(os, q->body);
            os << ")";
            break;
        }

        case UnifiedExpr::Kind::EXISTS: {
            auto* q = expr->asQuant();
            os << "exists(";
            for (size_t i = 0; i < q->generators.size(); ++i) {
                if (i > 0) os << ", ";
                os << q->generators[i].first << " in ";
                printExprMiniZincImpl(os, q->generators[i].second);
            }
            os << ")(";
            printExprMiniZincImpl(os, q->body);
            os << ")";
            break;
        }
    }
}

// ── Variable declarations ──────────────────────────────────────────

void UnifiedPrinter::printVarDeclMiniZinc(std::ostream& os, const UnifiedVarDecl& decl) const {
    os << (decl.type.isPar() ? "par " : "var ");
    os << typeToMiniZinc(decl.type);
    os << ": " << decl.name;
    if (decl.init) {
        os << " = ";
        printExprMiniZincImpl(os, decl.init);
    }
}

void UnifiedPrinter::printVarDeclSmtLib(std::ostream& os, const UnifiedVarDecl& decl) const {
    os << "(declare-fun " << mangleIdent(decl.name) << " () " << typeToSmtLib(decl.type) << ")";
    if (decl.init) {
        os << "\n(assert (= " << mangleIdent(decl.name) << " ";
        printExprSmtLibImpl(os, decl.init);
        os << "))";
    }
}

// ── Type printers ──────────────────────────────────────────────────

std::string UnifiedPrinter::typeToMiniZinc(const UnifiedType& type) const {
    switch (type.sort.kind) {
        case UnifiedSort::Kind::BOOL: return "bool";
        case UnifiedSort::Kind::INT:  return "int";
        case UnifiedSort::Kind::REAL: return "float";
        case UnifiedSort::Kind::FLOAT: return "float";
        case UnifiedSort::Kind::STRING: return "string";
        case UnifiedSort::Kind::SET: {
            std::string elem = "int";
            if (!type.sort.params.empty()) {
                UnifiedType et(type.sort.params[0]);
                elem = typeToMiniZinc(et);
            }
            return "set of " + elem;
        }
        case UnifiedSort::Kind::ARRAY: {
            std::string result = "array[";
            for (size_t i = 0; i < type.array_dims.size(); ++i) {
                if (i > 0) result += ", ";
                result += "int";
            }
            result += "] of ";
            if (!type.sort.params.empty()) {
                UnifiedType et(type.sort.params[0]);
                result += typeToMiniZinc(et);
            } else {
                result += "int";
            }
            return result;
        }
        case UnifiedSort::Kind::ENUM: return "enum";
        case UnifiedSort::Kind::TUPLE: return "tuple";
        case UnifiedSort::Kind::RECORD: return "record";
        default: return "var int";
    }
}

std::string UnifiedPrinter::typeToSmtLib(const UnifiedType& type) const {
    switch (type.sort.kind) {
        case UnifiedSort::Kind::BOOL: return "Bool";
        case UnifiedSort::Kind::INT:  return "Int";
        case UnifiedSort::Kind::REAL: return "Real";
        case UnifiedSort::Kind::FLOAT: return "Float";
        case UnifiedSort::Kind::STRING: return "String";
        case UnifiedSort::Kind::ARRAY: return "(Array Int Int)";
        case UnifiedSort::Kind::SET:  return "(Set Int)";
        default: return "Int";
    }
}

// ── Model printers ─────────────────────────────────────────────────

void UnifiedPrinter::printSmtLib(std::ostream& os, const UnifiedModel& model) const {
    os << "; Unified IR -> SMT-LIB2\n";
    for (const auto& decl : model.vars) {
        printVarDeclSmtLib(os, decl);
        os << "\n";
    }
    for (const auto& c : model.constraints) {
        os << "(assert ";
        printExprSmtLibImpl(os, c.expr);
        os << ")\n";
    }
    if (!model.objectives.empty()) {
        // SMT-LIB doesn't have native optimize, but we can emit as comments or push
        for (const auto& o : model.objectives) {
            os << "; objective: ";
            switch (o.mode) {
                case UnifiedObjective::Mode::SATISFY: os << "satisfy"; break;
                case UnifiedObjective::Mode::MINIMIZE: os << "minimize"; break;
                case UnifiedObjective::Mode::MAXIMIZE: os << "maximize"; break;
            }
            if (o.expr) {
                os << " ";
                printExprSmtLibImpl(os, o.expr);
            }
            os << "\n";
        }
    }
    os << "(check-sat)\n";
}

void UnifiedPrinter::printMiniZinc(std::ostream& os, const UnifiedModel& model) const {
    os << "% Unified IR -> MiniZinc\n";
    for (const auto& decl : model.vars) {
        printVarDeclMiniZinc(os, decl);
        os << ";\n";
    }
    for (const auto& c : model.constraints) {
        os << "constraint ";
        printExprMiniZincImpl(os, c.expr);
        os << ";\n";
    }
    for (const auto& o : model.objectives) {
        switch (o.mode) {
            case UnifiedObjective::Mode::SATISFY: os << "solve satisfy;\n"; break;
            case UnifiedObjective::Mode::MINIMIZE:
                os << "solve minimize ";
                printExprMiniZincImpl(os, o.expr);
                os << ";\n";
                break;
            case UnifiedObjective::Mode::MAXIMIZE:
                os << "solve maximize ";
                printExprMiniZincImpl(os, o.expr);
                os << ";\n";
                break;
        }
    }
    for (const auto& out : model.outputs) {
        os << "output ";
        printExprMiniZincImpl(os, out);
        os << ";\n";
    }
}

// ── String wrappers ────────────────────────────────────────────────

std::string UnifiedPrinter::toSmtLib(const UnifiedModel& model) const {
    std::ostringstream oss;
    printSmtLib(oss, model);
    return oss.str();
}

std::string UnifiedPrinter::toMiniZinc(const UnifiedModel& model) const {
    std::ostringstream oss;
    printMiniZinc(oss, model);
    return oss.str();
}

std::string UnifiedPrinter::exprToSmtLib(ExprPtr expr) const {
    std::ostringstream oss;
    printExprSmtLibImpl(oss, expr);
    return oss.str();
}

std::string UnifiedPrinter::exprToMiniZinc(ExprPtr expr) const {
    std::ostringstream oss;
    printExprMiniZincImpl(oss, expr);
    return oss.str();
}

// ── Debug printer ──────────────────────────────────────────────────

std::string UnifiedPrinter::toDebugString(ExprPtr expr) const {
    std::ostringstream oss;
    printDebug(oss, expr, 0);
    return oss.str();
}

void UnifiedPrinter::printDebug(std::ostream& os, ExprPtr expr, int indent) const {
    if (!expr) { os << std::string(indent * 2, ' ') << "null\n"; return; }

    os << std::string(indent * 2, ' ');
    switch (expr->kind) {
        case UnifiedExpr::Kind::LITERAL: {
            auto* lit = expr->asLiteral();
            os << "LITERAL(";
            printLiteral(os, *lit);
            os << ")\n";
            break;
        }
        case UnifiedExpr::Kind::IDENT:
            os << "IDENT(" << expr->asIdent()->name << ")\n";
            break;
        case UnifiedExpr::Kind::OP: {
            auto* op = expr->asOp();
            const auto* def = registry_.getDef(op->op);
            os << "OP(" << (def ? def->unified_name : "unknown") << ")\n";
            for (auto& arg : op->args) {
                printDebug(os, arg, indent + 1);
            }
            break;
        }
        case UnifiedExpr::Kind::ARRAY_LIT: {
            os << "ARRAY_LIT\n";
            for (auto& e : expr->asArray()->elems) {
                printDebug(os, e, indent + 1);
            }
            break;
        }
        case UnifiedExpr::Kind::SET_LIT: {
            os << "SET_LIT\n";
            for (auto& e : expr->asSet()->elems) {
                printDebug(os, e, indent + 1);
            }
            break;
        }
        case UnifiedExpr::Kind::TUPLE_LIT: {
            os << "TUPLE_LIT\n";
            for (auto& e : expr->as<UnifiedExpr::TupleLit>()->elems) {
                printDebug(os, e, indent + 1);
            }
            break;
        }
        case UnifiedExpr::Kind::RECORD_LIT: {
            os << "RECORD_LIT\n";
            for (auto& [name, val] : expr->as<UnifiedExpr::RecordLit>()->fields) {
                os << std::string((indent + 1) * 2, ' ') << name << ":\n";
                printDebug(os, val, indent + 2);
            }
            break;
        }
        case UnifiedExpr::Kind::LET: {
            auto* let = expr->asLet();
            os << "LET\n";
            for (auto& local : let->locals) {
                os << std::string((indent + 1) * 2, ' ') << "var " << local.name << ":\n";
                if (local.init) printDebug(os, local.init, indent + 2);
            }
            os << std::string((indent + 1) * 2, ' ') << "body:\n";
            printDebug(os, let->body, indent + 2);
            break;
        }
        case UnifiedExpr::Kind::ITE: {
            os << "ITE\n";
            os << std::string((indent + 1) * 2, ' ') << "cond:\n";
            printDebug(os, expr->asIte()->cond, indent + 2);
            os << std::string((indent + 1) * 2, ' ') << "then:\n";
            printDebug(os, expr->asIte()->then_expr, indent + 2);
            os << std::string((indent + 1) * 2, ' ') << "else:\n";
            printDebug(os, expr->asIte()->else_expr, indent + 2);
            break;
        }
        case UnifiedExpr::Kind::FORALL:
            os << "FORALL\n";
            printDebug(os, expr->asQuant()->body, indent + 1);
            break;
        case UnifiedExpr::Kind::EXISTS:
            os << "EXISTS\n";
            printDebug(os, expr->asQuant()->body, indent + 1);
            break;
    }
}

} // namespace SOMTParser::Unified
