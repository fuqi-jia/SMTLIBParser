/* -*- C++ -*-
 *
 * MiniZinc Frontend — Lexer Tests
 */

#include "somtparser/frontends/minizinc/mzn_lexer.h"
#include "somtparser/frontends/minizinc/mzn_token.h"
#include <iostream>
#include <cassert>
#include <vector>

using namespace SOMTParser::MiniZinc;

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "  Running " #name "... "; \
    try { test_##name(); tests_passed++; std::cout << "OK\n"; } \
    catch (const std::exception& e) { tests_failed++; std::cout << "FAILED: " << e.what() << "\n"; } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        std::ostringstream oss; \
        oss << "Assertion failed: " #a " == " #b " (got " << (a) << " vs " << (b) << ")"; \
        throw std::runtime_error(oss.str()); \
    } \
} while(0)

#define ASSERT_TRUE(x) do { \
    if (!(x)) { \
        throw std::runtime_error("Assertion failed: " #x); \
    } \
} while(0)

// ── Helper: tokenize and check types ───────────────────────────────
static std::vector<Token> tokenize(const std::string& src) {
    MznLexer lexer(src);
    return lexer.tokenizeAll();
}

static void assertTokenTypes(const std::vector<Token>& toks,
                              const std::vector<TokenType>& expected) {
    ASSERT_EQ(toks.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        if (toks[i].type != expected[i]) {
            std::ostringstream oss;
            oss << "Token[" << i << "] expected " << tokenTypeToString(expected[i])
                << " but got " << tokenTypeToString(toks[i].type)
                << " (\"" << toks[i].text << "\")";
            throw std::runtime_error(oss.str());
        }
    }
}

// ── Tests ──────────────────────────────────────────────────────────

TEST(empty) {
    auto toks = tokenize("");
    ASSERT_EQ(toks.size(), 1);
    ASSERT_TRUE(toks[0].is(TokenType::END_OF_FILE));
}

