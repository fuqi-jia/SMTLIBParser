/* -*- C++ -*-
 *
 * SMT-LIB AST → Unified IR converter implementation.
 */

#include "somtparser/frontends/smt/smt_to_unified.h"

#include <iostream>
#include <sstream>

namespace SOMTParser {

namespace U = SOMTParser::Unified;

using UExprPtr = U::ExprPtr;
using UOpRef   = U::UnifiedOpRef;
using USort    = U::UnifiedSort;
using ULit     = U::UnifiedExpr::Literal;
using UIdent   = U::UnifiedExpr::Ident;
using UOpNode  = U::UnifiedExpr::OpNode;
using UQuant   = U::UnifiedExpr::QuantExpr;
using ULet     = U::UnifiedExpr::LetExpr;
using UIte     = U::UnifiedExpr::IteExpr;

// ── Constructor ────────────────────────────────────────────────────

SmtLibToUnifiedIR::SmtLibToUnifiedIR(const U::UnifiedOpRegistry& registry)
    : registry_(registry) {}

// ── Error handling ─────────────────────────────────────────────────

void SmtLibToUnifiedIR::addError(const std::string& msg) {
    errors_.push_back(msg);
    std::cerr << "[SmtLibToUnifiedIR] " << msg << "\n";
}

// ── Sort conversion ────────────────────────────────────────────────

USort SmtLibToUnifiedIR::convertSort(const std::shared_ptr<Sort>& sort) const {
    if (!sort) return USort::mkAny();

    switch (sort->kind) {
        case SORT_KIND::SK_BOOL:   return USort::mkBool();
        case SORT_KIND::SK_INT:
        case SORT_KIND::SK_NAT:
        case SORT_KIND::SK_INTOREAL:
            return USort::mkInt();
        case SORT_KIND::SK_REAL:
        case SORT_KIND::SK_RAT:
        case SORT_KIND::SK_ALGEBRAIC:
        case SORT_KIND::SK_TRANSCENDENTAL:
            return USort::mkReal();
        case SORT_KIND::SK_BV:
            // Bit-vectors map to INT in the unified layer (simplification)
            return USort::mkInt();
        case SORT_KIND::SK_FP:
            return USort::mkFloat();
        case SORT_KIND::SK_STR:
        case SORT_KIND::SK_REG:
            return USort::mkString();
        case SORT_KIND::SK_ARRAY: {
            // Array sort has children: [index_sort, elem_sort]
            if (sort->children.size() >= 2) {
                auto elem = convertSort(sort->children[1]);
                return USort::mkArray(elem);
            }
            return USort::mkArray(USort::mkAny());
        }
        case SORT_KIND::SK_SET:
            if (!sort->children.empty()) {
                return USort::mkSet(convertSort(sort->children[0]));
            }
            return USort::mkSet(USort::mkAny());
        default:
            return USort::mkAny();
    }
}

// ── NODE_KIND → SMT-LIB name mapping ───────────────────────────────

std::string SmtLibToUnifiedIR::nodeKindToSmtName(NODE_KIND kind) const {
    switch (kind) {
        // Boolean
        case NODE_KIND::NT_AND:      return "and";
        case NODE_KIND::NT_OR:       return "or";
        case NODE_KIND::NT_NOT:      return "not";
        case NODE_KIND::NT_IMPLIES:  return "=>";
        case NODE_KIND::NT_XOR:      return "xor";

        // Core
        case NODE_KIND::NT_EQ:
        case NODE_KIND::NT_EQ_BOOL:
        case NODE_KIND::NT_EQ_OTHER:
            return "=";
        case NODE_KIND::NT_DISTINCT:
        case NODE_KIND::NT_DISTINCT_BOOL:
        case NODE_KIND::NT_DISTINCT_OTHER:
            return "distinct";
        case NODE_KIND::NT_ITE:      return "ite";

        // Arithmetic comparison
        case NODE_KIND::NT_LT:       return "<";
        case NODE_KIND::NT_LE:       return "<=";
        case NODE_KIND::NT_GT:       return ">";
        case NODE_KIND::NT_GE:       return ">=";

        // Arithmetic operators
        case NODE_KIND::NT_ADD:      return "+";
        case NODE_KIND::NT_NEG:
        case NODE_KIND::NT_SUB:      return "-";
        case NODE_KIND::NT_MUL:      return "*";
        case NODE_KIND::NT_DIV_INT:  return "div";
        case NODE_KIND::NT_DIV_REAL: return "/";
        case NODE_KIND::NT_MOD:      return "mod";
        case NODE_KIND::NT_ABS:      return "abs";
        case NODE_KIND::NT_POW:      return "^";
        case NODE_KIND::NT_POW2:     return "pow2";

        // Conversion
        case NODE_KIND::NT_TO_INT:   return "to_int";
        case NODE_KIND::NT_TO_REAL:  return "to_real";

        // Array
        case NODE_KIND::NT_SELECT:   return "select";
        case NODE_KIND::NT_STORE:    return "store";

        // String
        case NODE_KIND::NT_STR_LEN:       return "str.len";
        case NODE_KIND::NT_STR_CONCAT:    return "str.++";
        case NODE_KIND::NT_STR_SUBSTR:    return "str.substr";
        case NODE_KIND::NT_STR_INDEXOF:   return "str.indexof";
        case NODE_KIND::NT_STR_PREFIXOF:  return "str.prefixof";
        case NODE_KIND::NT_STR_SUFFIXOF:  return "str.suffixof";
        case NODE_KIND::NT_STR_CONTAINS:  return "str.contains";
        case NODE_KIND::NT_STR_REPLACE:   return "str.replace";
        case NODE_KIND::NT_STR_REPLACE_ALL: return "str.replace_all";
        case NODE_KIND::NT_STR_TO_INT:    return "str.to_int";
        case NODE_KIND::NT_STR_FROM_INT:  return "str.from_int";
        case NODE_KIND::NT_STR_LT:        return "str.<";
        case NODE_KIND::NT_STR_LE:        return "str.<=";

        // Quantifiers
        case NODE_KIND::NT_FORALL:   return "forall";
        case NODE_KIND::NT_EXISTS:   return "exists";

        // Uninterpreted function
        case NODE_KIND::NT_UF_APPLY: return "uf_apply";

        default:
            return "";
    }
}

// ── Registry lookup ────────────────────────────────────────────────

UOpRef SmtLibToUnifiedIR::lookupOpBySmtName(const std::string& name) const {
    return registry_.lookupByLangName("smtlib", name);
}

// ── Script conversion ──────────────────────────────────────────────

U::UnifiedModel SmtLibToUnifiedIR::convert(const Script& script) {
    U::UnifiedModel model;
    for (const auto& cmd : script.commands()) {
        convertCommand(cmd, model);
    }
    return model;
}

// ── Parser conversion ──────────────────────────────────────────────

U::UnifiedModel SmtLibToUnifiedIR::convert(const Parser& parser) {
    U::UnifiedModel model;

    // Variables
    for (const auto& var : parser.getVariables()) {
        if (!var) continue;
        U::UnifiedVarDecl decl;
        decl.name = var->getPureName();
        decl.type = U::UnifiedType(convertSort(var->getSort()));
        model.addVar(std::move(decl));
    }

    // Declared functions (as variables with function sort)
    for (const auto& func : parser.getFunctions()) {
        if (!func) continue;
        if (func->isFuncDec() || func->isFuncDef() || func->isFuncRec()) {
            U::UnifiedVarDecl decl;
            decl.name = func->getName();
            decl.type = U::UnifiedType(convertSort(func->getSort()));
            if (func->isFuncDef() && !func->getFuncBody()->isNull()) {
                decl.init = convertExpr(func->getFuncBody());
            }
            model.addVar(std::move(decl));
        }
    }

    // Assertions
    for (const auto& assertion : parser.getAssertions()) {
        if (!assertion) continue;
        auto expr = convertExpr(assertion);
        if (expr) {
            model.addConstraint(U::UnifiedConstraint(expr));
        }
    }

    // Objectives
    for (const auto& obj : parser.getObjectives()) {
        if (!obj) continue;
        U::UnifiedObjective::Mode mode = U::UnifiedObjective::Mode::SATISFY;
        switch (obj->getObjectiveKind()) {
            case OPT_KIND::OPT_MINIMIZE: mode = U::UnifiedObjective::Mode::MINIMIZE; break;
            case OPT_KIND::OPT_MAXIMIZE: mode = U::UnifiedObjective::Mode::MAXIMIZE; break;
            default: break;
        }
        auto term = obj->getObjectiveTerm();
        auto expr = term ? convertExpr(term) : nullptr;
        if (expr || mode == U::UnifiedObjective::Mode::SATISFY) {
            model.addObjective(U::UnifiedObjective(mode, expr));
        }
    }

    return model;
}

// ── Command conversion ─────────────────────────────────────────────

void SmtLibToUnifiedIR::convertCommand(const Command& cmd, U::UnifiedModel& model) {
    switch (cmd.type) {
        case CMD_TYPE::CT_DECLARE_CONST:
        case CMD_TYPE::CT_DECLARE_FUN: {
            U::UnifiedVarDecl decl;
            decl.name = cmd.name;
            if (cmd.sort) {
                decl.type = U::UnifiedType(convertSort(cmd.sort));
            }
            model.addVar(std::move(decl));
            break;
        }

        case CMD_TYPE::CT_DEFINE_FUN: {
            // Treat defined function as a variable with initialization
            U::UnifiedVarDecl decl;
            decl.name = cmd.name;
            if (cmd.sort) {
                decl.type = U::UnifiedType(convertSort(cmd.sort));
            }
            if (cmd.expr) {
                decl.init = convertExpr(cmd.expr);
            }
            model.addVar(std::move(decl));
            break;
        }

        case CMD_TYPE::CT_ASSERT: {
            if (cmd.expr) {
                auto expr = convertExpr(cmd.expr);
                if (expr) {
                    model.addConstraint(U::UnifiedConstraint(expr));
                }
            }
            break;
        }

        case CMD_TYPE::CT_MINIMIZE: {
            if (cmd.expr) {
                auto expr = convertExpr(cmd.expr);
                if (expr) {
                    U::UnifiedObjective obj(U::UnifiedObjective::Mode::MINIMIZE, expr);
                    model.addObjective(std::move(obj));
                }
            }
            break;
        }

        case CMD_TYPE::CT_MAXIMIZE: {
            if (cmd.expr) {
                auto expr = convertExpr(cmd.expr);
                if (expr) {
                    U::UnifiedObjective obj(U::UnifiedObjective::Mode::MAXIMIZE, expr);
                    model.addObjective(std::move(obj));
                }
            }
            break;
        }

        case CMD_TYPE::CT_SET_LOGIC: {
            // Logic hint can be stored as metadata if needed;
            // UnifiedModel does not have a logic field yet.
            break;
        }

        case CMD_TYPE::CT_PUSH:
        case CMD_TYPE::CT_POP:
        case CMD_TYPE::CT_CHECK_SAT:
        case CMD_TYPE::CT_GET_MODEL:
        case CMD_TYPE::CT_GET_VALUE:
            // Solver interaction commands; not part of the model
            break;

        default:
            // Unknown or unhandled command type
            break;
    }
}

// ── Expression dispatch ────────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertExpr(const std::shared_ptr<DAGNode>& node) {
    if (!node) return nullptr;
    return dispatchExpr(node);
}

