/* -*- Header -*-
 *
 * MiniZinc Frontend — Token Definitions
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef MZN_TOKEN_H
#define MZN_TOKEN_H

#include "somtparser/minizinc/mzn_common.h"
#include <string>

namespace SOMTParser::MiniZinc {

/**
 * @brief Enumeration of all MiniZinc token types.
 *
 * Covers keywords, operators, literals, punctuation, and special tokens.
 */
enum class TokenType {
    // ── Sentinel ───────────────────────────────────────────────
    END_OF_FILE,
    ERROR,

    // ── Identifiers ────────────────────────────────────────────
    IDENT,          // foo, Bar, _123
    QUOTED_IDENT,   // 'foo bar'

    // ── Literals ───────────────────────────────────────────────
    INT_LIT,
    FLOAT_LIT,
    BOOL_LIT,       // true, false
    STRING_LIT,

    // ── Keywords ───────────────────────────────────────────────
    KW_VAR,
    KW_PAR,
    KW_OPT,
    KW_SET,
    KW_OF,
    KW_ARRAY,
    KW_LIST,
    KW_TUPLE,
    KW_RECORD,
    KW_ENUM,
    KW_BOOL,
    KW_INT,
    KW_FLOAT,
    KW_STRING,
    KW_ANN,
    KW_ANY,

    KW_CONSTRAINT,
    KW_SOLVE,
    KW_SATISFY,
    KW_MINIMIZE,
    KW_MAXIMIZE,
    KW_OUTPUT,

    KW_PREDICATE,
    KW_FUNCTION,
    KW_TEST,
    KW_INCLUDE,

    KW_IF,
    KW_THEN,
    KW_ELSE,
    KW_ELSEIF,
    KW_ENDIF,
    KW_LET,
    KW_IN,
    KW_WHERE,
    KW_ANNOTATION,

    // ── Boolean operators ──────────────────────────────────────
    OP_NOT,
    OP_AND,         // logical AND ("/\\")
    OP_OR,          // logical OR ("\\/")
    OP_IMPLIES,     // ->
    OP_IMPLIED_BY,  // <-
    OP_IFF,         // <->
    OP_XOR,

    // ── Arithmetic operators ───────────────────────────────────
    OP_PLUS,
    OP_MINUS,
    OP_MUL,
    OP_DIV,         // /
    OP_DIV_INT,     // integer division
    OP_MOD,         // modulo
    OP_POW,         // power

    // ── Comparison operators ───────────────────────────────────
    OP_EQ,          // =
    OP_NEQ,         // !=
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,

    // ── Set operators ──────────────────────────────────────────
    OP_IN,
    OP_SUBSET,
    OP_SUPERSET,
    OP_UNION,
    OP_DIFF,
    OP_SYMDIFF,
    OP_INTERSECT,

    // ── Range / Array operators ────────────────────────────────
    OP_RANGE,       // range ("..")
    OP_RANGE_HALF_OPEN_L,  // half-open range left ("..<")
    OP_RANGE_HALF_OPEN_R,  // half-open range right ("<..")
    OP_RANGE_OPEN,         // open range ("<..<")
    OP_CONCAT,      // concatenation ("++")

    // ── Punctuation ────────────────────────────────────────────
    SEMICOLON,
    COLON,
    COMMA,
    DOT,
    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    LBRACE,
    RBRACE,
    PIPE,
    AMPERSAND,
    ARROW,          // -> (also used in function types / records)

    // ── Annotation ─────────────────────────────────────────────
    ANNOTATION_START,  // ::

    // ── Anonymous variable ─────────────────────────────────────
    ANON_VAR,       // _
};

/**
 * @brief Convert a TokenType to its human-readable string.
 */
std::string tokenTypeToString(TokenType type);

inline std::ostream& operator<<(std::ostream& os, TokenType type) {
    return os << tokenTypeToString(type);
}

/**
 * @brief A single token produced by the lexer.
 */
struct Token {
    TokenType type;
    std::string text;     // raw text from source
    SourceLoc loc;

    Token() : type(TokenType::ERROR), text(""), loc() {}
    Token(TokenType type, const std::string& text, const SourceLoc& loc)
        : type(type), text(text), loc(loc) {}

    bool is(TokenType t) const { return type == t; }
    bool isKeyword() const;
    bool isLiteral() const;
    bool isOperator() const;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_TOKEN_H
