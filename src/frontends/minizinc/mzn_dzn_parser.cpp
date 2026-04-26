/* -*- C++ -*-
 *
 * MiniZinc Frontend — .dzn Data File Parser & Merger Implementation
 */

#include "somtparser/frontends/minizinc/mzn_dzn_parser.h"
#include <fstream>
#include <sstream>

namespace SOMTParser::MiniZinc {

// ═══════════════════════════════════════════════════════════════════
// MznDznParser
// ═══════════════════════════════════════════════════════════════════

MznDznParser::MznDznParser() = default;

DznData MznDznParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw MznParseError(SourceLoc(), "Cannot open .dzn file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), filename);
}

DznData MznDznParser::parseString(const std::string& source,
                                  const std::string& filename) {
    this->current_filename = filename;
    lexer.init(source);
    has_current = false;
    advance();

    DznData data;
    while (!check(TokenType::END_OF_FILE)) {
        parseAssignment(data);
        if (check(TokenType::SEMICOLON)) {
            advance();
        } else if (!check(TokenType::END_OF_FILE)) {
            error("Expected ';' after assignment");
        }
    }
    return data;
}

// ── Token helpers ────────────────────────────────────────────────
void MznDznParser::advance() {
    current = lexer.nextToken();
    has_current = true;
}

bool MznDznParser::check(TokenType type) const {
    return current.type == type;
}

bool MznDznParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token MznDznParser::consume(TokenType type, const std::string& err_msg) {
    if (check(type)) {
        Token tok = current;
        advance();
        return tok;
    }
    errorAt(current, err_msg);
    return Token();
}

void MznDznParser::error(const std::string& msg) {
    errorAt(current, msg);
}

void MznDznParser::errorAt(const Token& tok, const std::string& msg) {
    throw MznParseError(tok.loc, msg + " (got '" + tok.text + "')");
}

// ── Assignment parsing ───────────────────────────────────────────
void MznDznParser::parseAssignment(DznData& data) {
    Token name = consume(TokenType::IDENT, "Expected identifier in .dzn assignment");
    if (data.has(name.text)) {
        errorAt(name, "Duplicate key in .dzn data: " + name.text);
    }
    consume(TokenType::OP_EQ, "Expected '=' in .dzn assignment");
    ExprPtr value = parseExpr();
    data.assignments[name.text] = value;
    data.ordered_keys.push_back(name.text);
}

// ── Expression parsing (dzn subset) ──────────────────────────────
ExprPtr MznDznParser::parseExpr() {
    return parsePrimary();
}

ExprPtr MznDznParser::parsePrimary() {
    switch (current.type) {
        case TokenType::INT_LIT: {
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, current.loc);
            expr->data = IntLit{std::stoll(current.text)};
            advance();
            // Check for range: 1..10 — in DZN this represents a set
            if (match(TokenType::OP_RANGE)) {
                ExprPtr end = parseExpr();
                auto set_expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, current.loc);
                SetLit set;
                set.elements.push_back(expr);
                set.elements.push_back(end);
                set_expr->data = set;
                return set_expr;
            }
            return expr;
        }
        case TokenType::FLOAT_LIT: {
            auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, current.loc);
            expr->data = FloatLit{std::stod(current.text)};
            advance();
            return expr;
        }
        case TokenType::BOOL_LIT: {
            auto expr = std::make_shared<Expr>(Expr::Kind::BOOL_LIT, current.loc);
            expr->data = BoolLit{current.text == "true"};
            advance();
            return expr;
        }
        case TokenType::STRING_LIT: {
            auto expr = std::make_shared<Expr>(Expr::Kind::STRING_LIT, current.loc);
            expr->data = StringLit{current.text};
            advance();
            return expr;
        }
        case TokenType::IDENT: {
            std::string name = current.text;
            advance();
            if (name == "array2d") {
                return parseArray2d();
            }
            // Simple identifier reference (should be another key)
            auto expr = std::make_shared<Expr>(Expr::Kind::IDENT, current.loc);
            expr->data = Ident{name};
            return expr;
        }
        case TokenType::LBRACKET:
            return parseArrayLiteral();
        case TokenType::LBRACE:
            return parseSetLiteral();
        case TokenType::OP_MINUS: {
            // Negative number literal
            advance();
            if (check(TokenType::INT_LIT)) {
                auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, current.loc);
                expr->data = IntLit{-std::stoll(current.text)};
                advance();
                return expr;
            }
            if (check(TokenType::FLOAT_LIT)) {
                auto expr = std::make_shared<Expr>(Expr::Kind::FLOAT_LIT, current.loc);
                expr->data = FloatLit{-std::stod(current.text)};
                advance();
                return expr;
            }
            error("Expected number after '-'");
            return nullptr;
        }
        default:
            error("Unexpected token in .dzn expression");
            return nullptr;
    }
}