UExprPtr SmtLibToUnifiedIR::dispatchExpr(const std::shared_ptr<DAGNode>& node) {
    NODE_KIND kind = node->getKind();

    // Literals / constants
    if (node->isConst() || node->isTrue() || node->isFalse() ||
        kind == NODE_KIND::NT_CONST_PI || kind == NODE_KIND::NT_CONST_E ||
        kind == NODE_KIND::NT_INFINITY || kind == NODE_KIND::NT_NAN ||
        kind == NODE_KIND::NT_EPSILON) {
        return convertLiteral(node);
    }

    // Variables (including quantified and let-bound)
    if (node->isVar() || node->isTempVar() || node->isQuantVar() ||
        node->isLetBindVar() || node->isPlaceholderVar()) {
        return convertIdent(node);
    }

    // ITE
    if (node->isIte()) {
        return convertIte(node);
    }

    // Quantifiers
    if (node->getKind() == NODE_KIND::NT_FORALL || node->getKind() == NODE_KIND::NT_EXISTS) {
        return convertQuantifier(node);
    }

    // Let
    if (node->isLet() || node->isLetChain()) {
        return convertLet(node);
    }

    // Array operations
    if (node->isSelect() || node->isStore()) {
        return convertArrayOp(node);
    }

    // Uninterpreted function application
    if (node->isUFApplication()) {
        return convertUFApplication(node);
    }

    // Constructor / selector / tester / match
    if (node->isConstructorApp() || node->isSelectorApp() ||
        node->isTesterApp() || node->isMatchApp()) {
        return convertOpNode(node);
    }

    // Default: try to map via registry using the SMT-LIB name
    return convertOpNode(node);
}

