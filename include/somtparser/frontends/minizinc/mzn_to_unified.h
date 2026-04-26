/* -*- Header -*-
 *
 * MiniZinc AST → Unified IR converter.
 *
 * Walks the MiniZinc AST and emits Unified IR nodes.
 * Operator resolution is registry-driven:
 *   BinaryOp::ADD  → registry.lookupByLangName("minizinc", "+")
 *   CallExpr("all_different", ...) → registry.lookupByLangName("minizinc", "all_different")
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef MZN_TO_UNIFIED_H
#define MZN_TO_UNIFIED_H

#include "somtparser/frontends/minizinc/mzn_ast.h"
#include "somtparser/unified/unified_ir.h"
#include "somtparser/unified/unified_op_registry.h"

namespace SOMTParser::MiniZinc {

/**
 * Convert a MiniZinc AST Model into a Unified IR Model.
 *
 * The registry must be loaded with ops before calling convert().
 */
class MznAstToUnifiedIR {
public:
    explicit MznAstToUnifiedIR(const Unified::UnifiedOpRegistry& registry)
        : registry_(registry) {}

    /** Convert entire MiniZinc model. */
    Unified::UnifiedModel convert(const Model& mzn_model) const;

    /** Convert a single MiniZinc expression. */
    Unified::ExprPtr convertExpr(const ExprPtr& expr) const;

    /** Convert a MiniZinc TypeInst to UnifiedType. */
    Unified::UnifiedType convertType(const std::shared_ptr<TypeInst>& type) const;

private:
    const Unified::UnifiedOpRegistry& registry_;

    // Expression converters
    Unified::ExprPtr convertLiteral(const ExprPtr& expr) const;
    Unified::ExprPtr convertIdent(const ExprPtr& expr) const;
    Unified::ExprPtr convertUnaryOp(const ExprPtr& expr) const;
    Unified::ExprPtr convertBinaryOp(const ExprPtr& expr) const;
    Unified::ExprPtr convertCall(const ExprPtr& expr) const;
    Unified::ExprPtr convertArrayAccess(const ExprPtr& expr) const;
    Unified::ExprPtr convertArrayLit(const ExprPtr& expr) const;
    Unified::ExprPtr convertSetLit(const ExprPtr& expr) const;
    Unified::ExprPtr convertIfThenElse(const ExprPtr& expr) const;
    Unified::ExprPtr convertLet(const ExprPtr& expr) const;
    Unified::ExprPtr convertAnnotated(const ExprPtr& expr) const;

    // Helpers
    Unified::UnifiedOpRef lookupOpByMznName(const std::string& name) const;
    std::string binaryOpToMznName(BinaryOp::Op op) const;
    std::string unaryOpToMznName(UnaryOp::Op op) const;
};

} // namespace SOMTParser::MiniZinc

#endif // MZN_TO_UNIFIED_H
