/* -*- C++ -*-
 *
 * MiniZinc Frontend — Lexer Implementation
 */

#include "somtparser/minizinc/mzn_lexer.h"

#include <cctype>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

// ── Keyword table ──────────────────────────────────────────────────
static const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"var", TokenType::KW_VAR},
    {"par", TokenType::KW_PAR},
    {"opt", TokenType::KW_OPT},
    {"set", TokenType::KW_SET},
    {"of", TokenType::KW_OF},
    {"array", TokenType::KW_ARRAY},
    {"list", TokenType::KW_LIST},
    {"tuple", TokenType::KW_TUPLE},
    {"record", TokenType::KW_RECORD},
    {"enum", TokenType::KW_ENUM},
    {"bool", TokenType::KW_BOOL},
    {"int", TokenType::KW_INT},
    {"float", TokenType::KW_FLOAT},
    {"string", TokenType::KW_STRING},
    {"ann", TokenType::KW_ANN},
    {"any", TokenType::KW_ANY},
    {"constraint", TokenType::KW_CONSTRAINT},
    {"solve", TokenType::KW_SOLVE},
    {"satisfy", TokenType::KW_SATISFY},
    {"minimize", TokenType::KW_MINIMIZE},
    {"maximize", TokenType::KW_MAXIMIZE},
    {"output", TokenType::KW_OUTPUT},
    {"predicate", TokenType::KW_PREDICATE},
    {"function", TokenType::KW_FUNCTION},
    {"test", TokenType::KW_TEST},
    {"include", TokenType::KW_INCLUDE},
    {"if", TokenType::KW_IF},
    {"then", TokenType::KW_THEN},
    {"else", TokenType::KW_ELSE},
    {"elseif", TokenType::KW_ELSEIF},
    {"endif", TokenType::KW_ENDIF},
    {"let", TokenType::KW_LET},
    {"in", TokenType::KW_IN},
    {"where", TokenType::KW_WHERE},
    {"annotation", TokenType::KW_ANNOTATION},
    {"not", TokenType::OP_NOT},
    {"div", TokenType::OP_DIV_INT},
    {"mod", TokenType::OP_MOD},
    {"xor", TokenType::OP_XOR},
    {"union", TokenType::OP_UNION},
    {"intersect", TokenType::OP_INTERSECT},
    {"diff", TokenType::OP_DIFF},
    {"symdiff", TokenType::OP_SYMDIFF},
    {"subset", TokenType::OP_SUBSET},
    {"superset", TokenType::OP_SUPERSET},
    {"true", TokenType::BOOL_LIT},
    {"false", TokenType::BOOL_LIT},
};

// ── Constructor / Init ─────────────────────────────────────────────
MznLexer::MznLexer() = default;

MznLexer::MznLexer(const std::string& source) {
    init(source);
}

void MznLexer::init(const std::string& src) {
    source_str = src;
    this->source = source_str.c_str();
    this->length = source_str.size();
    this->pos = 0;
    this->loc = SourceLoc(1, 1);
    this->has_peek = false;
}

// ── Character helpers ──────────────────────────────────────────────
char MznLexer::current() const {
    if (pos >= length) return '\0';
    return source[pos];
}

char MznLexer::advance() {
    if (pos >= length) return '\0';
    char c = source[pos++];
    if (c == '\n') {
        loc.line++;
        loc.col = 1;
    } else {
        loc.col++;
    }
    return c;
}

char MznLexer::peek() const {
    if (pos >= length) return '\0';
    return source[pos];
}

char MznLexer::peekNext(size_t offset) const {
    if (pos + offset >= length) return '\0';
    return source[pos + offset];
}

bool MznLexer::match(char expected) {
    if (current() == expected) {
        advance();
        return true;
    }
    return false;
}

bool MznLexer::isAtEnd() const {
    return pos >= length;
}

void MznLexer::newLine() {
    loc.line++;
    loc.col = 1;
}

// ── Skip whitespace and comments ───────────────────────────────────
void MznLexer::skipWhitespace() {
    while (true) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '%') {
            skipComment();
        } else if (c == '/' && peekNext() == '*') {
            skipBlockComment();
        } else {
            break;
        }
    }
}