ExprPtr MznDznParser::parseArrayLiteral() {
    SourceLoc loc = current.loc;
    advance(); // '['

    if (match(TokenType::PIPE)) {
        // 2D array literal: [| 1, 2 | 3, 4 |]
        std::vector<ExprPtr> all_elements;
        std::vector<size_t> row_lengths;
        do {
            std::vector<ExprPtr> row;
            if (!check(TokenType::PIPE) && !check(TokenType::RBRACKET)) {
                row.push_back(parseExpr());
                while (match(TokenType::COMMA)) {
                    row.push_back(parseExpr());
                }
            }
            row_lengths.push_back(row.size());
            all_elements.insert(all_elements.end(), row.begin(), row.end());
        } while (match(TokenType::PIPE));
        consume(TokenType::RBRACKET, "Expected ']' after 2D array literal");
        auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
        expr->data = ArrayLit{all_elements};
        return expr;
    }

    // Regular 1D array literal
    std::vector<ExprPtr> elems;
    if (!check(TokenType::RBRACKET)) {
        elems.push_back(parseExpr());
        while (match(TokenType::COMMA)) {
            elems.push_back(parseExpr());
        }
    }
    consume(TokenType::RBRACKET, "Expected ']' after array literal");
    auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
    expr->data = ArrayLit{elems};
    return expr;
}

ExprPtr MznDznParser::parseSetLiteral() {
    SourceLoc loc = current.loc;
    advance(); // '{'

    if (check(TokenType::RBRACE)) {
        advance();
        auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, loc);
        expr->data = SetLit{};
        return expr;
    }

    ExprPtr first = parseExpr();

    // Check for range: {1..10}
    if (match(TokenType::OP_RANGE)) {
        ExprPtr last = parseExpr();
        consume(TokenType::RBRACE, "Expected '}' after set range");
        // Represent range as a set literal with range expression
        SetLit set;
        set.elements.push_back(first);
        set.elements.push_back(last);
        auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, loc);
        expr->data = std::move(set);
        return expr;
    }

    // Regular set literal
    std::vector<ExprPtr> elems;
    elems.push_back(first);
    while (match(TokenType::COMMA)) {
        elems.push_back(parseExpr());
    }
    consume(TokenType::RBRACE, "Expected '}' after set literal");
    auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, loc);
    expr->data = SetLit{elems};
    return expr;
}

ExprPtr MznDznParser::parseArray2d() {
    SourceLoc loc = current.loc;
    consume(TokenType::LPAREN, "Expected '(' after array2d");

    // Parse dimensions (ranges like 1..2)
    ExprPtr dim1 = parseExpr();
    consume(TokenType::COMMA, "Expected ',' after first dimension");

    ExprPtr dim2 = parseExpr();
    consume(TokenType::COMMA, "Expected ',' after second dimension");

    // Parse flat data array
    ExprPtr data = parseExpr();
    consume(TokenType::RPAREN, "Expected ')' after array2d");

    // Return as array literal (flattened)
    auto* arr = data->as<ArrayLit>();
    if (arr) {
        auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
        expr->data = ArrayLit{arr->elements};
        return expr;
    }
    return data;
}

// ═══════════════════════════════════════════════════════════════════
// MznDznMerger
// ═══════════════════════════════════════════════════════════════════

Model MznDznMerger::merge(const Model& model, const DznData& data) {
    Model result = model; // shallow copy of items vector
    // Deep copy items so we can mutate in place
    for (size_t i = 0; i < result.items.size(); ++i) {
        if (auto* vd = dynamic_cast<VarDeclItem*>(result.items[i].get())) {
            if (data.has(vd->name)) {
                auto val = data.get(vd->name);
                std::string err;
                if (!typeCheckMerge(*vd->type, val, err)) {
                    throw MznTypeError(val->loc, "Type mismatch for " + vd->name + ": " + err);
                }
                vd->init = val;
                vd->type->par_var = TypeInst::ParVar::PAR; // mark as fixed
            }
        }
    }
    return result;
}

bool MznDznMerger::typeCheckMerge(const TypeInst& expected,
                                  const ExprPtr& value,
                                  std::string& out_error) const {
    // Simplified type checking for .dzn merge
    switch (expected.base) {
        case TypeInst::BaseKind::INT: {
            auto* lit = value->as<IntLit>();
            if (!lit) { out_error = "Expected int"; return false; }
            return true;
        }
        case TypeInst::BaseKind::BOOL: {
            auto* lit = value->as<BoolLit>();
            if (!lit) { out_error = "Expected bool"; return false; }
            return true;
        }
        case TypeInst::BaseKind::FLOAT: {
            auto* lit = value->as<FloatLit>();
            if (!lit) { out_error = "Expected float"; return false; }
            return true;
        }
        case TypeInst::BaseKind::STRING: {
            auto* lit = value->as<StringLit>();
            if (!lit) { out_error = "Expected string"; return false; }
            return true;
        }
        case TypeInst::BaseKind::UNKNOWN: {
            // Array type
            if (!expected.array_dims.empty()) {
                auto* arr = value->as<ArrayLit>();
                if (!arr) { out_error = "Expected array"; return false; }
                return true;
            }
            out_error = "Unknown expected type";
            return false;
        }
        default:
            out_error = "Unsupported type for .dzn merge";
            return false;
    }
}

} // namespace SOMTParser::MiniZinc
