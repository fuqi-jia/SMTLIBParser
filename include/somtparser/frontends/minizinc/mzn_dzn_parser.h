/* -*- Header -*-
 *
 * MiniZinc Frontend — .dzn Data File Parser & Merger
 *
 * Parses MiniZinc data files (.dzn) and merges them into a Model.
 */

#ifndef MZN_DZN_PARSER_H
#define MZN_DZN_PARSER_H

#include "somtparser/frontends/minizinc/mzn_ast.h"
#include "somtparser/frontends/minizinc/mzn_lexer.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace SOMTParser::MiniZinc {

// ── Parsed .dzn data ─────────────────────────────────────────────
struct DznData {
    std::unordered_map<std::string, ExprPtr> assignments;
    std::vector<std::string> ordered_keys;

    bool has(const std::string& key) const {
        return assignments.find(key) != assignments.end();
    }
    ExprPtr get(const std::string& key) const {
        auto it = assignments.find(key);
        return (it != assignments.end()) ? it->second : nullptr;
    }
};

/**
 * @brief Parser for MiniZinc data files (.dzn subset).
 */
class MznDznParser {
public:
    MznDznParser();

    DznData parseFile(const std::string& filename);
    DznData parseString(const std::string& source, const std::string& filename = "<dzn>");

private:
    MznLexer lexer;
    Token current;
    bool has_current = false;
    std::string current_filename;

    void advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    Token consume(TokenType type, const std::string& err_msg);
    void error(const std::string& msg);
    void errorAt(const Token& tok, const std::string& msg);

    void parseAssignment(DznData& data);
    ExprPtr parseExpr();
    ExprPtr parsePrimary();
    ExprPtr parseArrayLiteral();
    ExprPtr parseSetLiteral();
    ExprPtr parseArray2d();
};

/**
 * @brief Merges .dzn data into a .mzn Model.
 *
 * Replaces par declarations with their data values and marks them fixed.
 */
class MznDznMerger {
public:
    // Merge data into model; returns a new Model with inlined parameters.
    Model merge(const Model& model, const DznData& data);

    // Validate that a data value matches the expected type.
    bool typeCheckMerge(const TypeInst& expected, const ExprPtr& value,
                        std::string& out_error) const;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_DZN_PARSER_H