void MznLexer::skipComment() {
    // Line comment: % to end of line
    while (current() != '\n' && !isAtEnd()) {
        advance();
    }
}

void MznLexer::skipBlockComment() {
    // Block comment: /* ... */  (supports nesting)
    advance(); // '/'
    advance(); // '*'
    int depth = 1;
    while (depth > 0 && !isAtEnd()) {
        if (current() == '/' && peekNext() == '*') {
            advance();
            advance();
            depth++;
        } else if (current() == '*' && peekNext() == '/') {
            advance();
            advance();
            depth--;
        } else {
            advance();
        }
    }
}

// ── Token factories ────────────────────────────────────────────────
Token MznLexer::makeToken(TokenType type, const std::string& text) {
    return Token(type, text, loc);
}

Token MznLexer::makeToken(TokenType type, size_t start_pos) {
    return Token(type, std::string(source + start_pos, pos - start_pos), loc);
}

Token MznLexer::errorToken(const std::string& msg) {
    return Token(TokenType::ERROR, msg, loc);
}

// ── Scanners ───────────────────────────────────────────────────────
Token MznLexer::scanIdentifier() {
    size_t start = pos;
    while (std::isalnum(current()) || current() == '_') {
        advance();
    }
    std::string text(source + start, pos - start);
    auto it = KEYWORDS.find(text);
    if (it != KEYWORDS.end()) {
        return Token(it->second, text, loc);
    }
    return Token(TokenType::IDENT, text, loc);
}

Token MznLexer::scanQuotedIdentifier() {
    // 'foo bar'
    size_t start = pos;
    advance(); // '
    while (current() != '\'' && !isAtEnd()) {
        advance();
    }
    if (isAtEnd()) {
        return errorToken("Unterminated quoted identifier");
    }
    advance(); // '
    std::string text(source + start + 1, pos - start - 2);
    return Token(TokenType::QUOTED_IDENT, text, loc);
}

Token MznLexer::scanNumber() {
    size_t start = pos;
    bool is_float = false;

    // Optional leading minus (handled at expression level, but allow here for literals)
    if (current() == '-') {
        advance();
    }

    // Integer part
    while (std::isdigit(current())) {
        advance();
    }

    // Fractional part
    if (current() == '.' && std::isdigit(peekNext())) {
        is_float = true;
        advance(); // '.'
        while (std::isdigit(current())) {
            advance();
        }
    }

    // Exponent
    if (current() == 'e' || current() == 'E') {
        is_float = true;
        advance();
        if (current() == '+' || current() == '-') {
            advance();
        }
        while (std::isdigit(current())) {
            advance();
        }
    }

    std::string text(source + start, pos - start);
    if (is_float) {
        return Token(TokenType::FLOAT_LIT, text, loc);
    }
    return Token(TokenType::INT_LIT, text, loc);
}

Token MznLexer::scanString() {
    size_t start = pos;
    advance(); // '"'
    std::string value;
    while (current() != '"' && !isAtEnd()) {
        if (current() == '\\') {
            advance();
            char esc = current();
            switch (esc) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += esc; break;
            }
            advance();
        } else {
            value += current();
            advance();
        }
    }
    if (isAtEnd()) {
        return errorToken("Unterminated string literal");
    }
    advance(); // '"'
    return Token(TokenType::STRING_LIT, value, loc);
}

