/* -*- Header -*-
 *
 * UnifiedPrinter — Pretty-print Unified IR to target languages.
 *
 * Supports: SMT-LIB2, MiniZinc, and a human-readable debug format.
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef UNIFIED_PRINTER_H
#define UNIFIED_PRINTER_H

#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"

#include <ostream>
#include <sstream>
#include <string>

namespace SOMTParser::Unified {

class UnifiedPrinter {
public:
    explicit UnifiedPrinter(const UnifiedOpRegistry& registry)
        : registry_(registry) {}

    // ── Target language output ─────────────────────────────────────

    /** Print model to SMT-LIB2 format. */
    std::string toSmtLib(const UnifiedModel& model) const;
    void printSmtLib(std::ostream& os, const UnifiedModel& model) const;

    /** Print model to MiniZinc format. */
    std::string toMiniZinc(const UnifiedModel& model) const;
    void printMiniZinc(std::ostream& os, const UnifiedModel& model) const;

    /** Print single expression to SMT-LIB2. */
    std::string exprToSmtLib(ExprPtr expr) const;
    void printExprSmtLib(std::ostream& os, ExprPtr expr) const;

    /** Print single expression to MiniZinc. */
    std::string exprToMiniZinc(ExprPtr expr) const;
    void printExprMiniZinc(std::ostream& os, ExprPtr expr) const;

    /** Human-readable debug dump (uses unified op names). */
    std::string toDebugString(ExprPtr expr) const;
    void printDebug(std::ostream& os, ExprPtr expr, int indent = 0) const;

    /** Print type to MiniZinc syntax. */
    std::string typeToMiniZinc(const UnifiedType& type) const;

    /** Print type to SMT-LIB sort syntax. */
    std::string typeToSmtLib(const UnifiedType& type) const;

private:
    const UnifiedOpRegistry& registry_;

    void printExprSmtLibImpl(std::ostream& os, ExprPtr expr) const;
    void printExprMiniZincImpl(std::ostream& os, ExprPtr expr) const;
    void printLiteral(std::ostream& os, const UnifiedExpr::Literal& lit) const;
    void printVarDeclMiniZinc(std::ostream& os, const UnifiedVarDecl& decl) const;
    void printVarDeclSmtLib(std::ostream& os, const UnifiedVarDecl& decl) const;

    std::string escapeString(const std::string& s) const;
    std::string mangleIdent(const std::string& name) const; // for SMT-LIB compat
};

} // namespace SOMTParser::Unified

#endif // UNIFIED_PRINTER_H