// ── Literal conversion ─────────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertLiteral(const std::shared_ptr<DAGNode>& node) {
    auto expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LITERAL, U::SourceLoc{});

    if (node->isTrue()) {
        expr->data = ULit::mkBool(true);
        return expr;
    }
    if (node->isFalse()) {
        expr->data = ULit::mkBool(false);
        return expr;
    }

    auto sort = node->getSort();
    const std::string& name = node->getName();

    if (node->isCInt() || (sort && sort->isInt())) {
        try {
            int64_t val = std::stoll(name);
            expr->data = ULit::mkInt(val);
        } catch (...) {
            // Fallback: store as string literal
            expr->data = ULit::mkString(name);
        }
        return expr;
    }

    if (node->isCReal() || (sort && sort->isReal())) {
        try {
            double val = std::stod(name);
            expr->data = ULit::mkFloat(val);
        } catch (...) {
            expr->data = ULit::mkString(name);
        }
        return expr;
    }

    if (node->isCBV() || (sort && sort->isBv())) {
        // BV constants: try to parse #b / #x / decimal
        try {
            if (name.size() >= 2 && name[0] == '#') {
                if (name[1] == 'b') {
                    int64_t val = 0;
                    for (size_t i = 2; i < name.size(); ++i) {
                        val = val * 2 + (name[i] == '1' ? 1 : 0);
                    }
                    expr->data = ULit::mkInt(val);
                } else if (name[1] == 'x') {
                    int64_t val = std::stoll(name.substr(2), nullptr, 16);
                    expr->data = ULit::mkInt(val);
                } else {
                    expr->data = ULit::mkString(name);
                }
            } else {
                int64_t val = std::stoll(name);
                expr->data = ULit::mkInt(val);
            }
        } catch (...) {
            expr->data = ULit::mkString(name);
        }
        return expr;
    }

    if (node->isCStr() || (sort && sort->isStr())) {
        // Strip SMT-LIB quotes if present
        std::string s = name;
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
            s = s.substr(1, s.size() - 2);
            // Unescape ""
            std::string result;
            result.reserve(s.size());
            for (size_t i = 0; i < s.size(); ++i) {
                if (i + 1 < s.size() && s[i] == '"' && s[i+1] == '"') {
                    result += '"';
                    ++i;
                } else {
                    result += s[i];
                }
            }
            s = result;
        }
        expr->data = ULit::mkString(s);
        return expr;
    }

    // Fallback: treat as string literal
    expr->data = ULit::mkString(name);
    return expr;
}

