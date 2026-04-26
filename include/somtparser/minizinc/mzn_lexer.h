/* -*- Header -*-
 *
 * MiniZinc Frontend — Lexer
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef MZN_LEXER_H
#define MZN_LEXER_H

#include "somtparser/minizinc/mzn_common.h"
#include "somtparser/minizinc/mzn_token.h"

#include <string>
#include <vector>

namespace SOMTParser::MiniZinc {

/**
 * @brief Hand-written lexer for MiniZinc.
 *
 * Operates directly on a char buffer. Supports UTF-8 source files,
 * line/block comments, string literals with escapes, and all MiniZinc
 * operators including multi-character tokens like /\, \/, <->, ::, etc.
 */
class MznLexer {
public:
    MznLexer();
    explicit MznLexer(const std::string& source);

    void init(const std::string& source);

    /**
     * @brief Return the next token from the input.
     */
    Token nextToken();

    /**
     * @brief Peek at the next token without consuming it.
     */
    Token peekToken();

    /**
     * @brief Tokenize the entire source into a vector.
     */
    std::vector<Token> tokenizeAll();

    const SourceLoc& getCurrentLoc() const { return loc; }
    bool isAtEnd() const;

private:
    std::string source_str;
    const char* source = nullptr;
    size_t length = 0;
    size_t pos = 0;
    SourceLoc loc;
    Token cached_peek;
    bool has_peek = false;

    // Character handling
    char current() const;
    char advance();
    char peek() const;
    char peekNext(size_t offset = 1) const;
    bool match(char expected);
    void skipWhitespace();
    void skipComment();
    void skipBlockComment();

    // Token factories
    Token makeToken(TokenType type, const std::string& text);
    Token makeToken(TokenType type, size_t start_pos);
    Token errorToken(const std::string& msg);

    // Literal scanners
    Token scanIdentifier();
    Token scanQuotedIdentifier();
    Token scanNumber();
    Token scanString();

    // Keyword lookup
    static TokenType lookupKeyword(const std::string& text);

    // Location tracking
    void newLine();
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_LEXER_H
