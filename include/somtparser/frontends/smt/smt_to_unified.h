/* -*- Header -*-
 *
 * SMT-LIB AST → Unified IR converter.
 *
 * Provides a parallel path alongside the fast text→DAGNode pipeline:
 *   Script / Command / DAGNode  →  Unified::Model
 *
 * This enables convergence with MiniZinc/NL frontends for comparison,
 * transformation, and cross-language tooling.
 */

#ifndef SMT_TO_UNIFIED_H
#define SMT_TO_UNIFIED_H

#include "somtparser/frontend/command.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"

#include <memory>
#include <string>
#include <vector>

namespace SOMTParser {

// ── Forward declarations ───────────────────────────────────────────
class Script;
class Command;
class DAGNode;
class Sort;

// ── SMT-LIB → Unified IR converter ─────────────────────────────────

class SmtLibToUnifiedIR {
public:
    explicit SmtLibToUnifiedIR(const Unified::UnifiedOpRegistry& registry);

    /** Convert an entire SMT-LIB script to a Unified model.
     *
     *  @note The Script currently only records command types (not full
     *        expressions/names/sorts). For a complete conversion use
     *        convert(const Parser&) instead.
     */
    Unified::UnifiedModel convert(const Script& script);

    /** Convert a parsed Parser (with internal state) to a Unified model.
     *
     *  Uses Parser::getAssertions(), getVariables(), getObjectives(), etc.
     *  This is the recommended path for full SMT-LIB → Unified IR conversion.
     */
    Unified::UnifiedModel convert(const Parser& parser);

    /** Convert a single DAGNode subtree to a Unified expression. */
    Unified::ExprPtr convertExpr(const std::shared_ptr<DAGNode>& node);

    /** Access accumulated errors (non-fatal). */
    const std::vector<std::string>& errors() const { return errors_; }

private:
    const Unified::UnifiedOpRegistry& registry_;
    std::vector<std::string> errors_;

    // ── Sort conversion ──────────────────────────────────────────────
    Unified::UnifiedSort convertSort(const std::shared_ptr<Sort>& sort) const;

    // ── Command dispatch ─────────────────────────────────────────────
    void convertCommand(const Command& cmd, Unified::UnifiedModel& model);

    // ── Expression dispatch ──────────────────────────────────────────
    Unified::ExprPtr dispatchExpr(const std::shared_ptr<DAGNode>& node);

    Unified::ExprPtr convertLiteral(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertIdent(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertOpNode(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertIte(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertLet(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertQuantifier(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertArrayOp(const std::shared_ptr<DAGNode>& node);
    Unified::ExprPtr convertUFApplication(const std::shared_ptr<DAGNode>& node);

    // ── Helpers ──────────────────────────────────────────────────────
    std::string nodeKindToSmtName(NODE_KIND kind) const;
    Unified::UnifiedOpRef lookupOpBySmtName(const std::string& name) const;
    void addError(const std::string& msg);
};

} // namespace SOMTParser

#endif // SMT_TO_UNIFIED_H
