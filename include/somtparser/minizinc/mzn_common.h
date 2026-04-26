/* -*- Header -*-
 *
 * MiniZinc Frontend — Common Definitions
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction...
 */

#ifndef MZN_COMMON_H
#define MZN_COMMON_H

#include <stdexcept>
#include <string>
#include <sstream>

namespace SOMTParser::MiniZinc {

/**
 * @brief Source location within a MiniZinc file.
 */
struct SourceLoc {
    size_t line = 1;
    size_t col = 1;
    SourceLoc() = default;
    SourceLoc(size_t line, size_t col) : line(line), col(col) {}
    std::string toString() const {
        std::ostringstream oss;
        oss << line << ":" << col;
        return oss.str();
    }
};

/**
 * @brief Base exception for all MiniZinc frontend errors.
 */
class MznError : public std::runtime_error {
protected:
    SourceLoc loc;
    std::string msg;
public:
    MznError(const SourceLoc& loc, const std::string& what)
        : std::runtime_error(what), loc(loc), msg(what) {}
    const SourceLoc& getLoc() const { return loc; }
    const char* what() const noexcept override {
        return msg.c_str();
    }
};

/**
 * @brief Lexical error (unrecognized token, unclosed string, etc.)
 */
class MznLexicalError : public MznError {
public:
    MznLexicalError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "Lexical error at " + loc.toString() + ": " + what) {}
};

/**
 * @brief Syntax error (unexpected token, missing semicolon, etc.)
 */
class MznParseError : public MznError {
public:
    MznParseError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "Parse error at " + loc.toString() + ": " + what) {}
};

/**
 * @brief Type error (type mismatch, undefined identifier, etc.)
 */
class MznTypeError : public MznError {
public:
    MznTypeError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "Type error at " + loc.toString() + ": " + what) {}
};

/**
 * @brief Semantic error (duplicate declaration, invalid domain, etc.)
 */
class MznSemanticError : public MznError {
public:
    MznSemanticError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "Semantic error at " + loc.toString() + ": " + what) {}
};

/**
 * @brief DZN data file error.
 */
class MznDznError : public MznError {
public:
    MznDznError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "DZN error at " + loc.toString() + ": " + what) {}
};

/**
 * @brief Error for unsupported constructs (Phase 1–5 soft failures).
 */
class MznUnsupportedError : public MznError {
public:
    MznUnsupportedError(const SourceLoc& loc, const std::string& what)
        : MznError(loc, "Unsupported at " + loc.toString() + ": " + what) {}
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_COMMON_H
