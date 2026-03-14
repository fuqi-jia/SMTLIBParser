/* -*- Source -*-
 *
 * An SMT/OMT Parser (Evaluation part)
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "somtparser/frontend/parser.h"
#include <stack>
#include <unordered_map>

namespace SOMTParser{

    void not_implemented_warning(const std::string& op){
        std::cerr << "Not implemented warning: " << op << " is not implemented" << std::endl;
    }
    std::shared_ptr<DAGNode> Parser::evaluate(std::shared_ptr<DAGNode> expr, const Model &model){
        std::shared_ptr<DAGNode> result = NodeManager::NULL_NODE;
        std::shared_ptr<Model> model_ptr = std::make_shared<Model>(model);
        evaluate(expr, model_ptr, result);
        return result;
    }

    std::shared_ptr<DAGNode> Parser::evaluate(std::shared_ptr<DAGNode> expr, const std::shared_ptr<Model> &model){
        std::shared_ptr<DAGNode> result = NodeManager::NULL_NODE;
        evaluate(expr, model, result);
        return result;
    }
    bool Parser::evaluate(std::shared_ptr<DAGNode> expr, const std::shared_ptr<Model> &model, std::shared_ptr<DAGNode> &result){
        if(model->isEmpty()){
            result = expr;
            return false;
        }
        
        // Use switch-case to avoid MSVC C1061 "blocks nested too deeply" error
        switch(expr->getKind()) {
        // Special cases with custom logic
        case NODE_KIND::NT_UNKNOWN:
            return false;
        case NODE_KIND::NT_ERROR:
            return false;
        case NODE_KIND::NT_CONST:
        case NODE_KIND::NT_CONST_TRUE:
        case NODE_KIND::NT_CONST_FALSE:
        case NODE_KIND::NT_CONST_PI:
        case NODE_KIND::NT_CONST_E:
        case NODE_KIND::NT_CONST_ARRAY:
            result = expr;
            return false;
        case NODE_KIND::NT_VAR:
        case NODE_KIND::NT_TEMP_VAR:
            result = model->get(expr);
            if(result->isUnknown()){
                result = expr;
                return false;
            }
            return true;
            
        // Boolean operators
        case NODE_KIND::NT_AND: return evaluateAnd(expr, model, result);
        case NODE_KIND::NT_OR: return evaluateOr(expr, model, result);
        case NODE_KIND::NT_NOT: return evaluateNot(expr, model, result);
        case NODE_KIND::NT_IMPLIES: return evaluateImpl(expr, model, result);
        case NODE_KIND::NT_XOR: return evaluateXor(expr, model, result);
        
        // Core operators
        case NODE_KIND::NT_EQ:
        case NODE_KIND::NT_EQ_BOOL:
        case NODE_KIND::NT_EQ_OTHER: return evaluateEq(expr, model, result);
        case NODE_KIND::NT_DISTINCT:
        case NODE_KIND::NT_DISTINCT_BOOL:
        case NODE_KIND::NT_DISTINCT_OTHER: return evaluateDistinct(expr, model, result);
        case NODE_KIND::NT_ITE: return evaluateIte(expr, model, result);
        
        // Arithmetic operators
        case NODE_KIND::NT_ADD: return evaluateAdd(expr, model, result);
        case NODE_KIND::NT_NEG: return evaluateNeg(expr, model, result);
        case NODE_KIND::NT_SUB: return evaluateSub(expr, model, result);
        case NODE_KIND::NT_MUL: return evaluateMul(expr, model, result);
        case NODE_KIND::NT_DIV_INT: return evaluateDivInt(expr, model, result);
        case NODE_KIND::NT_DIV_REAL: return evaluateDivReal(expr, model, result);
        case NODE_KIND::NT_MOD: return evaluateMod(expr, model, result);
        case NODE_KIND::NT_POW: return evaluatePow(expr, model, result);
        case NODE_KIND::NT_POW2: return evaluatePow2(expr, model, result);
        case NODE_KIND::NT_IAND: return evaluateIand(expr, model, result);
        case NODE_KIND::NT_ABS: return evaluateAbs(expr, model, result);
        case NODE_KIND::NT_SQRT: return evaluateSqrt(expr, model, result);
        case NODE_KIND::NT_SAFESQRT: return evaluateSafeSqrt(expr, model, result);
        case NODE_KIND::NT_CEIL: return evaluateCeil(expr, model, result);
        case NODE_KIND::NT_FLOOR: return evaluateFloor(expr, model, result);
        case NODE_KIND::NT_ROUND: return evaluateRound(expr, model, result);
        
        // Transcendental functions
        case NODE_KIND::NT_EXP: return evaluateExp(expr, model, result);
        case NODE_KIND::NT_LN: return evaluateLn(expr, model, result);
        case NODE_KIND::NT_LG: return evaluateLg(expr, model, result);
        case NODE_KIND::NT_LB: return evaluateLb(expr, model, result);
        case NODE_KIND::NT_LOG: return evaluateLog(expr, model, result);
        case NODE_KIND::NT_SIN: return evaluateSin(expr, model, result);
        case NODE_KIND::NT_COS: return evaluateCos(expr, model, result);
        case NODE_KIND::NT_TAN: return evaluateTan(expr, model, result);
        case NODE_KIND::NT_ASIN: return evaluateAsin(expr, model, result);
        case NODE_KIND::NT_ACOS: return evaluateAcos(expr, model, result);
        case NODE_KIND::NT_ATAN: return evaluateAtan(expr, model, result);
        case NODE_KIND::NT_SINH: return evaluateSinh(expr, model, result);
        case NODE_KIND::NT_COSH: return evaluateCosh(expr, model, result);
        case NODE_KIND::NT_TANH: return evaluateTanh(expr, model, result);
        case NODE_KIND::NT_ASINH: return evaluateAsinh(expr, model, result);
        case NODE_KIND::NT_ACOSH: return evaluateAcosh(expr, model, result);
        case NODE_KIND::NT_ATANH: return evaluateAtanh(expr, model, result);
        case NODE_KIND::NT_ASECH: return evaluateAsech(expr, model, result);
        case NODE_KIND::NT_ACSCH: return evaluateAcsch(expr, model, result);
        case NODE_KIND::NT_ACOTH: return evaluateAcoth(expr, model, result);
        case NODE_KIND::NT_ATAN2: return evaluateAtan2(expr, model, result);
        
        // Arithmetic comparison
        case NODE_KIND::NT_LE: return evaluateLe(expr, model, result);
        case NODE_KIND::NT_LT: return evaluateLt(expr, model, result);
        case NODE_KIND::NT_GE: return evaluateGe(expr, model, result);
        case NODE_KIND::NT_GT: return evaluateGt(expr, model, result);
        
        // Arithmetic conversion
        case NODE_KIND::NT_TO_REAL: return evaluateToReal(expr, model, result);
        case NODE_KIND::NT_TO_INT: return evaluateToInt(expr, model, result);
        
        // Arithmetic properties
        case NODE_KIND::NT_IS_INT: return evaluateIsInt(expr, model, result);
        case NODE_KIND::NT_IS_DIVISIBLE: return evaluateIsDivisible(expr, model, result);
        case NODE_KIND::NT_IS_PRIME: return evaluateIsPrime(expr, model, result);
        case NODE_KIND::NT_IS_EVEN: return evaluateIsEven(expr, model, result);
        case NODE_KIND::NT_IS_ODD: return evaluateIsOdd(expr, model, result);
        
        // Arithmetic functions
        case NODE_KIND::NT_GCD: return evaluateGcd(expr, model, result);
        case NODE_KIND::NT_LCM: return evaluateLcm(expr, model, result);
        case NODE_KIND::NT_FACT: return evaluateFact(expr, model, result);
        
        // Bitvector operators
        case NODE_KIND::NT_BV_NOT: return evaluateBvNot(expr, model, result);
        case NODE_KIND::NT_BV_NEG: return evaluateBvNeg(expr, model, result);
        case NODE_KIND::NT_BV_AND: return evaluateBvAnd(expr, model, result);
        case NODE_KIND::NT_BV_OR: return evaluateBvOr(expr, model, result);
        case NODE_KIND::NT_BV_XOR: return evaluateBvXor(expr, model, result);
        case NODE_KIND::NT_BV_NAND: return evaluateBvNand(expr, model, result);
        case NODE_KIND::NT_BV_NOR: return evaluateBvNor(expr, model, result);
        case NODE_KIND::NT_BV_XNOR: return evaluateBvXnor(expr, model, result);
        case NODE_KIND::NT_BV_COMP: return evaluateBvComp(expr, model, result);
        case NODE_KIND::NT_BV_ADD: return evaluateBvAdd(expr, model, result);
        case NODE_KIND::NT_BV_SUB: return evaluateBvSub(expr, model, result);
        case NODE_KIND::NT_BV_MUL: return evaluateBvMul(expr, model, result);
        case NODE_KIND::NT_BV_UDIV: return evaluateBvUdiv(expr, model, result);
        case NODE_KIND::NT_BV_UREM: return evaluateBvUrem(expr, model, result);
        case NODE_KIND::NT_BV_SDIV: return evaluateBvSdiv(expr, model, result);
        case NODE_KIND::NT_BV_SREM: return evaluateBvSrem(expr, model, result);
        case NODE_KIND::NT_BV_SMOD: return evaluateBvSmod(expr, model, result);
        case NODE_KIND::NT_BV_SHL: return evaluateBvShl(expr, model, result);
        case NODE_KIND::NT_BV_LSHR: return evaluateBvLshr(expr, model, result);
        case NODE_KIND::NT_BV_ASHR: return evaluateBvAshr(expr, model, result);
        case NODE_KIND::NT_BV_ULT: return evaluateBvUlt(expr, model, result);
        case NODE_KIND::NT_BV_ULE: return evaluateBvUle(expr, model, result);
        case NODE_KIND::NT_BV_UGT: return evaluateBvUgt(expr, model, result);
        case NODE_KIND::NT_BV_UGE: return evaluateBvUge(expr, model, result);
        case NODE_KIND::NT_BV_SLT: return evaluateBvSlt(expr, model, result);
        case NODE_KIND::NT_BV_SLE: return evaluateBvSle(expr, model, result);
        case NODE_KIND::NT_BV_SGT: return evaluateBvSgt(expr, model, result);
        case NODE_KIND::NT_BV_SGE: return evaluateBvSge(expr, model, result);
        case NODE_KIND::NT_BV_CONCAT: return evaluateBvConcat(expr, model, result);
        case NODE_KIND::NT_BV_TO_NAT: return evaluateBvToNat(expr, model, result);
        case NODE_KIND::NT_NAT_TO_BV: return evaluateBvNatToBv(expr, model, result);
        case NODE_KIND::NT_INT_TO_BV: return evaluateBvIntToBv(expr, model, result);
        case NODE_KIND::NT_BV_TO_INT: return evaluateBvToInt(expr, model, result);
        
        // Floating point operators
        case NODE_KIND::NT_FP_ABS: return evaluateFpAbs(expr, model, result);
        case NODE_KIND::NT_FP_NEG: return evaluateFpNeg(expr, model, result);
        case NODE_KIND::NT_FP_ADD: return evaluateFpAdd(expr, model, result);
        case NODE_KIND::NT_FP_SUB: return evaluateFpSub(expr, model, result);
        case NODE_KIND::NT_FP_MUL: return evaluateFpMul(expr, model, result);
        case NODE_KIND::NT_FP_DIV: return evaluateFpDiv(expr, model, result);
        case NODE_KIND::NT_FP_FMA: return evaluateFpFma(expr, model, result);
        case NODE_KIND::NT_FP_SQRT: return evaluateFpSqrt(expr, model, result);
        case NODE_KIND::NT_FP_REM: return evaluateFpRem(expr, model, result);
        case NODE_KIND::NT_FP_ROUND_TO_INTEGRAL: return evaluateFpRoundToIntegral(expr, model, result);
        case NODE_KIND::NT_FP_MIN: return evaluateFpMin(expr, model, result);
        case NODE_KIND::NT_FP_MAX: return evaluateFpMax(expr, model, result);
        case NODE_KIND::NT_FP_LE: return evaluateFpLe(expr, model, result);
        case NODE_KIND::NT_FP_LT: return evaluateFpLt(expr, model, result);
        case NODE_KIND::NT_FP_GE: return evaluateFpGe(expr, model, result);
        case NODE_KIND::NT_FP_GT: return evaluateFpGt(expr, model, result);
        case NODE_KIND::NT_FP_EQ: return evaluateFpEq(expr, model, result);
        case NODE_KIND::NT_FP_TO_UBV: return evaluateFpToUbv(expr, model, result);
        case NODE_KIND::NT_FP_TO_SBV: return evaluateFpToSbv(expr, model, result);
        case NODE_KIND::NT_FP_TO_REAL: return evaluateFpToReal(expr, model, result);
        case NODE_KIND::NT_FP_TO_FP:
        case NODE_KIND::NT_FP_TO_FP_UNSIGNED: return evaluateToFp(expr, model, result);
        case NODE_KIND::NT_FP_IS_NORMAL: return evaluateFpIsNormal(expr, model, result);
        case NODE_KIND::NT_FP_IS_SUBNORMAL: return evaluateFpIsSubnormal(expr, model, result);
        case NODE_KIND::NT_FP_IS_ZERO: return evaluateFpIsZero(expr, model, result);
        case NODE_KIND::NT_FP_IS_INF: return evaluateFpIsInf(expr, model, result);
        case NODE_KIND::NT_FP_IS_NAN: return evaluateFpIsNaN(expr, model, result);
        case NODE_KIND::NT_FP_IS_NEG: return evaluateFpIsNeg(expr, model, result);
        case NODE_KIND::NT_FP_IS_POS: return evaluateFpIsPos(expr, model, result);
        
        // Array operators
        case NODE_KIND::NT_SELECT: return evaluateSelect(expr, model, result);
        case NODE_KIND::NT_STORE: return evaluateStore(expr, model, result);
        
        // String operators
        case NODE_KIND::NT_STR_LEN: return evaluateStrLen(expr, model, result);
        case NODE_KIND::NT_STR_CONCAT: return evaluateStrConcat(expr, model, result);
        case NODE_KIND::NT_STR_SUBSTR: return evaluateStrSubstr(expr, model, result);
        case NODE_KIND::NT_STR_PREFIXOF: return evaluateStrPrefixof(expr, model, result);
        case NODE_KIND::NT_STR_SUFFIXOF: return evaluateStrSuffixof(expr, model, result);
        case NODE_KIND::NT_STR_INDEXOF: return evaluateStrIndexof(expr, model, result);
        case NODE_KIND::NT_STR_CHARAT: return evaluateStrCharat(expr, model, result);
        case NODE_KIND::NT_STR_UPDATE: return evaluateStrUpdate(expr, model, result);
        case NODE_KIND::NT_STR_REPLACE: return evaluateStrReplace(expr, model, result);
        case NODE_KIND::NT_STR_REPLACE_ALL: return evaluateStrReplaceAll(expr, model, result);
        case NODE_KIND::NT_STR_TO_LOWER: return evaluateStrToLower(expr, model, result);
        case NODE_KIND::NT_STR_TO_UPPER: return evaluateStrToUpper(expr, model, result);
        case NODE_KIND::NT_STR_REV: return evaluateStrRev(expr, model, result);
        case NODE_KIND::NT_STR_SPLIT: return evaluateStrSplit(expr, model, result);
        case NODE_KIND::NT_STR_SPLIT_REST: return evaluateStrSplitRest(expr, model, result);
        case NODE_KIND::NT_STR_SPLIT_AT_RE: return evaluateStrSplitAtRe(expr, model, result);
        case NODE_KIND::NT_STR_SPLIT_REST_RE: return evaluateStrSplitRestRe(expr, model, result);
        case NODE_KIND::NT_STR_NUM_SPLITS_RE: return evaluateStrNumSplitsRe(expr, model, result);
        case NODE_KIND::NT_STR_LT: return evaluateStrLt(expr, model, result);
        case NODE_KIND::NT_STR_LE: return evaluateStrLe(expr, model, result);
        case NODE_KIND::NT_STR_GT: return evaluateStrGt(expr, model, result);
        case NODE_KIND::NT_STR_GE: return evaluateStrGe(expr, model, result);
        case NODE_KIND::NT_STR_IN_REG: return evaluateStrInReg(expr, model, result);
        case NODE_KIND::NT_STR_CONTAINS: return evaluateStrContains(expr, model, result);
        case NODE_KIND::NT_STR_IS_DIGIT: return evaluateStrIsDigit(expr, model, result);
        case NODE_KIND::NT_STR_FROM_INT: return evaluateStrFromInt(expr, model, result);
        case NODE_KIND::NT_STR_TO_INT: return evaluateStrToInt(expr, model, result);
        case NODE_KIND::NT_STR_TO_REG: return evaluateStrToReg(expr, model, result);
        case NODE_KIND::NT_STR_TO_CODE: return evaluateStrToCode(expr, model, result);
        case NODE_KIND::NT_STR_FROM_CODE: return evaluateStrFromCode(expr, model, result);
        
        // Regular expression operators
        case NODE_KIND::NT_REG_CONCAT: return evaluateRegConcat(expr, model, result);
        case NODE_KIND::NT_REG_UNION: return evaluateRegUnion(expr, model, result);
        case NODE_KIND::NT_REG_INTER: return evaluateRegInter(expr, model, result);
        case NODE_KIND::NT_REG_DIFF: return evaluateRegDiff(expr, model, result);
        case NODE_KIND::NT_REG_STAR: return evaluateRegStar(expr, model, result);
        case NODE_KIND::NT_REG_PLUS: return evaluateRegPlus(expr, model, result);
        case NODE_KIND::NT_REG_OPT: return evaluateRegOpt(expr, model, result);
        case NODE_KIND::NT_REG_RANGE: return evaluateRegRange(expr, model, result);
        case NODE_KIND::NT_REG_REPEAT: return evaluateRegRepeat(expr, model, result);
        case NODE_KIND::NT_REG_COMPLEMENT: return evaluateRegComplement(expr, model, result);
        
        // Function operators
        case NODE_KIND::NT_FUNC_APPLY:
        case NODE_KIND::NT_UF_APPLY: return evaluateApplyFun(expr, model, result);
        
        // Let expressions
        case NODE_KIND::NT_LET:
        case NODE_KIND::NT_LET_CHAIN: return evaluateLet(expr, model, result);
        
        // Default case
        default:
            result = expr;
            return false;
        }
    }

    bool Parser::evaluateSimpleOp(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result, NODE_KIND op){
        bool changed = false;
        switch(op){
            // unary operations
            case NODE_KIND::NT_NOT:
            case NODE_KIND::NT_NEG:
            case NODE_KIND::NT_ABS:
            case NODE_KIND::NT_SQRT:
            case NODE_KIND::NT_SAFESQRT:
            case NODE_KIND::NT_CEIL:
            case NODE_KIND::NT_FLOOR:
            case NODE_KIND::NT_ROUND:
            case NODE_KIND::NT_EXP:
            case NODE_KIND::NT_POW2:
            case NODE_KIND::NT_LN:
            case NODE_KIND::NT_LG:
            case NODE_KIND::NT_LB:
            case NODE_KIND::NT_SIN:
            case NODE_KIND::NT_COS:
            case NODE_KIND::NT_TAN:
            case NODE_KIND::NT_COT:
            case NODE_KIND::NT_CSC:
            case NODE_KIND::NT_SEC:
            case NODE_KIND::NT_ASIN:
            case NODE_KIND::NT_ACOS:
            case NODE_KIND::NT_ATAN:
            case NODE_KIND::NT_ASEC:
            case NODE_KIND::NT_ACSC:
            case NODE_KIND::NT_ACOT:
            case NODE_KIND::NT_SINH:
            case NODE_KIND::NT_COSH:
            case NODE_KIND::NT_TANH:
            case NODE_KIND::NT_SECH:
            case NODE_KIND::NT_CSCH:
            case NODE_KIND::NT_COTH:
            case NODE_KIND::NT_ASINH:
            case NODE_KIND::NT_ACOSH:
            case NODE_KIND::NT_ATANH:
            case NODE_KIND::NT_ACOTH:
            case NODE_KIND::NT_ASECH:
            case NODE_KIND::NT_ACSCH:
            case NODE_KIND::NT_TO_REAL:
            case NODE_KIND::NT_TO_INT:
            case NODE_KIND::NT_IS_INT:
            case NODE_KIND::NT_IS_DIVISIBLE:
            case NODE_KIND::NT_IS_PRIME:
            case NODE_KIND::NT_IS_EVEN:
            case NODE_KIND::NT_IS_ODD:
            case NODE_KIND::NT_FACT:
            case NODE_KIND::NT_BV_NOT:
            case NODE_KIND::NT_BV_NEG:
            case NODE_KIND::NT_BV_TO_NAT:
            case NODE_KIND::NT_BV_TO_INT:
            case NODE_KIND::NT_STR_LEN:
            case NODE_KIND::NT_STR_TO_LOWER:
            case NODE_KIND::NT_STR_TO_UPPER:
            case NODE_KIND::NT_STR_REV:
            case NODE_KIND::NT_STR_IS_DIGIT:
            case NODE_KIND::NT_STR_FROM_INT:
            case NODE_KIND::NT_STR_TO_INT:
            case NODE_KIND::NT_STR_TO_REG:
            case NODE_KIND::NT_STR_TO_CODE:
            case NODE_KIND::NT_STR_FROM_CODE:
            {
                std::shared_ptr<DAGNode> child = NodeManager::NULL_NODE;
                changed |= evaluate(expr->getChildren()[0], model, child);
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                result = mkOper(expr->getSort(), op, child);
                return true;
            }
            // binary operations
            case NODE_KIND::NT_IMPLIES:
            case NODE_KIND::NT_MOD:
            case NODE_KIND::NT_LOG:
            case NODE_KIND::NT_POW:
            case NODE_KIND::NT_ATAN2:
            case NODE_KIND::NT_LE:
            case NODE_KIND::NT_LT:
            case NODE_KIND::NT_GE:
            case NODE_KIND::NT_GT:
            case NODE_KIND::NT_GCD:
            case NODE_KIND::NT_LCM:
            case NODE_KIND::NT_BV_COMP:
            case NODE_KIND::NT_BV_UREM:
            case NODE_KIND::NT_BV_SREM:
            case NODE_KIND::NT_BV_UMOD:
            case NODE_KIND::NT_BV_SMOD:
            case NODE_KIND::NT_BV_SHL:
            case NODE_KIND::NT_BV_LSHR:
            case NODE_KIND::NT_BV_ASHR:
            case NODE_KIND::NT_BV_ULT:
            case NODE_KIND::NT_BV_ULE:
            case NODE_KIND::NT_BV_UGT:
            case NODE_KIND::NT_BV_UGE:
            case NODE_KIND::NT_BV_SLT:
            case NODE_KIND::NT_BV_SGT:
            case NODE_KIND::NT_BV_SLE:
            case NODE_KIND::NT_BV_SGE:
            case NODE_KIND::NT_NAT_TO_BV:
            case NODE_KIND::NT_INT_TO_BV:
            case NODE_KIND::NT_STR_PREFIXOF:
            case NODE_KIND::NT_STR_SUFFIXOF:
            case NODE_KIND::NT_STR_CONTAINS:
            case NODE_KIND::NT_STR_CHARAT:
            case NODE_KIND::NT_STR_LT: // TODO: lt/le/gt/ge now is binary operation, but it should be n-ary operation
            case NODE_KIND::NT_STR_LE: // TODO: lt/le/gt/ge now is binary operation, but it should be n-ary operation
            case NODE_KIND::NT_STR_GT: // TODO: lt/le/gt/ge now is binary operation, but it should be n-ary operation
            case NODE_KIND::NT_STR_GE: // TODO: lt/le/gt/ge now is binary operation, but it should be n-ary operation
            case NODE_KIND::NT_STR_IN_REG:
            {
                std::shared_ptr<DAGNode> l = NodeManager::NULL_NODE;
                std::shared_ptr<DAGNode> r = NodeManager::NULL_NODE;
                changed |= evaluate(expr->getChildren()[0], model, l);
                changed |= evaluate(expr->getChildren()[1], model, r);
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                result = mkOper(expr->getSort(), op, l, r);
                return true;
            }
            // triple operations
            case NODE_KIND::NT_STR_SUBSTR:
            case NODE_KIND::NT_STR_INDEXOF:
            case NODE_KIND::NT_STR_UPDATE:
            case NODE_KIND::NT_STR_REPLACE:
            case NODE_KIND::NT_STR_REPLACE_ALL:
            {
                bool changed = false;
                std::shared_ptr<DAGNode> l = NodeManager::NULL_NODE;
                std::shared_ptr<DAGNode> r = NodeManager::NULL_NODE;
                std::shared_ptr<DAGNode> s = NodeManager::NULL_NODE;
                changed |= evaluate(expr->getChildren()[0], model, l);
                changed |= evaluate(expr->getChildren()[1], model, r);
                changed |= evaluate(expr->getChildren()[2], model, s);
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                result = mkOper(expr->getSort(), op, {l, r, s});
                return true;
            }
            // simplify using binary operations
            case NODE_KIND::NT_IAND:
            case NODE_KIND::NT_ADD:
            case NODE_KIND::NT_MUL:
            case NODE_KIND::NT_BV_AND:
            case NODE_KIND::NT_BV_OR:
            case NODE_KIND::NT_BV_XOR:
            case NODE_KIND::NT_BV_NAND:
            case NODE_KIND::NT_BV_NOR:
            case NODE_KIND::NT_BV_XNOR:
            case NODE_KIND::NT_BV_ADD:
            case NODE_KIND::NT_BV_MUL:
            case NODE_KIND::NT_MAX:
            case NODE_KIND::NT_MIN:
            {
                changed = false;
                std::vector<std::shared_ptr<DAGNode>> children;
                for(auto child : expr->getChildren()){
                    std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
                    changed |= evaluate(child, model, eval);
                    children.emplace_back(eval);
                }
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                condAssert(!children.empty(), "evaluateSimpleOp: children is empty");
                // compute the sum of the children that are constant
                std::vector<std::shared_ptr<DAGNode>> const_children;
                std::vector<std::shared_ptr<DAGNode>> non_const_children;
                for(auto child : children){
                    if(child->isConst()){
                        const_children.emplace_back(child);
                    }
                    else{
                        non_const_children.emplace_back(child);
                    }
                }
                if(const_children.empty()){
                    if(non_const_children.size() == 1){
                        result = non_const_children[0];
                    }
                    else{
                        result = mkOper(expr->getSort(), op, non_const_children);
                    }
                }
                else{
                    // compute the and of the constant children
                    result = const_children[0];
                    for(size_t i = 1; i < const_children.size(); ++i){
                        result = mkOper(expr->getSort(), op, result, const_children[i]);
                    }
                    non_const_children.emplace_back(result);
                    if(non_const_children.size() == 1){
                        result = non_const_children[0];
                    }
                    else{
                        result = mkOper(expr->getSort(), op, non_const_children);
                    }
                }
                return true; 
            }
            // simplify using binary operations but the first child is special
            case NODE_KIND::NT_SUB:
            case NODE_KIND::NT_DIV_REAL:
            case NODE_KIND::NT_BV_SUB:
            case NODE_KIND::NT_BV_UDIV:
            case NODE_KIND::NT_BV_SDIV:
            case NODE_KIND::NT_FP_SUB:
            {
                changed = false;
                std::vector<std::shared_ptr<DAGNode>> children;
                for(auto child : expr->getChildren()){
                    std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
                    changed |= evaluate(child, model, eval);
                    children.emplace_back(eval);
                }
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                condAssert(!children.empty(), "evaluateSimpleOp: children is empty");
                // compute the difference of the children that are constant
                bool first_child_is_const = children[0]->isConst();
                std::vector<std::shared_ptr<DAGNode>> const_children;
                std::vector<std::shared_ptr<DAGNode>> non_const_children;
                for(auto child : children){
                    if(child->isConst()){
                        const_children.emplace_back(child);
                    }
                    else{
                        non_const_children.emplace_back(child);
                    }
                }
                if(const_children.empty()){
                    if(non_const_children.size() == 1){
                        result = non_const_children[0];
                    }
                    else{
                        result = mkOper(expr->getSort(), op, non_const_children);
                    }
                }
                else{
                    if(first_child_is_const){
                        result = const_children[0];
                        for(size_t i = 1; i < const_children.size(); ++i){
                            result = mkOper(expr->getSort(), op, result, const_children[i]);
                        }
                        non_const_children.insert(non_const_children.begin(), result);
                        if(non_const_children.size() == 1){
                            result = non_const_children[0];
                        }
                        else{
                            result = mkOper(expr->getSort(), op, non_const_children);
                        }
                    }
                    else{
                        auto opposite_op = getFlipKind(op);
                        result = const_children[0];
                        for(size_t i = 1; i < const_children.size(); ++i){
                            result = mkOper(expr->getSort(), opposite_op, result, const_children[i]);
                        }
                        non_const_children.emplace_back(result);
                        if(non_const_children.size() == 1){
                            result = non_const_children[0];
                        }
                        else{
                            result = mkOper(expr->getSort(), op, non_const_children);
                        }
                    }
                }
                return true;
            }
            // concat
            case NODE_KIND::NT_BV_CONCAT:
            case NODE_KIND::NT_STR_CONCAT:
            {
                changed = false;
                // sequential evaluation
                std::vector<std::shared_ptr<DAGNode>> children;
                size_t i = 0;
                while(i < expr->getChildren().size()){
                    // concat until the last constant child
                    std::shared_ptr<DAGNode> child = NodeManager::NULL_NODE;
                    changed |= evaluate(expr->getChildren()[i], model, child);
                    if(child->isConst()){
                        // go on until the child is not constant
                        std::shared_ptr<DAGNode> child_ = NodeManager::NULL_NODE;
                        while(i < expr->getChildren().size()){
                            changed |= evaluate(expr->getChildren()[i], model, child_);
                            if(!child_->isConst()) break;
                            child = mkOper(expr->getSort(), op, child, child_);
                            i++;
                        }
                        if(i == expr->getChildren().size()){
                            // all remaining children are constant
                            children.emplace_back(child);
                            break;
                        }
                        else if(child_->isNull()){
                            // child_ is null -> only child is constant
                            children.emplace_back(child);
                        }
                        else{
                            condAssert(!child->isConst(), "evaluateSimpleOp: child is constant");
                            children.emplace_back(child);
                        }
                    }
                    else{
                        children.emplace_back(child);
                    }
                    i++;
                }
                if(!changed){
                    result = expr;
                    return false;
                }
                condAssert(changed, "evaluateSimpleOp: changed is false");
                condAssert(!children.empty(), "evaluateSimpleOp: children is empty");
                if(children.size() == 1){
                    result = children.back();
                    return false;
                }
                else{
                    result = mkOper(expr->getSort(), op, children);
                }
                return true;
            }
            default:
                condAssert(false, "evaluateSimpleOp: no implementation for this kind");
                result = expr;
                return false;
        }

        return changed;
    }
    bool Parser::evaluateAnd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        std::vector<std::shared_ptr<DAGNode>> children;
        bool changed = false;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, eval);
            if(eval->isConst()){
                if(eval->isFalse()){
                    result = eval;
                    return true; // result has been changed
                }
            }
            else{
                children.emplace_back(eval);
            }
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateAnd: changed is false");
        if(children.empty()){
            result = mkTrue();
        }
        else if(children.size() == 1){
            result = children[0];
        }
        else{
            result = mkAnd(children);
        }
        return true;
    }
    bool Parser::evaluateOr(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        std::vector<std::shared_ptr<DAGNode>> children;
        bool changed = false;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, eval);
            if(eval->isConst()){
                if(eval->isTrue()){
                    result = eval;
                    return true;
                }
            }
            else{
                children.emplace_back(eval);
            }
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateOr: changed is false");
        if(children.empty()){
            result = mkFalse();
        }
        else if(children.size() == 1){
            result = children[0];
        }
        else{
            result = mkOr(children);
        }
        return true;
    }
    bool Parser::evaluateNot(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_NOT);
    }
    bool Parser::evaluateImpl(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IMPLIES);
    }
    bool Parser::evaluateXor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        bool changed = false;
        // count true
        size_t trueCount = 0;
        // save unevaluated children
        std::vector<std::shared_ptr<DAGNode>> remainingChildren;
        
        // traverse all children
        for (auto child : expr->getChildren()) {
            std::shared_ptr<DAGNode> evaluatedChild = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, evaluatedChild);
            
            // evaluated as constant
            if (evaluatedChild->isConst()) {
                if (evaluatedChild->isTrue()) {
                    trueCount++;
                }
                // ignore false
            } else {
                // save unevaluated children
                remainingChildren.emplace_back(evaluatedChild);
            }
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateXor: changed is false");
        // all children are constants
        if (remainingChildren.empty()) {
            // result depends on true count is odd or even
            result = (trueCount % 2 == 1) ? mkTrue() : mkFalse();
            return true;
        }
        
        // only one unevaluated child
        if (remainingChildren.size() == 1) {
            // if true count is even, result is the child
            // if true count is odd, result is the negation of the child
            result = (trueCount % 2 == 0) ? 
                   remainingChildren[0] : 
                   mkNot(remainingChildren[0]);
            return true;
        }
        
        // multiple unevaluated children
        // decide return XOR or its negation based on true count
        result = (trueCount % 2 == 0) ? 
                   mkXor(remainingChildren) : 
                   mkNot(mkXor(remainingChildren));
        return true;
    }
    bool Parser::evaluateEq(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        bool changed = false;
        std::vector<std::shared_ptr<DAGNode>> children;
        std::vector<std::shared_ptr<DAGNode>> const_vals;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, eval);
            if(eval->isConst()){
                const_vals.emplace_back(eval);
            }
            else{
                children.emplace_back(eval);
            }
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateEq: changed is false");
        if(const_vals.empty()){
            result = mkEq(children);
            return true;
        }
        condAssert(!const_vals.empty(), "evaluateEq: const_vals is empty");
        auto const_val = const_vals[0];
        for(size_t i = 1; i < const_vals.size(); ++i){
            if(const_val->isCInt() && const_vals[i]->isCInt()){
                if(toInt(const_val) != toInt(const_vals[i])){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isCReal() && const_vals[i]->isCReal()){
                if(toReal(const_val) != toReal(const_vals[i])){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isCBool() && const_vals[i]->isCBool()){
                if(const_val->isTrue() != const_vals[i]->isTrue()){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isCStr() && const_vals[i]->isCStr()){
                if(const_val->toString() != const_vals[i]->toString()){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isCBV() && const_vals[i]->isCBV()){
                if(const_val->toString() != const_vals[i]->toString()){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isCFP() && const_vals[i]->isCFP()){
                if(const_val->toString() != const_vals[i]->toString()){
                    result = mkFalse();
                    return true;
                }
            }
            else if(const_val->isArray() && const_vals[i]->isArray()){
                // Use canonical form to check array equality
                if(!areArraysEqual(const_val, const_vals[i])){
                    result = mkFalse();
                    return true;
                }
            }
            else{
                condAssert(false, "evaluateEq: const_val is not a constant");
            }
        }
        if(children.size() == 0){
            result = mkTrue();
            return true;
        }
        else if(children.size() == 1){
            children.emplace_back(const_val);
            result = mkEq(children);
        }
        else{
            if(const_val->isNull()){
                result = mkEq(children);
            }
            else{
                children.emplace_back(const_val);
                result = mkEq(children);
            }
        }
        return true;
    }
    bool Parser::evaluateDistinct(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        bool changed = false;
        std::vector<std::shared_ptr<DAGNode>> children;
        std::unordered_set<std::shared_ptr<DAGNode>> const_vals;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, eval);
            if(eval->isConst()){
                if(const_vals.empty()){
                    const_vals.insert(eval);
                }
                else if(const_vals.find(eval) == const_vals.end()){
                    const_vals.insert(eval);
                }
                else{
                    result = mkFalse();
                    return true;
                }
            }
            else{
                children.emplace_back(eval);
            }
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateDistinct: changed is false");
        if(const_vals.empty()){
            result = mkDistinct(children);
            return true;
        }
        condAssert(!const_vals.empty(), "evaluateDistinct: const_vals is empty");
        if(children.empty()){
            result = mkTrue();
        }
        else{
            children.insert(children.end(), const_vals.begin(), const_vals.end());
            result = mkDistinct(children);
        }
        return true;
    }
    bool Parser::evaluateIte(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        bool changed = false;
        std::shared_ptr<DAGNode> cond = NodeManager::NULL_NODE;
        std::shared_ptr<DAGNode> then_child = NodeManager::NULL_NODE;
        std::shared_ptr<DAGNode> else_child = NodeManager::NULL_NODE;
        changed |= evaluate(expr->getChild(0), model, cond);
        changed |= evaluate(expr->getChild(1), model, then_child);
        changed |= evaluate(expr->getChild(2), model, else_child);
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateIte: changed is false");
        if(cond->isConst()){
            result = cond->isTrue() ? then_child : else_child;
        }
        else{
            result = mkIte(cond, then_child, else_child);
        }
        return true;
    }
    bool Parser::evaluateAdd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ADD);
    }
    bool Parser::evaluateNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_NEG);
    }
    bool Parser::evaluateSub(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SUB);
	}
    bool Parser::evaluateMul(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_MUL);
	}
    bool Parser::evaluateDivInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        bool changed = false;
        std::vector<std::shared_ptr<DAGNode>> children;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, eval);
            children.emplace_back(eval);
        }
        if(!changed){
            result = expr;
            return false;
        }
        condAssert(changed, "evaluateDivInt: changed is false");
        condAssert(!children.empty(), "evaluateDivInt: children is empty");
        // compute the quotient of the children that are constant
        bool first_child_is_const = children[0]->isConst();
        std::vector<std::shared_ptr<DAGNode>> const_children;
        std::vector<std::shared_ptr<DAGNode>> non_const_children;
        for(auto child : children){
            if(child->isConst()){
                const_children.emplace_back(child);
            }
            else{
                non_const_children.emplace_back(child);
            }
        }
        if(const_children.empty()){
            result = mkDivInt(non_const_children);
        }
        else{
            if(first_child_is_const){
                auto res = mkConstInt(1);
                for(size_t i = 1; i < const_children.size(); ++i){
                    res = mkMul({res, const_children[i]});
                }
                if(toInt(res) > toInt(const_children[0])){
                    // 1 / 3 is 0 in integer arithmetic
                    result = mkConstInt(0);
                    return true;
                }
                else{
                    // 5 / 3 is 1 in integer arithmetic
                    auto div = mkDivInt({const_children[0], res});
                    non_const_children.insert(non_const_children.begin(), div);
                }
            }
            else{
                auto res = mkConstInt(1);
                for(size_t i = 0; i < const_children.size(); ++i){
                    res = mkMul({res, const_children[i]});
                }
                non_const_children.emplace_back(res);
                result = mkDivInt(non_const_children);
            }
        }
        return true;
	}
    bool Parser::evaluateDivReal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_DIV_REAL);
	}   
    bool Parser::evaluateMod(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_MOD);
	}
    bool Parser::evaluatePow(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_POW);
    }
    bool Parser::evaluatePow2(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_POW2);
	}
    bool Parser::evaluateIand(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IAND);
    }
    bool Parser::evaluateAbs(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ABS);
	}
    bool Parser::evaluateSqrt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SQRT);
	}
    bool Parser::evaluateSafeSqrt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SAFESQRT);
	}
    bool Parser::evaluateCeil(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_CEIL);
	}
    bool Parser::evaluateFloor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_FLOOR);
	}
    bool Parser::evaluateRound(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ROUND);
	}
    bool Parser::evaluateExp(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_EXP);
	}
    bool Parser::evaluateLn(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LN);
	}
    bool Parser::evaluateLg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LG);
	}
    bool Parser::evaluateLog(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LOG);
	}
    bool Parser::evaluateLb(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LB);
	}
    bool Parser::evaluateSin(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SIN);
	}
    bool Parser::evaluateCos(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_COS);
	}
    bool Parser::evaluateTan(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_TAN);
	}
    bool Parser::evaluateCot(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_COT);
	}
    bool Parser::evaluateCsc(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_CSC);
	}
    bool Parser::evaluateSec(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SEC);
	}
    
    bool Parser::evaluateAsin(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ASIN);
	}
    bool Parser::evaluateAcos(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACOS);
	}
    bool Parser::evaluateAtan(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ATAN);
	}
    bool Parser::evaluateAsec(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ASEC);
	}
    bool Parser::evaluateAcsc(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACSC);
	}
    bool Parser::evaluateAcot(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACOT);
	}
    
    bool Parser::evaluateSinh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SINH);
	}
    bool Parser::evaluateCosh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_COSH);
	}
    bool Parser::evaluateTanh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_TANH);
	}
    bool Parser::evaluateSech(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_SECH);
	}
    bool Parser::evaluateCsch(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_CSCH);
	}
    bool Parser::evaluateCoth(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_COTH);
	}

    bool Parser::evaluateAsinh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ASINH);
	}
    bool Parser::evaluateAcosh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACOSH);
	}
    bool Parser::evaluateAtanh(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ATANH);
	}
    bool Parser::evaluateAsech(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ASECH);
	}
    bool Parser::evaluateAcsch(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACSCH);
	}
    bool Parser::evaluateAcoth(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ACOTH);
	}

    bool Parser::evaluateAtan2(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_ATAN2);
	}
    bool Parser::evaluateLe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LE);
	}
    bool Parser::evaluateLt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LT);
	}
    bool Parser::evaluateGe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_GE);
	}
    bool Parser::evaluateGt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_GT);
	}
    bool Parser::evaluateToReal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_TO_REAL);
	}
    bool Parser::evaluateToInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_TO_INT);
	}
    bool Parser::evaluateIsInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IS_INT);
	}
    bool Parser::evaluateIsDivisible(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IS_DIVISIBLE);
	}
    bool Parser::evaluateIsPrime(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IS_PRIME);
	}
    bool Parser::evaluateIsEven(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IS_EVEN);
	}
    bool Parser::evaluateIsOdd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_IS_ODD);
	}
    bool Parser::evaluateGcd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_GCD);
	}
    bool Parser::evaluateLcm(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_LCM);
	}
    bool Parser::evaluateFact(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_FACT);
	}
    bool Parser::evaluateBvNot(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_NOT);
	}
    bool Parser::evaluateBvNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_NEG);
	}
    bool Parser::evaluateBvAnd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_AND);
	}
    bool Parser::evaluateBvOr(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_OR);
	}
    bool Parser::evaluateBvXor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_XOR);
	}
    bool Parser::evaluateBvNand(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_NAND);
	}
    bool Parser::evaluateBvNor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_NOR);
	}
    bool Parser::evaluateBvXnor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_XNOR);
	}
    bool Parser::evaluateBvComp(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_COMP);
	}
    bool Parser::evaluateBvAdd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_ADD);
	}
    bool Parser::evaluateBvSub(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SUB);
	}
    bool Parser::evaluateBvMul(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_MUL);
	}
    bool Parser::evaluateBvUdiv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_UDIV);
	}
    bool Parser::evaluateBvUrem(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_UREM);
	}
    bool Parser::evaluateBvSdiv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SDIV);
	}
    bool Parser::evaluateBvSrem(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SREM);
	}
    bool Parser::evaluateBvSmod(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SMOD);
	}
    bool Parser::evaluateBvShl(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SHL);
	}
    bool Parser::evaluateBvLshr(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_LSHR);
	}
    bool Parser::evaluateBvAshr(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_ASHR);
	}
    bool Parser::evaluateBvUlt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_ULT);
	}
    bool Parser::evaluateBvUle(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_ULE);
	}
    bool Parser::evaluateBvUgt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_UGT);
	}
    bool Parser::evaluateBvUge(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_UGE);
	}
    bool Parser::evaluateBvSlt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SLT);
	}
    bool Parser::evaluateBvSle(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SLE);
	}
    bool Parser::evaluateBvSgt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SGT);
	}
    bool Parser::evaluateBvSge(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_SGE);
	}
    bool Parser::evaluateBvConcat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_CONCAT);
    }
    bool Parser::evaluateBvToNat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_TO_NAT);
	}
    bool Parser::evaluateBvNatToBv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_NAT_TO_BV);
	}
    bool Parser::evaluateBvIntToBv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_BV_TO_INT);
	}
    bool Parser::evaluateBvToInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_NAT_TO_BV);
	}
    bool Parser::evaluateFpAbs(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.abs");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.neg");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpAdd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.add");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpSub(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.sub");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpMul(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.mul");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpDiv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.div");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpFma(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.fma");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpSqrt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.sqrt");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpRem(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.rem");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpRoundToIntegral(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.roundToIntegral");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpMin(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.min");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpMax(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.max");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpLe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.leq");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpLt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.lt");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpGe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.geq");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpGt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.gt");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpEq(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.eq");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpToUbv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.to_ubv");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpToSbv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.to_sbv");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpToReal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.to_real");
        result = expr;
        return false;
	}   
    bool Parser::evaluateToFp(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("to_fp");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsNormal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isNormal");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsSubnormal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isSubnormal");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsZero(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isZero");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsInf(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isInfinite");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsNaN(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isNaN");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isNegative");
        result = expr;
        return false;
	}
    bool Parser::evaluateFpIsPos(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("fp.isPositive");
        result = expr;
        return false;
	}
    bool Parser::evaluateSelect(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        // Evaluate array and index first (this will handle variable substitution)
        std::shared_ptr<DAGNode> array = expr->getSelectArray();
        std::shared_ptr<DAGNode> index = expr->getSelectIndex();
        
        std::shared_ptr<DAGNode> eval_array;
        std::shared_ptr<DAGNode> eval_index;
        bool array_changed = evaluate(array, model, eval_array);
        bool index_changed = evaluate(index, model, eval_index);
        
        // Apply array simplification to the evaluated select
        if (array_changed || index_changed) {
            // Use mkOper directly to create select, then simplify
            std::shared_ptr<DAGNode> new_select = mkOper(eval_array->getSort()->getElemSort(), NODE_KIND::NT_SELECT, eval_array, eval_index);
            std::shared_ptr<DAGNode> simplified = simplifyArray(new_select);
            result = simplified;
            return true;
        }
        
        // If nothing changed, try to simplify the original select
        std::shared_ptr<DAGNode> simplified = simplifyArray(expr);
        if (simplified != expr) {
            result = simplified;
            return true;
        }
        
        result = expr;
        return false;
	}
    bool Parser::evaluateStore(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        // Evaluate array, index, and value first (this will handle variable substitution)
        std::shared_ptr<DAGNode> array = expr->getStoreArray();
        std::shared_ptr<DAGNode> index = expr->getStoreIndex();
        std::shared_ptr<DAGNode> value = expr->getStoreValue();
        
        std::shared_ptr<DAGNode> eval_array;
        std::shared_ptr<DAGNode> eval_index;
        std::shared_ptr<DAGNode> eval_value;
        bool array_changed = evaluate(array, model, eval_array);
        bool index_changed = evaluate(index, model, eval_index);
        bool value_changed = evaluate(value, model, eval_value);
        
        // Apply array simplification to the evaluated store
        if (array_changed || index_changed || value_changed) {
            // Use mkOper directly to create store, then simplify
            std::shared_ptr<DAGNode> new_store = mkOper(eval_array->getSort(), NODE_KIND::NT_STORE, eval_array, eval_index, eval_value);
            std::shared_ptr<DAGNode> simplified = simplifyArray(new_store);
            result = simplified;
            return true;
        }
        
        // If nothing changed, try to simplify the original store
        std::shared_ptr<DAGNode> simplified = simplifyArray(expr);
        if (simplified != expr) {
            result = simplified;
            return true;
        }
        
        result = expr;
        return false;
	}
    bool Parser::evaluateStrLen(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_LEN);
    }
    bool Parser::evaluateStrConcat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_CONCAT);
    }
    bool Parser::evaluateStrSubstr(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_SUBSTR);
	}
    bool Parser::evaluateStrPrefixof(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_PREFIXOF);
	}
    bool Parser::evaluateStrSuffixof(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_SUFFIXOF);
	}
    bool Parser::evaluateStrIndexof(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_INDEXOF);
	}
    bool Parser::evaluateStrCharat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_CHARAT);
	}
    bool Parser::evaluateStrUpdate(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_UPDATE);
	}
    bool Parser::evaluateStrReplace(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_REPLACE);
	}
    bool Parser::evaluateStrReplaceAll(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_REPLACE_ALL);
	}
    bool Parser::evaluateStrToLower(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_TO_LOWER);
	}
    bool Parser::evaluateStrToUpper(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_TO_UPPER);
	}
    bool Parser::evaluateStrRev(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_REV);
	}
    bool Parser::evaluateStrSplit(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("str.split");
        result = expr;
        return false;
	}
    bool Parser::evaluateStrSplitRest(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_SPLIT_REST);
	}
    bool Parser::evaluateStrLt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_LT);
	}   
    bool Parser::evaluateStrLe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_LE);
	}
    bool Parser::evaluateStrGt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_GT);
	}
    bool Parser::evaluateStrGe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_GE);
	}
    bool Parser::evaluateStrInReg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_IN_REG);
	}
    bool Parser::evaluateStrContains(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_CONTAINS);
	}
    bool Parser::evaluateStrIsDigit(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_IS_DIGIT);
	}
    bool Parser::evaluateStrFromInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_FROM_INT);
	}
    bool Parser::evaluateStrToInt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_TO_INT);
	}
    bool Parser::evaluateStrToReg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_TO_REG);
	}
    bool Parser::evaluateStrToCode(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_TO_CODE);
	}
    bool Parser::evaluateStrFromCode(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_FROM_CODE);
	}
    bool Parser::evaluateRegConcat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.concat");
        result = expr;
        return false;
    }
    bool Parser::evaluateRegUnion(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.union");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegInter(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.inter");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegDiff(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.diff");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegStar(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.star");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegPlus(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.plus");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegOpt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.opt");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegRange(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.range");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegRepeat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.repeat");
        result = expr;
        return false;
	}
    bool Parser::evaluateRegComplement(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        (void)model;
		not_implemented_warning("reg.complement");
        result = expr;
        return false;
	}
    bool Parser::evaluateApplyFun(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        (void)model;
		not_implemented_warning("function-apply");
        result = expr;
        return false;
    }
    bool Parser::evaluateLet(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result){
        std::shared_ptr<DAGNode> body = expandLet(expr);
        return evaluate(body, model, result);
    }

    std::shared_ptr<DAGNode> Parser::expandLet(std::shared_ptr<DAGNode> expr){
        if(getOptions()->parsing_preserve_let && (expr->isLet() || expr->isLetChain())){
            // Determine the body based on the structure type
            std::shared_ptr<DAGNode> body;
            if(expr->isLet()){
                // expand let - body is the first child
                body = expr->getChild(0);
            } else {
                // expand let-chain - body is the last child: [bind_var_list1, ..., bind_var_listN, body]
                condAssert(expr->getChildrenSize() >= 2, "let-chain should have at least one bind_var_list and one body");
                body = expr->getChild(expr->getChildrenSize() - 1);
            }
            
            // use iteration instead of recursion to handle all nested let_bind_var
            std::stack<std::shared_ptr<DAGNode>> nodeStack;
            std::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> resultMap; // save the result of processed nodes
            std::unordered_map<std::shared_ptr<DAGNode>, bool> hasChangedMap; // save the flag of whether the node has been changed
            
            // push the initial node
            nodeStack.push(body);
            
            // iterate until the stack is empty
            while(!nodeStack.empty()) {
                std::shared_ptr<DAGNode> currentNode = nodeStack.top();
                
                // check if the node has been processed
                if(resultMap.find(currentNode) != resultMap.end()) {
                    nodeStack.pop();
                    continue;
                }
                
                // check if all children have been processed
                bool allChildrenProcessed = true;
                std::vector<std::shared_ptr<DAGNode>> processedChildren;
                bool hasChanged = false;
                
                // process the children
                for(size_t i = 0; i < currentNode->getChildrenSize(); i++) {
                    std::shared_ptr<DAGNode> child = currentNode->getChild(i);
                    
                    // if the child has been processed, use the processed result
                    if(resultMap.find(child) != resultMap.end()) {
                        processedChildren.push_back(resultMap[child]);
                        if(hasChangedMap[child]){
                            hasChanged = true;
                        }
                    } else {
                        // if the child has not been processed, push it to the stack and process it
                        nodeStack.push(child);
                        allChildrenProcessed = false;
                        break;
                    }
                }
                
                // if all children have been processed, process the current node
                if(allChildrenProcessed) {
                    nodeStack.pop();
                    std::shared_ptr<DAGNode> result;
                    
                    if(currentNode->isLetBindVar()) {
                        // if the current node is a let_bind_var, replace it with its child(0)
                        condAssert(currentNode->getChildrenSize() > 0, "let_bind_var should have at least one child");
                        result = resultMap[currentNode->getChild(0)]; // use the processed child(0)
                        hasChangedMap[currentNode] = true;
                    } else if(hasChanged) {
                        // if some children have been replaced, reconstruct the node
                        result = mkOper(currentNode->getSort(), currentNode->getKind(), processedChildren);
                        hasChangedMap[currentNode] = true;
                    } else {
                        // no change, keep the original node
                        result = currentNode;
                        hasChangedMap[currentNode] = false;
                    }
                    
                    // save the processed result
                    resultMap[currentNode] = result;
                }
            }
            
            // return the processed result
            return resultMap[body];
        }
        else{
            return expr;
        }
    }
    bool Parser::evaluateMax(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_MAX);
    }
    bool Parser::evaluateMin(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_MIN);
    }
    bool Parser::evaluateStrSplitAtRe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_SPLIT_AT_RE);
    }
    bool Parser::evaluateStrSplitRestRe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_SPLIT_REST_RE);
    }
    bool Parser::evaluateStrNumSplitsRe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_NUM_SPLITS_RE);
    }
    
}