// ── Identifier conversion ──────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertIdent(const std::shared_ptr<DAGNode>& node) {
    auto expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::IDENT, U::SourceLoc{});
    expr->data = UIdent{node->getPureName()};
    return expr;
}

// ── Operation node conversion ──────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertOpNode(const std::shared_ptr<DAGNode>& node) {
    std::string smt_name = nodeKindToSmtName(node->getKind());

    // For UF applications and datatype ops, use the node's name
    if (smt_name.empty() && !node->getName().empty()) {
        smt_name = node->getName();
    }

    // Fallback: use kind name as string
    if (smt_name.empty()) {
        std::ostringstream oss;
        oss << "kind_" << static_cast<int>(node->getKind());
        smt_name = oss.str();
    }

    UOpRef op_ref = lookupOpBySmtName(smt_name);

    // If not found in registry, we still create an OpNode with an invalid ref.
    // The consumer (e.g. printer) can fall back to the raw name if needed.
    // For now we leave op_ref.id == 0 as a sentinel.
    (void)op_ref; // may be unused if we add name-based fallback later

    auto expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    UOpNode op_node;
    op_node.op = op_ref;

    for (size_t i = 0; i < node->getChildrenSize(); ++i) {
        auto child = convertExpr(node->getChild(i));
        if (child) {
            op_node.args.push_back(child);
        }
    }

    expr->data = std::move(op_node);
    return expr;
}