TEST(keywords) {
    auto toks = tokenize("var par opt set of array bool int float solve satisfy minimize maximize constraint output predicate function test include if then else endif let in where enum tuple record ann any list");
    std::vector<TokenType> expected = {
        TokenType::KW_VAR, TokenType::KW_PAR, TokenType::KW_OPT,
        TokenType::KW_SET, TokenType::KW_OF, TokenType::KW_ARRAY,
        TokenType::KW_BOOL, TokenType::KW_INT, TokenType::KW_FLOAT,
        TokenType::KW_SOLVE, TokenType::KW_SATISFY, TokenType::KW_MINIMIZE,
        TokenType::KW_MAXIMIZE, TokenType::KW_CONSTRAINT, TokenType::KW_OUTPUT,
        TokenType::KW_PREDICATE, TokenType::KW_FUNCTION, TokenType::KW_TEST,
        TokenType::KW_INCLUDE, TokenType::KW_IF, TokenType::KW_THEN,
        TokenType::KW_ELSE, TokenType::KW_ENDIF, TokenType::KW_LET,
        TokenType::KW_IN, TokenType::KW_WHERE, TokenType::KW_ENUM,
        TokenType::KW_TUPLE, TokenType::KW_RECORD, TokenType::KW_ANN,
        TokenType::KW_ANY, TokenType::KW_LIST,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(boolean_operators) {
    auto toks = tokenize("not /\\ \\/ -> <- <-> xor");
    std::vector<TokenType> expected = {
        TokenType::OP_NOT, TokenType::OP_AND, TokenType::OP_OR,
        TokenType::OP_IMPLIES, TokenType::OP_IMPLIED_BY,
        TokenType::OP_IFF, TokenType::OP_XOR,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(arithmetic_operators) {
    auto toks = tokenize("+ - * / div mod ^");
    std::vector<TokenType> expected = {
        TokenType::OP_PLUS, TokenType::OP_MINUS, TokenType::OP_MUL,
        TokenType::OP_DIV, TokenType::OP_DIV_INT, TokenType::OP_MOD,
        TokenType::OP_POW,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(comparison_operators) {
    auto toks = tokenize("= != < <= > >=");
    std::vector<TokenType> expected = {
        TokenType::OP_EQ, TokenType::OP_NEQ,
        TokenType::OP_LT, TokenType::OP_LE,
        TokenType::OP_GT, TokenType::OP_GE,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(set_operators) {
    auto toks = tokenize("in subset superset union diff symdiff intersect");
    std::vector<TokenType> expected = {
        TokenType::KW_IN, TokenType::OP_SUBSET, TokenType::OP_SUPERSET,
        TokenType::OP_UNION, TokenType::OP_DIFF, TokenType::OP_SYMDIFF,
        TokenType::OP_INTERSECT,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(range_operators) {
    auto toks = tokenize(".. ..< <.. <..<");
    std::vector<TokenType> expected = {
        TokenType::OP_RANGE, TokenType::OP_RANGE_HALF_OPEN_L,
        TokenType::OP_RANGE_HALF_OPEN_R, TokenType::OP_RANGE_OPEN,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(punctuation) {
    auto toks = tokenize("; : , . ( ) [ ] { } | & ::");
    std::vector<TokenType> expected = {
        TokenType::SEMICOLON, TokenType::COLON, TokenType::COMMA,
        TokenType::DOT, TokenType::LPAREN, TokenType::RPAREN,
        TokenType::LBRACKET, TokenType::RBRACKET,
        TokenType::LBRACE, TokenType::RBRACE,
        TokenType::PIPE, TokenType::AMPERSAND,
        TokenType::ANNOTATION_START,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(identifiers) {
    auto toks = tokenize("foo Bar_baz _123 x'");
    ASSERT_EQ(toks[0].type, TokenType::IDENT);
    ASSERT_EQ(toks[0].text, "foo");
    ASSERT_EQ(toks[1].type, TokenType::IDENT);
    ASSERT_EQ(toks[1].text, "Bar_baz");
    ASSERT_EQ(toks[2].type, TokenType::IDENT);
    ASSERT_EQ(toks[2].text, "_123");
}

TEST(quoted_identifiers) {
    auto toks = tokenize("'foo bar' '123abc'");
    ASSERT_EQ(toks[0].type, TokenType::QUOTED_IDENT);
    ASSERT_EQ(toks[0].text, "foo bar");
    ASSERT_EQ(toks[1].type, TokenType::QUOTED_IDENT);
    ASSERT_EQ(toks[1].text, "123abc");
}

TEST(int_literals) {
    auto toks = tokenize("42 0 -7");
    ASSERT_EQ(toks[0].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[0].text, "42");
    ASSERT_EQ(toks[1].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[1].text, "0");
    ASSERT_EQ(toks[2].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[2].text, "-7");
}

TEST(float_literals) {
    auto toks = tokenize("3.14 -2.5 1e10 6.022E-23");
    ASSERT_EQ(toks[0].type, TokenType::FLOAT_LIT);
    ASSERT_EQ(toks[0].text, "3.14");
    ASSERT_EQ(toks[1].type, TokenType::FLOAT_LIT);
    ASSERT_EQ(toks[1].text, "-2.5");
    ASSERT_EQ(toks[2].type, TokenType::FLOAT_LIT);
    ASSERT_EQ(toks[2].text, "1e10");
    ASSERT_EQ(toks[3].type, TokenType::FLOAT_LIT);
    ASSERT_EQ(toks[3].text, "6.022E-23");
}

TEST(bool_literals) {
    auto toks = tokenize("true false");
    ASSERT_EQ(toks[0].type, TokenType::BOOL_LIT);
    ASSERT_EQ(toks[0].text, "true");
    ASSERT_EQ(toks[1].type, TokenType::BOOL_LIT);
    ASSERT_EQ(toks[1].text, "false");
}

TEST(string_literals) {
    auto toks = tokenize("\"hello\" \"esc\\\"aped\" \"line1\\nline2\"");
    ASSERT_EQ(toks[0].type, TokenType::STRING_LIT);
    ASSERT_EQ(toks[0].text, "hello");
    ASSERT_EQ(toks[1].type, TokenType::STRING_LIT);
    ASSERT_EQ(toks[1].text, "esc\"aped");
    ASSERT_EQ(toks[2].type, TokenType::STRING_LIT);
    ASSERT_EQ(toks[2].text, "line1\nline2");
}

TEST(line_comments) {
    auto toks = tokenize("% this is a comment\n42");
    ASSERT_EQ(toks[0].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[0].text, "42");
}

TEST(block_comments) {
    auto toks = tokenize("/* block */ 42");
    ASSERT_EQ(toks[0].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[0].text, "42");
}

TEST(nested_block_comments) {
    auto toks = tokenize("/* outer /* inner */ */ 42");
    ASSERT_EQ(toks[0].type, TokenType::INT_LIT);
    ASSERT_EQ(toks[0].text, "42");
}

TEST(anonymous_var) {
    auto toks = tokenize("_");
    ASSERT_EQ(toks[0].type, TokenType::ANON_VAR);
}

TEST(concat_operator) {
    auto toks = tokenize("++");
    ASSERT_EQ(toks[0].type, TokenType::OP_CONCAT);
}

TEST(complex_expression) {
    std::string src = "var int: x; constraint x >= 0;";
    auto toks = tokenize(src);
    std::vector<TokenType> expected = {
        TokenType::KW_VAR, TokenType::KW_INT, TokenType::COLON,
        TokenType::IDENT, TokenType::SEMICOLON,
        TokenType::KW_CONSTRAINT, TokenType::IDENT,
        TokenType::OP_GE, TokenType::INT_LIT, TokenType::SEMICOLON,
        TokenType::END_OF_FILE
    };
    assertTokenTypes(toks, expected);
}

TEST(error_unclosed_string) {
    auto toks = tokenize("\"unclosed");
    // Should have ERROR token before EOF
    bool has_error = false;
    for (const auto& t : toks) {
        if (t.type == TokenType::ERROR) {
            has_error = true;
            break;
        }
    }
    ASSERT_TRUE(has_error);
}

TEST(error_invalid_char) {
    auto toks = tokenize("@");
    ASSERT_EQ(toks[0].type, TokenType::ERROR);
}

// ── Main ───────────────────────────────────────────────────────────

int main() {
    std::cout << "======= MiniZinc Lexer Tests =======\n\n";

    RUN_TEST(empty);
    RUN_TEST(keywords);
    RUN_TEST(boolean_operators);
    RUN_TEST(arithmetic_operators);
    RUN_TEST(comparison_operators);
    RUN_TEST(set_operators);
    RUN_TEST(range_operators);
    RUN_TEST(punctuation);
    RUN_TEST(identifiers);
    RUN_TEST(quoted_identifiers);
    RUN_TEST(int_literals);
    RUN_TEST(float_literals);
    RUN_TEST(bool_literals);
    RUN_TEST(string_literals);
    RUN_TEST(line_comments);
    RUN_TEST(block_comments);
    RUN_TEST(nested_block_comments);
    RUN_TEST(anonymous_var);
    RUN_TEST(concat_operator);
    RUN_TEST(complex_expression);
    RUN_TEST(error_unclosed_string);
    RUN_TEST(error_invalid_char);

    std::cout << "\n=====================================\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";

    return tests_failed > 0 ? 1 : 0;
}
