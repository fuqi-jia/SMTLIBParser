/* -*- Header -*-
 *
 * MiniZinc Frontend — Parser
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef MZN_PARSER_H
#define MZN_PARSER_H

#include "somtparser/minizinc/mzn_lexer.h"
#include "somtparser/minizinc/mzn_ast.h"

#include <string>
#include <vector>

namespace SOMTParser::MiniZinc {

/**
 * @brief Hand-written recursive-descent parser for MiniZinc 2.8+.
 *
 * Parses a .mzn source file into a MiniZinc AST (MznAST::Model).
 * Uses a Pratt parser (top-down operator precedence) for expressions.
 */
class MznParser {
public:
    MznParser();

    /**
     * @brief Parse a complete .mzn file into a Model.
     */
    Model parseFile(const std::string& filename);

    /**
     * @brief Parse a MiniZinc model from a string.
     */
    Model parseString(const std::string& source, const std::string& filename = "<string>");

private:
    MznLexer lexer;
    Token current;
    bool has_current = false;
    std::string current_filename;

    // ── Token consumption ──────────────────────────────────────
    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& err_msg);

    // ── Top-level items ────────────────────────────────────────
    ItemPtr parseItem();
    ItemPtr parseInclude();
    ItemPtr parseVarDecl();
    ItemPtr parseAssignOrVarDecl();
    ItemPtr parseConstraint();
    ItemPtr parseSolve();
    ItemPtr parseOutput();
    ItemPtr parsePredicate();
    ItemPtr parseFunction();
    ItemPtr parseTest();
    ItemPtr parseAnnotationDecl();
    ItemPtr parseEnumDecl();

    // ── Types ──────────────────────────────────────────────────
    std::shared_ptr<TypeInst> parseTypeInst();
    void parseBaseTypeInto(TypeInst& type);
    std::shared_ptr<TypeInst> parseArrayType(std::shared_ptr<TypeInst> type);

    // ── Expressions (Pratt parser) ─────────────────────────────
    ExprPtr parseExpr(int min_prec = 0);
    ExprPtr parsePrimary();
    ExprPtr parseCallOrIdent();
    ExprPtr parseArrayLiteral();
    ExprPtr parseSetLiteral();
    ExprPtr parseTupleLiteral();
    ExprPtr parseRecordLiteral();
    ExprPtr parseIfThenElse();
    ExprPtr parseLet();
    ExprPtr parseComprehensionOrCall(ExprPtr head);

    // ── Generators ─────────────────────────────────────────────
    std::vector<Generator> parseGenerators();

    // ── Helpers ────────────────────────────────────────────────
    static int getPrefixPrecedence(TokenType type);
    static int getInfixPrecedence(TokenType type);
    static bool isRightAssociative(TokenType type);
    BinaryOp::Op tokenToBinaryOp(TokenType type);
    UnaryOp::Op tokenToUnaryOp(TokenType type);

    // Error reporting
    [[noreturn]] static void errorStatic(const std::string& msg);
    [[noreturn]] void error(const std::string& msg);
    [[noreturn]] void errorAt(const Token& tok, const std::string& msg);
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_PARSER_H
