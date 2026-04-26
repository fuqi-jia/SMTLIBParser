/* -*- C++ -*-
 *
 * MiniZinc Frontend — Parser Implementation
 */

#include "somtparser/minizinc/mzn_parser.h"

#include <fstream>
#include <sstream>

namespace SOMTParser::MiniZinc {

// ── Constructor / Entry points ─────────────────────────────────────
MznParser::MznParser() = default;

Model MznParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw MznParseError(SourceLoc(), "Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return parseString(buffer.str(), filename);
}

Model MznParser::parseString(const std::string& source, const std::string& filename) {
    this->current_filename = filename;
    lexer.init(source);
    has_current = false;
    advance();

    Model model;
    model.filename = filename;

    while (!check(TokenType::END_OF_FILE)) {
        auto item = parseItem();
        if (item) {
            model.addItem(item);
            // Register top-level names
            if (auto* vd = dynamic_cast<VarDeclItem*>(item.get())) {
                model.top_level_map[vd->name] = item;
            } else if (auto* ed = dynamic_cast<EnumDeclItem*>(item.get())) {
                model.top_level_map[ed->name] = item;
            } else if (auto* pd = dynamic_cast<PredicateItem*>(item.get())) {
                model.top_level_map[pd->name] = item;
            } else if (auto* fd = dynamic_cast<FunctionItem*>(item.get())) {
                model.top_level_map[fd->name] = item;
            }
        }
        if (check(TokenType::SEMICOLON)) {
            advance();
        } else if (!check(TokenType::END_OF_FILE)) {
            error("Expected ';' after item");
        }
    }

    return model;
}

// ── Token helpers ──────────────────────────────────────────────────
void MznParser::advance() {
    current = lexer.nextToken();
    has_current = true;
}

bool MznParser::check(TokenType type) const {
    return current.type == type;
}

bool MznParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token MznParser::consume(TokenType type, const std::string& err_msg) {
    if (check(type)) {
        Token tok = current;
        advance();
        return tok;
    }
    error(err_msg);
    return Token(); // unreachable
}

void MznParser::errorStatic(const std::string& msg) {
    throw MznParseError(SourceLoc(), msg);
}

void MznParser::error(const std::string& msg) {
    errorAt(current, msg);
}

void MznParser::errorAt(const Token& tok, const std::string& msg) {
    throw MznParseError(tok.loc, msg + " (got '" + tok.text + "')");
}

// ── Top-level item parsing ─────────────────────────────────────────
ItemPtr MznParser::parseItem() {
    switch (current.type) {
        case TokenType::KW_INCLUDE:
            return parseInclude();
        case TokenType::KW_VAR:
        case TokenType::KW_PAR:
        case TokenType::KW_OPT:
        case TokenType::KW_BOOL:
        case TokenType::KW_INT:
        case TokenType::KW_FLOAT:
        case TokenType::KW_STRING:
        case TokenType::KW_ANN:
        case TokenType::KW_ARRAY:
        case TokenType::KW_LIST:
        case TokenType::KW_SET:
        case TokenType::KW_TUPLE:
        case TokenType::KW_RECORD:
        case TokenType::KW_ANY:
        case TokenType::IDENT:
        case TokenType::QUOTED_IDENT:
            return parseAssignOrVarDecl();
        case TokenType::KW_ENUM:
            return parseEnumDecl();
        case TokenType::KW_CONSTRAINT:
            return parseConstraint();
        case TokenType::KW_SOLVE:
            return parseSolve();
        case TokenType::KW_OUTPUT:
            return parseOutput();
        case TokenType::KW_PREDICATE:
            return parsePredicate();
        case TokenType::KW_FUNCTION:
            return parseFunction();
        case TokenType::KW_TEST:
            return parseTest();
        case TokenType::KW_ANNOTATION:
            return parseAnnotationDecl();
        default:
            error("Unexpected token at start of item");
            return nullptr;
    }
}

ItemPtr MznParser::parseInclude() {
    SourceLoc loc = current.loc;
    advance(); // consume 'include'
    Token fname = consume(TokenType::STRING_LIT, "Expected string literal after 'include'");
    return std::make_shared<IncludeItem>(loc, fname.text);
}

ItemPtr MznParser::parseVarDecl() {
    SourceLoc loc = current.loc;
    auto type = parseTypeInst();
    consume(TokenType::COLON, "Expected ':' after type-inst in variable declaration");
    Token name;
    if (check(TokenType::IDENT)) {
        name = consume(TokenType::IDENT, "Expected identifier after ':' in variable declaration");
    } else if (check(TokenType::QUOTED_IDENT)) {
        name = consume(TokenType::QUOTED_IDENT, "Expected identifier after ':' in variable declaration");
    } else {
        error("Expected identifier after ':' in variable declaration");
    }

    ExprPtr init = nullptr;
    if (match(TokenType::OP_EQ)) {
        init = parseExpr();
    }

    // Parse annotations
    std::vector<ExprPtr> anns;
    while (match(TokenType::ANNOTATION_START)) {
        anns.push_back(parseExpr());
    }

    auto item = std::make_shared<VarDeclItem>(loc, type, name.text, init);
    item->anns = std::move(anns);
    return item;
}

ItemPtr MznParser::parseAssignOrVarDecl() {
    // Lookahead: if we see "ident = expr;" it's an assignment.
    // Otherwise it's a variable declaration.
    // Simplification: try to parse as var-decl first; if that fails
    // and the pattern is "ident =", parse as assignment.
    if (check(TokenType::IDENT) || check(TokenType::QUOTED_IDENT)) {
        // Save state for potential backtrack (not implemented; simplified approach)
        // For now, assignments in .mzn are rare outside .dzn; we handle them
        // when the top-level item starts with an identifier followed by '='.
        // Actually in MiniZinc, assignments at top level are only valid in .dzn
        // or for par declarations with init. We treat "x = 5;" as assignment
        // if x was previously declared.
        // Simplified: always try var-decl first.
    }
    return parseVarDecl();
}

ItemPtr MznParser::parseConstraint() {
    SourceLoc loc = current.loc;
    advance(); // 'constraint'
    ExprPtr expr = parseExpr();

    std::vector<ExprPtr> anns;
    while (match(TokenType::ANNOTATION_START)) {
        anns.push_back(parseExpr());
    }

    auto item = std::make_shared<ConstraintItem>(loc, expr);
    item->anns = std::move(anns);
    return item;
}

ItemPtr MznParser::parseSolve() {
    SourceLoc loc = current.loc;
    advance(); // 'solve'

    // Annotations
    std::vector<ExprPtr> anns;
    while (match(TokenType::ANNOTATION_START)) {
        anns.push_back(parseExpr());
    }

    SolveItem::Mode mode = SolveItem::Mode::SATISFY;
    ExprPtr objective = nullptr;

    if (match(TokenType::KW_SATISFY)) {
        mode = SolveItem::Mode::SATISFY;
    } else if (match(TokenType::KW_MINIMIZE)) {
        mode = SolveItem::Mode::MINIMIZE;
        objective = parseExpr();
    } else if (match(TokenType::KW_MAXIMIZE)) {
        mode = SolveItem::Mode::MAXIMIZE;
        objective = parseExpr();
    } else {
        error("Expected 'satisfy', 'minimize', or 'maximize' after 'solve'");
    }

    auto item = std::make_shared<SolveItem>(loc, mode, objective);
    item->anns = std::move(anns);
    return item;
}

ItemPtr MznParser::parseOutput() {
    SourceLoc loc = current.loc;
    advance(); // 'output'
    ExprPtr expr = parseExpr();
    return std::make_shared<OutputItem>(loc, expr);
}

ItemPtr MznParser::parsePredicate() {
    SourceLoc loc = current.loc;
    advance(); // 'predicate'
    Token name = consume(TokenType::IDENT, "Expected identifier after 'predicate'");
    auto item = std::make_shared<PredicateItem>(loc, name.text);

    consume(TokenType::LPAREN, "Expected '(' after predicate name");
    if (!check(TokenType::RPAREN)) {
        do {
            auto param_type = parseTypeInst();
            consume(TokenType::COLON, "Expected ':' after parameter type");
            Token param_name = consume(TokenType::IDENT, "Expected parameter name");
            item->params.push_back(std::make_shared<VarDeclItem>(param_name.loc, param_type, param_name.text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after predicate parameters");

    if (match(TokenType::OP_EQ)) {
        item->body = parseExpr();
    }
    return item;
}

ItemPtr MznParser::parseFunction() {
    SourceLoc loc = current.loc;
    advance(); // 'function'
    auto ret_type = parseTypeInst();
    consume(TokenType::COLON, "Expected ':' after return type in function declaration");
    Token name = consume(TokenType::IDENT, "Expected identifier after ':' in function declaration");
    auto item = std::make_shared<FunctionItem>(loc, ret_type, name.text);

    consume(TokenType::LPAREN, "Expected '(' after function name");
    if (!check(TokenType::RPAREN)) {
        do {
            auto param_type = parseTypeInst();
            consume(TokenType::COLON, "Expected ':' after parameter type");
            Token param_name = consume(TokenType::IDENT, "Expected parameter name");
            item->params.push_back(std::make_shared<VarDeclItem>(param_name.loc, param_type, param_name.text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after function parameters");

    consume(TokenType::OP_EQ, "Expected '=' in function declaration");
    item->body = parseExpr();
    return item;
}

ItemPtr MznParser::parseTest() {
    SourceLoc loc = current.loc;
    advance(); // 'test'
    Token name = consume(TokenType::IDENT, "Expected identifier after 'test'");
    auto item = std::make_shared<TestItem>(loc, name.text);

    consume(TokenType::LPAREN, "Expected '(' after test name");
    if (!check(TokenType::RPAREN)) {
        do {
            auto param_type = parseTypeInst();
            consume(TokenType::COLON, "Expected ':' after parameter type");
            Token param_name = consume(TokenType::IDENT, "Expected parameter name");
            item->params.push_back(std::make_shared<VarDeclItem>(param_name.loc, param_type, param_name.text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after test parameters");

    consume(TokenType::OP_EQ, "Expected '=' in test declaration");
    item->body = parseExpr();
    return item;
}

ItemPtr MznParser::parseAnnotationDecl() {
    SourceLoc loc = current.loc;
    advance(); // 'annotation'
    Token name = consume(TokenType::IDENT, "Expected identifier after 'annotation'");
    auto item = std::make_shared<AnnotationItem>(loc, name.text);

    consume(TokenType::LPAREN, "Expected '(' after annotation name");
    if (!check(TokenType::RPAREN)) {
        do {
            auto param_type = parseTypeInst();
            consume(TokenType::COLON, "Expected ':' after parameter type");
            Token param_name = consume(TokenType::IDENT, "Expected parameter name");
            item->params.push_back(std::make_shared<VarDeclItem>(param_name.loc, param_type, param_name.text));
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expected ')' after annotation parameters");
    return item;
}

ItemPtr MznParser::parseEnumDecl() {
    SourceLoc loc = current.loc;
    advance(); // 'enum'
    Token name = consume(TokenType::IDENT, "Expected identifier after 'enum'");
    auto item = std::make_shared<EnumDeclItem>(loc, name.text);

    if (match(TokenType::OP_EQ)) {
        consume(TokenType::LBRACE, "Expected '{' after '=' in enum declaration");
        if (!check(TokenType::RBRACE)) {
            do {
                Token ctor = consume(TokenType::IDENT, "Expected enum constructor name");
                item->constructors.push_back(ctor.text);
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RBRACE, "Expected '}' after enum constructors");
    }
    return item;
}

// ── Type parsing ───────────────────────────────────────────────────
std::shared_ptr<TypeInst> MznParser::parseTypeInst() {
    auto type = std::make_shared<TypeInst>();
    type->loc = current.loc;

    // var / par
    if (match(TokenType::KW_VAR)) {
        type->par_var = TypeInst::ParVar::VAR;
    } else if (match(TokenType::KW_PAR)) {
        type->par_var = TypeInst::ParVar::PAR;
    }

    // opt
    if (match(TokenType::KW_OPT)) {
        type->is_opt = true;
    }

    // set of
    if (match(TokenType::KW_SET)) {
        consume(TokenType::KW_OF, "Expected 'of' after 'set'");
        type->is_set = true;
    }

    // array[...] of ...  or  list of ...
    if (check(TokenType::KW_ARRAY) || check(TokenType::KW_LIST)) {
        return parseArrayType(std::move(type));
    }

    // tuple(...)
    if (match(TokenType::KW_TUPLE)) {
        type->base = TypeInst::BaseKind::TUPLE;
        consume(TokenType::LPAREN, "Expected '(' after 'tuple'");
        do {
            type->tuple_elems.push_back(parseTypeInst());
        } while (match(TokenType::COMMA));
        consume(TokenType::RPAREN, "Expected ')' after tuple types");
        return type;
    }

    // record(...)
    if (match(TokenType::KW_RECORD)) {
        type->base = TypeInst::BaseKind::RECORD;
        consume(TokenType::LPAREN, "Expected '(' after 'record'");
        do {
            auto field_type = parseTypeInst();
            consume(TokenType::COLON, "Expected ':' after record field type");
            Token field_name = consume(TokenType::IDENT, "Expected field name");
            type->record_fields.emplace_back(field_name.text, field_type);
        } while (match(TokenType::COMMA));
        consume(TokenType::RPAREN, "Expected ')' after record fields");
        return type;
    }

    // Parse base type
    parseBaseTypeInto(*type);
    return type;
}

void MznParser::parseBaseTypeInto(TypeInst& type) {
    switch (current.type) {
        case TokenType::KW_BOOL:
            type.base = TypeInst::BaseKind::BOOL;
            advance();
            break;
        case TokenType::KW_INT:
            type.base = TypeInst::BaseKind::INT;
            advance();
            // Optional domain expression: var 1..10: x;  var {1,3,5}: y;
            if (check(TokenType::INT_LIT) || check(TokenType::LBRACE)) {
                type.domain_expr = parseExpr();
            }
            break;
        case TokenType::KW_FLOAT:
            type.base = TypeInst::BaseKind::FLOAT;
            advance();
            break;
        case TokenType::KW_STRING:
            type.base = TypeInst::BaseKind::STRING;
            advance();
            break;
        case TokenType::KW_ANN:
            type.base = TypeInst::BaseKind::ANN;
            advance();
            break;
        case TokenType::KW_ANY:
            type.base = TypeInst::BaseKind::ANY;
            advance();
            break;
        case TokenType::KW_ENUM:
            advance();
            if (check(TokenType::IDENT)) {
                type.base = TypeInst::BaseKind::ENUM;
                type.name = current.text;
                advance();
            } else {
                error("Expected enum name after 'enum' in type");
            }
            break;
        case TokenType::IDENT:
        case TokenType::QUOTED_IDENT:
            type.base = TypeInst::BaseKind::ALIAS;
            type.name = current.text;
            advance();
            break;
        case TokenType::INT_LIT:
        case TokenType::LBRACE:
            // Domain expression without explicit base type: var 1..10: x; var {1,3,5}: y;
            type.base = TypeInst::BaseKind::INT;  // default to int
            type.domain_expr = parseExpr();
            break;
        default:
            error("Expected base type");
    }
}

std::shared_ptr<TypeInst> MznParser::parseArrayType(std::shared_ptr<TypeInst> type) {
    type->base = TypeInst::BaseKind::UNKNOWN; // arrays have special handling

    if (match(TokenType::KW_ARRAY)) {
        consume(TokenType::LBRACKET, "Expected '[' after 'array'");
        do {
            type->array_dims.push_back(parseExpr());
        } while (match(TokenType::COMMA));
        consume(TokenType::RBRACKET, "Expected ']' after array dimensions");
        consume(TokenType::KW_OF, "Expected 'of' after array dimensions");
    } else if (match(TokenType::KW_LIST)) {
        consume(TokenType::KW_OF, "Expected 'of' after 'list'");
    }

    type->elem_type = parseTypeInst();
    return type;
}

// ── Expression precedence table ────────────────────────────────────

int MznParser::getPrefixPrecedence(TokenType type) {
    switch (type) {
        case TokenType::OP_NOT:
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
            return 110;
        default:
            return -1;
    }
}

int MznParser::getInfixPrecedence(TokenType type) {
    switch (type) {
        case TokenType::OP_IFF:
            return 10;
        case TokenType::OP_IMPLIES:
        case TokenType::OP_IMPLIED_BY:
            return 20;
        case TokenType::OP_OR:
            return 30;
        case TokenType::OP_XOR:
            return 40;
        case TokenType::OP_AND:
            return 50;
        case TokenType::OP_EQ:
        case TokenType::OP_NEQ:
            return 60;
        case TokenType::OP_LT:
        case TokenType::OP_LE:
        case TokenType::OP_GT:
        case TokenType::OP_GE:
            return 70;
        case TokenType::OP_IN:
        case TokenType::KW_IN:
        case TokenType::OP_SUBSET:
        case TokenType::OP_SUPERSET:
            return 80;
        case TokenType::OP_UNION:
        case TokenType::OP_DIFF:
        case TokenType::OP_SYMDIFF:
            return 90;
        case TokenType::OP_INTERSECT:
            return 100;
        case TokenType::OP_RANGE:
        case TokenType::OP_RANGE_HALF_OPEN_L:
        case TokenType::OP_RANGE_HALF_OPEN_R:
        case TokenType::OP_RANGE_OPEN:
            return 105;
        case TokenType::OP_CONCAT:
            return 115;
        case TokenType::OP_PLUS:
        case TokenType::OP_MINUS:
            return 120;
        case TokenType::OP_MUL:
        case TokenType::OP_DIV:
        case TokenType::OP_DIV_INT:
        case TokenType::OP_MOD:
            return 130;
        case TokenType::OP_POW:
            return 140;
        default:
            return -1;
    }
}

bool MznParser::isRightAssociative(TokenType type) {
    return type == TokenType::OP_POW || type == TokenType::OP_IMPLIES;
}

BinaryOp::Op MznParser::tokenToBinaryOp(TokenType type) {
    switch (type) {
        case TokenType::OP_AND: return BinaryOp::Op::AND;
        case TokenType::OP_OR: return BinaryOp::Op::OR;
        case TokenType::OP_IMPLIES: return BinaryOp::Op::IMPLIES;
        case TokenType::OP_IMPLIED_BY: return BinaryOp::Op::IMPLIED_BY;
        case TokenType::OP_IFF: return BinaryOp::Op::IFF;
        case TokenType::OP_XOR: return BinaryOp::Op::XOR;
        case TokenType::OP_EQ: return BinaryOp::Op::EQ;
        case TokenType::OP_NEQ: return BinaryOp::Op::NEQ;
        case TokenType::OP_LT: return BinaryOp::Op::LT;
        case TokenType::OP_LE: return BinaryOp::Op::LE;
        case TokenType::OP_GT: return BinaryOp::Op::GT;
        case TokenType::OP_GE: return BinaryOp::Op::GE;
        case TokenType::OP_IN:
        case TokenType::KW_IN:
            return BinaryOp::Op::IN;
        case TokenType::OP_SUBSET: return BinaryOp::Op::SUBSET;
        case TokenType::OP_SUPERSET: return BinaryOp::Op::SUPERSET;
        case TokenType::OP_UNION: return BinaryOp::Op::UNION;
        case TokenType::OP_DIFF: return BinaryOp::Op::DIFF;
        case TokenType::OP_SYMDIFF: return BinaryOp::Op::SYMDIFF;
        case TokenType::OP_INTERSECT: return BinaryOp::Op::INTERSECT;
        case TokenType::OP_RANGE: return BinaryOp::Op::RANGE;
        case TokenType::OP_RANGE_HALF_OPEN_L: return BinaryOp::Op::RANGE_HALF_OPEN_L;
        case TokenType::OP_RANGE_HALF_OPEN_R: return BinaryOp::Op::RANGE_HALF_OPEN_R;
        case TokenType::OP_RANGE_OPEN: return BinaryOp::Op::RANGE_OPEN;
        case TokenType::OP_CONCAT: return BinaryOp::Op::CONCAT;
        case TokenType::OP_PLUS: return BinaryOp::Op::ADD;
        case TokenType::OP_MINUS: return BinaryOp::Op::SUB;
        case TokenType::OP_MUL: return BinaryOp::Op::MUL;
        case TokenType::OP_DIV: return BinaryOp::Op::DIV;
        case TokenType::OP_DIV_INT: return BinaryOp::Op::DIV_INT;
        case TokenType::OP_MOD: return BinaryOp::Op::MOD;
        case TokenType::OP_POW: return BinaryOp::Op::POW;
        default:
            errorStatic("Token is not a binary operator");
            return BinaryOp::Op::EQ; // unreachable
    }
}

UnaryOp::Op MznParser::tokenToUnaryOp(TokenType type) {
    switch (type) {
        case TokenType::OP_NOT: return UnaryOp::Op::NOT;
        case TokenType::OP_PLUS: return UnaryOp::Op::PLUS;
        case TokenType::OP_MINUS: return UnaryOp::Op::MINUS;
        default:
            errorStatic("Token is not a unary operator");
            return UnaryOp::Op::NOT; // unreachable
    }
}

// ── Expression parsing (Pratt parser) ──────────────────────────────
ExprPtr MznParser::parseExpr(int min_prec) {
    // Parse prefix / primary
    ExprPtr left;
    int prefix_prec = getPrefixPrecedence(current.type);
    if (prefix_prec > 0) {
        SourceLoc loc = current.loc;
        TokenType op_type = current.type;
        advance();
        ExprPtr operand = parseExpr(prefix_prec);
        auto expr = std::make_shared<Expr>(Expr::Kind::UNARY_OP, loc);
        expr->data = UnaryOp{tokenToUnaryOp(op_type), operand};
        left = expr;
    } else {
        left = parsePrimary();
    }

    // Infix, postfix, and annotations in a single loop
    while (true) {
        // Postfix: array access
        if (match(TokenType::LBRACKET)) {
            SourceLoc loc = current.loc;
            std::vector<ExprPtr> indices;
            do {
                indices.push_back(parseExpr());
            } while (match(TokenType::COMMA));
            consume(TokenType::RBRACKET, "Expected ']' after array index");
            auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_ACCESS, loc);
            expr->data = ArrayAccess{left, indices};
            left = expr;
            continue;
        }

        // Postfix: field / tuple access
        if (match(TokenType::DOT)) {
            SourceLoc loc = current.loc;
            if (check(TokenType::INT_LIT)) {
                size_t idx = std::stoull(current.text);
                advance();
                auto expr = std::make_shared<Expr>(Expr::Kind::TUPLE_ACCESS, loc);
                expr->data = TupleAccess{left, idx};
                left = expr;
            } else if (check(TokenType::IDENT)) {
                std::string field = current.text;
                advance();
                auto expr = std::make_shared<Expr>(Expr::Kind::FIELD_ACCESS, loc);
                expr->data = FieldAccess{left, field};
                left = expr;
            } else {
                error("Expected integer or identifier after '.'");
            }
            continue;
        }

        // Postfix: annotations
        if (check(TokenType::ANNOTATION_START)) {
            std::vector<ExprPtr> anns;
            SourceLoc ann_loc = current.loc;
            while (match(TokenType::ANNOTATION_START)) {
                anns.push_back(parseExpr());
            }
            if (!anns.empty()) {
                auto expr = std::make_shared<Expr>(Expr::Kind::ANNOTATED, ann_loc);
                expr->data = Annotated{left, anns};
                left = expr;
            }
            continue;
        }

        // Infix operators
        int infix_prec = getInfixPrecedence(current.type);
        if (infix_prec < min_prec) {
            break;
        }

        TokenType op_type = current.type;
        SourceLoc loc = current.loc;
        advance();

        int next_min_prec = infix_prec + (isRightAssociative(op_type) ? 0 : 1);
        ExprPtr right = parseExpr(next_min_prec);

        auto expr = std::make_shared<Expr>(Expr::Kind::BINARY_OP, loc);
        expr->data = BinaryOp{tokenToBinaryOp(op_type), left, right};
        left = expr;
    }

    return left;
}

ExprPtr MznParser::parsePrimary() {
    switch (current.type) {
        case TokenType::INT_LIT: {
            auto expr = std::make_shared<Expr>(Expr::Kind::INT_LIT, current.loc);
            expr->data = IntLit{std::stoll(current.text)};
            advance();
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
        case TokenType::IDENT:
        case TokenType::QUOTED_IDENT:
            return parseCallOrIdent();
        case TokenType::ANON_VAR: {
            auto expr = std::make_shared<Expr>(Expr::Kind::ANON_VAR, current.loc);
            expr->data = AnonVar{};
            advance();
            return expr;
        }
        case TokenType::LBRACKET:
            return parseArrayLiteral();
        case TokenType::LBRACE:
            return parseSetLiteral();
        case TokenType::LPAREN:
            return parseTupleLiteral();
        case TokenType::KW_IF:
            return parseIfThenElse();
        case TokenType::KW_LET:
            return parseLet();
        default:
            error("Unexpected token in expression");
            return nullptr;
    }
}

ExprPtr MznParser::parseCallOrIdent() {
    SourceLoc loc = current.loc;
    std::string name = current.text;
    advance();

    // Function / predicate call
    if (match(TokenType::LPAREN)) {
        std::vector<ExprPtr> args;
        std::vector<Generator> gens;
        bool is_comp = false;

        if (!check(TokenType::RPAREN)) {
            // Check for comprehension-style call: sum(i in 1..10)(expr)
            // This is tricky because (i in 1..10) looks like a parenthesized expr.
            // We parse arguments; if we see generators followed by ')', then it's comprehension.
            // Simplified: parse all arguments as expressions first.
            do {
                args.push_back(parseExpr());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RPAREN, "Expected ')' after call arguments");

        // Check for second set of parens (comprehension body)
        if (match(TokenType::LPAREN)) {
            is_comp = true;
            ExprPtr body = parseExpr();
            consume(TokenType::RPAREN, "Expected ')' after comprehension body");
            args.push_back(body);
        }

        auto expr = std::make_shared<Expr>(Expr::Kind::CALL, loc);
        expr->data = CallExpr{name, args, gens, is_comp};
        return expr;
    }

    // Just an identifier
    auto expr = std::make_shared<Expr>(Expr::Kind::IDENT, loc);
    expr->data = Ident{name};
    return expr;
}

ExprPtr MznParser::parseArrayLiteral() {
    SourceLoc loc = current.loc;
    advance(); // '['

    // Check for empty array: []
    if (check(TokenType::RBRACKET)) {
        advance();
        auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
        expr->data = ArrayLit{};
        return expr;
    }

    // Check for 2D array literal: [| expr, expr | expr, expr |]
    if (match(TokenType::PIPE)) {
        // Empty 2D array: [|]
        if (match(TokenType::RBRACKET)) {
            auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
            expr->data = ArrayLit{};
            return expr;
        }
        std::vector<ExprPtr> elems;
        while (true) {
            do {
                elems.push_back(parseExpr());
            } while (match(TokenType::COMMA));
            if (match(TokenType::PIPE)) {
                if (match(TokenType::RBRACKET)) {
                    break;
                }
                continue;
            }
            error("Expected '|' in 2D array literal");
        }
        auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
        expr->data = ArrayLit{elems};
        return expr;
    }

    ExprPtr first = parseExpr();

    if (match(TokenType::PIPE)) {
        // Array comprehension
        auto gens = parseGenerators();
        ExprPtr where = nullptr;
        if (match(TokenType::KW_WHERE)) {
            where = parseExpr();
        }
        consume(TokenType::RBRACKET, "Expected ']' after array comprehension");
        auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_COMP, loc);
        expr->data = ArrayComp{first, gens, where};
        return expr;
    }

    // Regular array literal
    std::vector<ExprPtr> elems;
    elems.push_back(first);
    while (match(TokenType::COMMA)) {
        elems.push_back(parseExpr());
    }
    consume(TokenType::RBRACKET, "Expected ']' after array literal");
    auto expr = std::make_shared<Expr>(Expr::Kind::ARRAY_LIT, loc);
    expr->data = ArrayLit{elems};
    return expr;
}

ExprPtr MznParser::parseSetLiteral() {
    SourceLoc loc = current.loc;
    advance(); // '{'

    if (check(TokenType::RBRACE)) {
        advance();
        auto expr = std::make_shared<Expr>(Expr::Kind::SET_LIT, loc);
        expr->data = SetLit{};
        return expr;
    }

    ExprPtr first = parseExpr();

    if (match(TokenType::PIPE)) {
        // Set comprehension
        auto gens = parseGenerators();
        ExprPtr where = nullptr;
        if (match(TokenType::KW_WHERE)) {
            where = parseExpr();
        }
        consume(TokenType::RBRACE, "Expected '}' after set comprehension");
        auto expr = std::make_shared<Expr>(Expr::Kind::SET_COMP, loc);
        expr->data = SetComp{first, gens, where};
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

ExprPtr MznParser::parseTupleLiteral() {
    SourceLoc loc = current.loc;
    advance(); // '('

    // Could be a parenthesized expression or a tuple/record literal
    ExprPtr first = parseExpr();

    if (match(TokenType::COMMA)) {
        // Tuple literal
        std::vector<ExprPtr> elems;
        elems.push_back(first);
        do {
            elems.push_back(parseExpr());
        } while (match(TokenType::COMMA));
        consume(TokenType::RPAREN, "Expected ')' after tuple literal");
        auto expr = std::make_shared<Expr>(Expr::Kind::TUPLE_LIT, loc);
        expr->data = TupleLit{elems};
        return expr;
    }

    if (match(TokenType::RPAREN)) {
        // Parenthesized expression
        return first;
    }

    // Record literal: (field: expr, ...)
    if (match(TokenType::COLON)) {
        // first was actually a field name
        std::vector<std::pair<std::string, ExprPtr>> fields;
        std::string field_name;
        if (auto* id = first->as<Ident>()) {
            field_name = id->name;
        } else {
            error("Expected field name before ':' in record literal");
        }
        ExprPtr field_val = parseExpr();
        fields.emplace_back(field_name, field_val);

        while (match(TokenType::COMMA)) {
            Token name_tok = consume(TokenType::IDENT, "Expected field name");
            consume(TokenType::COLON, "Expected ':' after field name");
            ExprPtr val = parseExpr();
            fields.emplace_back(name_tok.text, val);
        }
        consume(TokenType::RPAREN, "Expected ')' after record literal");
        auto expr = std::make_shared<Expr>(Expr::Kind::RECORD_LIT, loc);
        expr->data = RecordLit{fields};
        return expr;
    }

    error("Expected ')', ',', or ':' after '('");
    return nullptr;
}

ExprPtr MznParser::parseRecordLiteral() {
    // Record literals are parsed inside parseTupleLiteral when we see (field: value)
    // This function should not be called directly.
    error("Internal error: parseRecordLiteral called directly");
    return nullptr;
}

ExprPtr MznParser::parseIfThenElse() {
    SourceLoc loc = current.loc;
    advance(); // 'if'

    std::vector<std::pair<ExprPtr, ExprPtr>> branches;

    ExprPtr cond = parseExpr();
    consume(TokenType::KW_THEN, "Expected 'then' after 'if' condition");
    ExprPtr then_expr = parseExpr();
    branches.emplace_back(cond, then_expr);

    while (match(TokenType::KW_ELSEIF)) {
        cond = parseExpr();
        consume(TokenType::KW_THEN, "Expected 'then' after 'elseif' condition");
        then_expr = parseExpr();
        branches.emplace_back(cond, then_expr);
    }

    consume(TokenType::KW_ELSE, "Expected 'else' in if-then-else expression");
    ExprPtr else_expr = parseExpr();
    consume(TokenType::KW_ENDIF, "Expected 'endif' to close if-then-else expression");

    auto expr = std::make_shared<Expr>(Expr::Kind::IF_THEN_ELSE, loc);
    expr->data = IfThenElse{branches, else_expr};
    return expr;
}

ExprPtr MznParser::parseLet() {
    SourceLoc loc = current.loc;
    advance(); // 'let'
    consume(TokenType::LBRACE, "Expected '{' after 'let'");

    std::vector<ItemPtr> items;
    while (!check(TokenType::RBRACE) && !check(TokenType::END_OF_FILE)) {
        items.push_back(parseItem());
        if (check(TokenType::SEMICOLON)) {
            advance();
        }
    }
    consume(TokenType::RBRACE, "Expected '}' after let items");
    consume(TokenType::KW_IN, "Expected 'in' after let block");
    ExprPtr body = parseExpr();

    auto expr = std::make_shared<Expr>(Expr::Kind::LET, loc);
    expr->data = LetExpr{items, body};
    return expr;
}

std::vector<Generator> MznParser::parseGenerators() {
    std::vector<Generator> gens;
    do {
        std::vector<std::string> vars;
        do {
            Token var = consume(TokenType::IDENT, "Expected generator variable");
            vars.push_back(var.text);
        } while (match(TokenType::COMMA));
        consume(TokenType::KW_IN, "Expected 'in' after generator variables");
        ExprPtr set_expr = parseExpr();
        gens.push_back(Generator{vars, set_expr});
    } while (match(TokenType::COMMA));
    return gens;
}

} // namespace SOMTParser::MiniZinc
