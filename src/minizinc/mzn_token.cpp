/* -*- C++ -*-
 *
 * MiniZinc Frontend — Token Utilities
 */

#include "somtparser/minizinc/mzn_token.h"

namespace SOMTParser::MiniZinc {

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "EOF";
        case TokenType::ERROR: return "ERROR";
        case TokenType::IDENT: return "IDENT";
        case TokenType::QUOTED_IDENT: return "QUOTED_IDENT";
        case TokenType::INT_LIT: return "INT_LIT";
        case TokenType::FLOAT_LIT: return "FLOAT_LIT";
        case TokenType::BOOL_LIT: return "BOOL_LIT";
        case TokenType::STRING_LIT: return "STRING_LIT";
        case TokenType::KW_VAR: return "var";
        case TokenType::KW_PAR: return "par";
        case TokenType::KW_OPT: return "opt";
        case TokenType::KW_SET: return "set";
        case TokenType::KW_OF: return "of";
        case TokenType::KW_ARRAY: return "array";
        case TokenType::KW_LIST: return "list";
        case TokenType::KW_TUPLE: return "tuple";
        case TokenType::KW_RECORD: return "record";
        case TokenType::KW_ENUM: return "enum";
        case TokenType::KW_BOOL: return "bool";
        case TokenType::KW_INT: return "int";
        case TokenType::KW_FLOAT: return "float";
        case TokenType::KW_STRING: return "string";
        case TokenType::KW_ANN: return "ann";
        case TokenType::KW_ANY: return "any";
        case TokenType::KW_CONSTRAINT: return "constraint";
        case TokenType::KW_SOLVE: return "solve";
        case TokenType::KW_SATISFY: return "satisfy";
        case TokenType::KW_MINIMIZE: return "minimize";
        case TokenType::KW_MAXIMIZE: return "maximize";
        case TokenType::KW_OUTPUT: return "output";
        case TokenType::KW_PREDICATE: return "predicate";
        case TokenType::KW_FUNCTION: return "function";
        case TokenType::KW_TEST: return "test";
        case TokenType::KW_INCLUDE: return "include";
        case TokenType::KW_IF: return "if";
        case TokenType::KW_THEN: return "then";
        case TokenType::KW_ELSE: return "else";
        case TokenType::KW_ELSEIF: return "elseif";
        case TokenType::KW_ENDIF: return "endif";
        case TokenType::KW_LET: return "let";
        case TokenType::KW_IN: return "in";
        case TokenType::KW_WHERE: return "where";
        case TokenType::KW_ANNOTATION: return "annotation";
        case TokenType::OP_NOT: return "not";
        case TokenType::OP_AND: return "/\\";
        case TokenType::OP_OR: return "\\/";
        case TokenType::OP_IMPLIES: return "->";
        case TokenType::OP_IMPLIED_BY: return "<-";
        case TokenType::OP_IFF: return "<->";
        case TokenType::OP_XOR: return "xor";
        case TokenType::OP_PLUS: return "+";
        case TokenType::OP_MINUS: return "-";
        case TokenType::OP_MUL: return "*";
        case TokenType::OP_DIV: return "/";
        case TokenType::OP_DIV_INT: return "div";
        case TokenType::OP_MOD: return "mod";
        case TokenType::OP_POW: return "^";
        case TokenType::OP_EQ: return "=";
        case TokenType::OP_NEQ: return "!=";
        case TokenType::OP_LT: return "<";
        case TokenType::OP_LE: return "<=";
        case TokenType::OP_GT: return ">";
        case TokenType::OP_GE: return ">=";
        case TokenType::OP_IN: return "in";
        case TokenType::OP_SUBSET: return "subset";
        case TokenType::OP_SUPERSET: return "superset";
        case TokenType::OP_UNION: return "union";
        case TokenType::OP_DIFF: return "diff";
        case TokenType::OP_SYMDIFF: return "symdiff";
        case TokenType::OP_INTERSECT: return "intersect";
        case TokenType::OP_RANGE: return "..";
        case TokenType::OP_RANGE_HALF_OPEN_L: return "..<";
        case TokenType::OP_RANGE_HALF_OPEN_R: return "<..";
        case TokenType::OP_RANGE_OPEN: return "<..<";
        case TokenType::OP_CONCAT: return "++";
        case TokenType::SEMICOLON: return ";";
        case TokenType::COLON: return ":";
        case TokenType::COMMA: return ",";
        case TokenType::DOT: return ".";
        case TokenType::LPAREN: return "(";
        case TokenType::RPAREN: return ")";
        case TokenType::LBRACKET: return "[";
        case TokenType::RBRACKET: return "]";
        case TokenType::LBRACE: return "{";
        case TokenType::RBRACE: return "}";
        case TokenType::PIPE: return "|";
        case TokenType::AMPERSAND: return "&";
        case TokenType::ARROW: return "->";
        case TokenType::ANNOTATION_START: return "::";
        case TokenType::ANON_VAR: return "_";
    }
    return "UNKNOWN";
}

bool Token::isKeyword() const {
    return type >= TokenType::KW_VAR && type <= TokenType::KW_ANNOTATION;
}

bool Token::isLiteral() const {
    return type == TokenType::INT_LIT || type == TokenType::FLOAT_LIT
        || type == TokenType::BOOL_LIT || type == TokenType::STRING_LIT;
}

bool Token::isOperator() const {
    return (type >= TokenType::OP_NOT && type <= TokenType::OP_CONCAT);
}

} // namespace SOMTParser::MiniZinc