// ── Main nextToken ─────────────────────────────────────────────────
Token MznLexer::nextToken() {
    if (has_peek) {
        has_peek = false;
        return cached_peek;
    }

    skipWhitespace();

    if (isAtEnd()) {
        return Token(TokenType::END_OF_FILE, "", loc);
    }

    size_t start = pos;
    char c = current();

    // Anonymous variable (must be checked before generic identifier)
    if (c == '_') {
        size_t start = pos;
        advance();
        char next = current();
        if (next == '\0' || (!std::isalnum(static_cast<unsigned char>(next)) && next != '_')) {
            return Token(TokenType::ANON_VAR, "_", loc);
        }
        // Backtrack and scan as identifier
        pos = start;
        loc.col--;
        return scanIdentifier();
    }

    // Identifiers and keywords
    if (std::isalpha(c)) {
        return scanIdentifier();
    }

    // Quoted identifier
    if (c == '\'') {
        return scanQuotedIdentifier();
    }

    // Numbers
    if (std::isdigit(c) || (c == '-' && std::isdigit(peekNext()))) {
        return scanNumber();
    }

    // Strings
    if (c == '"') {
        return scanString();
    }

    // Multi-character and single-character operators
    switch (c) {
        case '/':
            advance();
            if (match('\\')) return Token(TokenType::OP_AND, "/\\", loc);
            if (match('*')) { skipBlockComment(); return nextToken(); }
            return Token(TokenType::OP_DIV, "/", loc);

        case '\\':
            advance();
            if (match('/')) return Token(TokenType::OP_OR, "\\/", loc);
            return errorToken("Unexpected backslash");

        case '<':
            advance();
            if (match('-')) {
                if (match('>')) return Token(TokenType::OP_IFF, "<->", loc);
                return Token(TokenType::OP_IMPLIED_BY, "<-", loc);
            }
            if (match('=')) return Token(TokenType::OP_LE, "<=", loc);
            if (match('>')) return Token(TokenType::OP_NEQ, "!=", loc);
            if (match('.')) {
                if (match('.')) {
                    if (match('<')) return Token(TokenType::OP_RANGE_OPEN, "<..<", loc);
                    return Token(TokenType::OP_RANGE_HALF_OPEN_R, "<..", loc);
                }
                return errorToken("Unexpected <.");
            }
            return Token(TokenType::OP_LT, "<", loc);

        case '>':
            advance();
            if (match('=')) return Token(TokenType::OP_GE, ">=", loc);
            return Token(TokenType::OP_GT, ">", loc);

        case '=':
            advance();
            if (match('=')) return Token(TokenType::OP_EQ, "==", loc);
            return Token(TokenType::OP_EQ, "=", loc);

        case '!':
            advance();
            if (match('=')) return Token(TokenType::OP_NEQ, "!=", loc);
            return errorToken("Unexpected '!'");

        case '-':
            advance();
            if (match('>')) return Token(TokenType::OP_IMPLIES, "->", loc);
            return Token(TokenType::OP_MINUS, "-", loc);

        case '+':
            advance();
            if (match('+')) return Token(TokenType::OP_CONCAT, "++", loc);
            return Token(TokenType::OP_PLUS, "+", loc);

        case '*':
            advance();
            return Token(TokenType::OP_MUL, "*", loc);

        case '^':
            advance();
            return Token(TokenType::OP_POW, "^", loc);

        case '.':
            advance();
            if (match('.')) {
                if (match('<')) return Token(TokenType::OP_RANGE_HALF_OPEN_L, "..<", loc);
                return Token(TokenType::OP_RANGE, "..", loc);
            }
            return Token(TokenType::DOT, ".", loc);

        case ':':
            advance();
            if (match(':')) return Token(TokenType::ANNOTATION_START, "::", loc);
            return Token(TokenType::COLON, ":", loc);

        case ';': advance(); return Token(TokenType::SEMICOLON, ";", loc);
        case ',': advance(); return Token(TokenType::COMMA, ",", loc);
        case '(': advance(); return Token(TokenType::LPAREN, "(", loc);
        case ')': advance(); return Token(TokenType::RPAREN, ")", loc);
        case '[': advance(); return Token(TokenType::LBRACKET, "[", loc);
        case ']': advance(); return Token(TokenType::RBRACKET, "]", loc);
        case '{': advance(); return Token(TokenType::LBRACE, "{", loc);
        case '}': advance(); return Token(TokenType::RBRACE, "}", loc);
        case '|': advance(); return Token(TokenType::PIPE, "|", loc);
        case '&': advance(); return Token(TokenType::AMPERSAND, "&", loc);

        default:
            advance();
            return errorToken(std::string("Unexpected character: ") + c);
    }
}

Token MznLexer::peekToken() {
    if (!has_peek) {
        cached_peek = nextToken();
        has_peek = true;
    }
    return cached_peek;
}

std::vector<Token> MznLexer::tokenizeAll() {
    std::vector<Token> tokens;
    while (true) {
        Token tok = nextToken();
        tokens.push_back(tok);
        if (tok.type == TokenType::END_OF_FILE || tok.type == TokenType::ERROR) {
            break;
        }
    }
    return tokens;
}

} // namespace SOMTParser::MiniZinc