// ── ITE conversion ─────────────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertIte(const std::shared_ptr<DAGNode>& node) {
    if (node->getChildrenSize() < 3) {
        addError("ITE node has fewer than 3 children");
        return nullptr;
    }

    auto expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::ITE, U::SourceLoc{});
    UIte ite;
    ite.cond      = convertExpr(node->getChild(0));
    ite.then_expr = convertExpr(node->getChild(1));
    ite.else_expr = convertExpr(node->getChild(2));
    expr->data = std::move(ite);
    return expr;
}

// ── Quantifier conversion ──────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertQuantifier(const std::shared_ptr<DAGNode>& node) {
    size_t n = node->getChildrenSize();
    if (n == 0) {
        addError("Quantifier node has no children");
        return nullptr;
    }

    // SMT-LIB quantifier structure: child[0] = body, child[1..n-1] = bound variables.
    auto quant_expr = std::make_shared<U::UnifiedExpr>(
        node->getKind() == NODE_KIND::NT_FORALL ? U::UnifiedExpr::Kind::FORALL : U::UnifiedExpr::Kind::EXISTS,
        U::SourceLoc{});

    UQuant quant;
    quant.body = convertExpr(node->getChild(0));

    for (size_t i = 1; i < n; ++i) {
        auto var_node = node->getChild(i);
        std::string var_name = var_node->getPureName();
        // No set expression for pure SMT-LIB quantifiers (range is implicit)
        quant.generators.emplace_back(var_name, nullptr);
    }

    quant_expr->data = std::move(quant);
    return quant_expr;
}

// ── Let conversion ─────────────────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertLet(const std::shared_ptr<DAGNode>& node) {
    auto let_expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::LET, U::SourceLoc{});
    ULet let;

    // Handle let-chain: children are [bind_var_list_1, ..., bind_var_list_n, body]
    size_t n = node->getChildrenSize();
    if (n == 0) {
        addError("Let node has no children");
        return nullptr;
    }

    // Last child is the body
    let.body = convertExpr(node->getChild(n - 1));

    // All but the last child are bind-var lists
    for (size_t i = 0; i + 1 < n; ++i) {
        auto list_node = node->getChild(i);
        if (!list_node->isLetBindVarList()) {
            // Single let-bind-var directly
            if (list_node->isLetBindVar()) {
                U::UnifiedVarDecl decl;
                decl.name = list_node->getPureName();
                if (list_node->getChildrenSize() > 0) {
                    decl.init = convertExpr(list_node->getChild(0));
                }
                let.locals.push_back(std::move(decl));
            }
            continue;
        }

        // Process bind-var list
        for (size_t j = 0; j < list_node->getChildrenSize(); ++j) {
            auto bind_var = list_node->getChild(j);
            if (!bind_var->isLetBindVar()) continue;

            U::UnifiedVarDecl decl;
            decl.name = bind_var->getPureName();
            if (bind_var->getChildrenSize() > 0) {
                decl.init = convertExpr(bind_var->getChild(0));
            }
            let.locals.push_back(std::move(decl));
        }
    }

    let_expr->data = std::move(let);
    return let_expr;
}

// ── Array operation conversion ─────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertArrayOp(const std::shared_ptr<DAGNode>& node) {
    // select and store are mapped via the registry just like other ops
    return convertOpNode(node);
}

// ── UF application conversion ──────────────────────────────────────

UExprPtr SmtLibToUnifiedIR::convertUFApplication(const std::shared_ptr<DAGNode>& node) {
    auto expr = std::make_shared<U::UnifiedExpr>(U::UnifiedExpr::Kind::OP, U::SourceLoc{});
    UOpNode op_node;

    // UF applications use the node's name as the function identifier.
    // We do not require them to be in the registry.
    op_node.op = UOpRef{}; // invalid / sentinel

    for (size_t i = 0; i < node->getChildrenSize(); ++i) {
        auto child = convertExpr(node->getChild(i));
        if (child) {
            op_node.args.push_back(child);
        }
    }

    expr->data = std::move(op_node);
    return expr;
}

} // namespace SOMTParser
