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
        if(model->isEmpty() && !expr->isEq() && !expr->isDistinct()
           && !expr->isConstructorApp() && !expr->isSelectorApp()
           && !expr->isTesterApp() && !expr->isMatchApp()){
            result = expr;
            return false;
        }
        if(expr->isUnknown()){
            return false;
        }
        else if(expr->isErr()){
            return false;
        }
        else if(expr->isConst()){
            result = expr;
            return false;
        }
        else if(expr->isVar()){
            result = model->get(expr);
            if(result->isUnknown()){
                result = expr;
                return false;
            }
            else{
                return true;
            }
        }
        else if(expr->isAnd()){
            return evaluateAnd(expr, model, result);
        }
        else if(expr->isOr()){
            return evaluateOr(expr, model, result);
        }
        else if(expr->isNot()){
            return evaluateNot(expr, model, result);
        }
        else if(expr->isImplies()){
            return evaluateImpl(expr, model, result);
        }
        else if(expr->isXor()){
            return evaluateXor(expr, model, result);
        }
        else if(expr->isEq()){
            return evaluateEq(expr, model, result);
        }
        else if(expr->isDistinct()){
            return evaluateDistinct(expr, model, result);
        }
        else if(expr->isIte()){
            return evaluateIte(expr, model, result);
        }
        else if(expr->isAdd()){
            return evaluateAdd(expr, model, result);
        }
        else if(expr->isNeg()){
            return evaluateNeg(expr, model, result);
        }
        else if(expr->isSub()){
            return evaluateSub(expr, model, result);
        }
        else if(expr->isMul()){
            return evaluateMul(expr, model, result);
        }
        else if(expr->isDivInt()){
            return evaluateDivInt(expr, model, result);
        }
        else if(expr->isDivReal()){
            return evaluateDivReal(expr, model, result);
        }
        else if(expr->isMod()){
            return evaluateMod(expr, model, result);
        }
        else if(expr->isPow()){
            return evaluatePow(expr, model, result);
        }
        else if(expr->isPow2()){
            return evaluatePow2(expr, model, result);
        }
        else if(expr->isIAnd()){
            return evaluateIand(expr, model, result);
        }
        else if(expr->isAbs()){
            return evaluateAbs(expr, model, result);
        }
        else if(expr->isSqrt()){
            return evaluateSqrt(expr, model, result);
        }
        else if(expr->isSafeSqrt()){
            return evaluateSafeSqrt(expr, model, result);
        }
        else if(expr->isCeil()){
            return evaluateCeil(expr, model, result);
        }
        else if(expr->isFloor()){
            return evaluateFloor(expr, model, result);
        }
        else if(expr->isRound()){
            return evaluateRound(expr, model, result);
        }
        else if(expr->isExp()){
            return evaluateExp(expr, model, result);
        }
        else if(expr->isLn()){
            return evaluateLn(expr, model, result);
        }
        else if(expr->isLg()){
            return evaluateLg(expr, model, result);
        }
        else if(expr->isLn()){
            return evaluateLn(expr, model, result);
        }
        else if(expr->isLog()){
            return evaluateLog(expr, model, result);
        }
        else if(expr->isLb()){
            return evaluateLb(expr, model, result);
        }
        else if(expr->isSin()){
            return evaluateSin(expr, model, result);
        }
        else if(expr->isCos()){
            return evaluateCos(expr, model, result);
        }
        else if(expr->isTan()){
            return evaluateTan(expr, model, result);
        }
        else if(expr->isAsin()){
            return evaluateAsin(expr, model, result);
        }
        else if(expr->isAcos()){
            return evaluateAcos(expr, model, result);
        }
        else if(expr->isAtan()){
            return evaluateAtan(expr, model, result);
        }
        else if(expr->isSinh()){
            return evaluateSinh(expr, model, result);
        }
        else if(expr->isCosh()){
            return evaluateCosh(expr, model, result);
        }
        else if(expr->isTanh()){
            return evaluateTanh(expr, model, result);
        }
        else if(expr->isAsinh()){
            return evaluateAsinh(expr, model, result);
        }
        else if(expr->isAcosh()){
            return evaluateAcosh(expr, model, result);
        }
        else if(expr->isAtanh()){
            return evaluateAtanh(expr, model, result);
        }
        else if(expr->isAsech()){
            return evaluateAsech(expr, model, result);
        }
        else if(expr->isAcsch()){
            return evaluateAcsch(expr, model, result);
        }
        else if(expr->isAcoth()){
            return evaluateAcoth(expr, model, result);
        }
        else if(expr->isAtan2()){
            return evaluateAtan2(expr, model, result);
        }
        else if(expr->isLe()){
            return evaluateLe(expr, model, result);
        }
        else if(expr->isLt()){
            return evaluateLt(expr, model, result);
        }
        else if(expr->isGe()){
            return evaluateGe(expr, model, result);
        }
        else if(expr->isGt()){
            return evaluateGt(expr, model, result);
        }
        else if(expr->isToReal()){
            return evaluateToReal(expr, model, result);
        }
        else if(expr->isToInt()){
            return evaluateToInt(expr, model, result);
        }
        else if(expr->isInt()){
            return evaluateIsInt(expr, model, result);
        }
        else if(expr->isDivisible()){
            return evaluateIsDivisible(expr, model, result);
        }
        else if(expr->isPrime()){
            return evaluateIsPrime(expr, model, result);
        }
        else if(expr->isEven()){
            return evaluateIsEven(expr, model, result);
        }
        else if(expr->isOdd()){
            return evaluateIsOdd(expr, model, result);
        }
        else if(expr->isGcd()){
            return evaluateGcd(expr, model, result);
        }
        else if(expr->isLcm()){
            return evaluateLcm(expr, model, result);
        }
        else if(expr->isFact()){
            return evaluateFact(expr, model, result);
        }
        else if(expr->isBVNot()){
            return evaluateBvNot(expr, model, result);
        }
        else if(expr->isBVNeg()){
            return evaluateBvNeg(expr, model, result);
        }
        else if(expr->isBVAnd()){
            return evaluateBvAnd(expr, model, result);
        }
        else if(expr->isBVOr()){
            return evaluateBvOr(expr, model, result);
        }
        else if(expr->isBVXor()){
            return evaluateBvXor(expr, model, result);
        }
        else if(expr->isBVNand()){
            return evaluateBvNand(expr, model, result);
        }
        else if(expr->isBVNor()){
            return evaluateBvNor(expr, model, result);
        }
        else if(expr->isBVXnor()){
            return evaluateBvXnor(expr, model, result);
        }
        else if(expr->isBVComp()){
            return evaluateBvComp(expr, model, result);
        }
        else if(expr->isBVAdd()){
            return evaluateBvAdd(expr, model, result);
        }
        else if(expr->isBVSub()){
            return evaluateBvSub(expr, model, result);
        }
        else if(expr->isBVMul()){
            return evaluateBvMul(expr, model, result);
        }
        else if(expr->isBVUDiv()){
            return evaluateBvUdiv(expr, model, result);
        }
        else if(expr->isBVURem()){
            return evaluateBvUrem(expr, model, result);
        }
        else if(expr->isBVSDiv()){
            return evaluateBvSdiv(expr, model, result);
        }
        else if(expr->isBVSRem()){
            return evaluateBvSrem(expr, model, result);
        }
        else if(expr->isBVSMod()){
            return evaluateBvSmod(expr, model, result);
        }
        else if(expr->isBVShl()){
            return evaluateBvShl(expr, model, result);
        }
        else if(expr->isBVLSHR()){
            return evaluateBvLshr(expr, model, result);
        }
        else if(expr->isBVASHR()){
            return evaluateBvAshr(expr, model, result);
        }
        else if(expr->isBVUlt()){
            return evaluateBvUlt(expr, model, result);
        }
        else if(expr->isBVUle()){
            return evaluateBvUle(expr, model, result);
        }
        else if(expr->isBVUgt()){
            return evaluateBvUgt(expr, model, result);
        }
        else if(expr->isBVUge()){
            return evaluateBvUge(expr, model, result);
        }
        else if(expr->isBVSlt()){
            return evaluateBvSlt(expr, model, result);
        }
        else if(expr->isBVSle()){
            return evaluateBvSle(expr, model, result);
        }
        else if(expr->isBVSgt()){
            return evaluateBvSgt(expr, model, result);
        }
        else if(expr->isBVSge()){
            return evaluateBvSge(expr, model, result);
        }
        else if(expr->isBVConcat()){
            return evaluateBvConcat(expr, model, result);
        }
        else if(expr->isBVToNat()){
            return evaluateBvToNat(expr, model, result);
        }
        else if(expr->isNatToBV()){
            return evaluateBvNatToBv(expr, model, result);
        }
        else if(expr->isIntToBV()){
            return evaluateBvIntToBv(expr, model, result);
        }
        else if(expr->isBVToInt()){
            return evaluateBvToInt(expr, model, result);
        }
        else if(expr->isBVExtract()){
            return evaluateBvExtract(expr, model, result);
        }
        else if(expr->isBVRepeat()){
            return evaluateBvRepeat(expr, model, result);
        }
        else if(expr->isBVZeroExt()){
            return evaluateBvZeroExt(expr, model, result);
        }
        else if(expr->isBVSignExt()){
            return evaluateBvSignExt(expr, model, result);
        }
        else if(expr->isBVRotLeft()){
            return evaluateBvRotLeft(expr, model, result);
        }
        else if(expr->isBVRotRight()){
            return evaluateBvRotRight(expr, model, result);
        }
        else if(expr->isFPAbs()){
            return evaluateFpAbs(expr, model, result);
        }
        else if(expr->isFPNeg()){
            return evaluateFpNeg(expr, model, result);
        }
        else if(expr->isFPAdd()){
            return evaluateFpAdd(expr, model, result);
        }
        else if(expr->isFPSub()){
            return evaluateFpSub(expr, model, result);
        }
        else if(expr->isFPMul()){
            return evaluateFpMul(expr, model, result);
        }
        else if(expr->isFPDiv()){
            return evaluateFpDiv(expr, model, result);
        }
        else if(expr->isFPFMA()){
            return evaluateFpFma(expr, model, result);
        }
        else if(expr->isFPSqrt()){
            return evaluateFpSqrt(expr, model, result);
        }
        else if(expr->isFPRem()){
            return evaluateFpRem(expr, model, result);
        }
        else if(expr->isFPRoundToIntegral()){
            return evaluateFpRoundToIntegral(expr, model, result);
        }
        else if(expr->isFPMin()){
            return evaluateFpMin(expr, model, result);
        }
        else if(expr->isFPMax()){
            return evaluateFpMax(expr, model, result);
        }
        else if(expr->isFPLe()){
            return evaluateFpLe(expr, model, result);
        }
        else if(expr->isFPLt()){
            return evaluateFpLt(expr, model, result);
        }
        else if(expr->isFPGe()){
            return evaluateFpGe(expr, model, result);
        }
        else if(expr->isFPGt()){
            return evaluateFpGt(expr, model, result);
        }
        else if(expr->isFPEq()){
            return evaluateFpEq(expr, model, result);
        }
        else if(expr->isFPToUBV()){
            return evaluateFpToUbv(expr, model, result);
        }
        else if(expr->isFPToSBV()){
            return evaluateFpToSbv(expr, model, result);
        }
        else if(expr->isFPToReal()){
            return evaluateFpToReal(expr, model, result);
        }
        else if(expr->isToFP()){
            return evaluateToFp(expr, model, result);
        }
        else if(expr->isToFPUnsigned()){
            return evaluateToFpUnsigned(expr, model, result);
        }
        else if(expr->isFPIsNormal()){
            return evaluateFpIsNormal(expr, model, result);
        }
        else if(expr->isFPIsSubnormal()){
            return evaluateFpIsSubnormal(expr, model, result);
        }
        else if(expr->isFPIsZero()){
            return evaluateFpIsZero(expr, model, result);
        }
        else if(expr->isFPIsInf()){
            return evaluateFpIsInf(expr, model, result);
        }
        else if(expr->isFPIsNaN()){
            return evaluateFpIsNaN(expr, model, result);
        }
        else if(expr->isFPIsNeg()){
            return evaluateFpIsNeg(expr, model, result);
        }
        else if(expr->isFPIsPos()){
            return evaluateFpIsPos(expr, model, result);
        }
        else if(expr->isSelect()){
            return evaluateSelect(expr, model, result);
        }
        else if(expr->isStore()){
            return evaluateStore(expr, model, result);
        }
        else if(expr->isStrLen()){
            return evaluateStrLen(expr, model, result);
        }
        else if(expr->isStrConcat()){
            return evaluateStrConcat(expr, model, result);
        }
        else if(expr->isStrSubstr()){
            return evaluateStrSubstr(expr, model, result);
        }
        else if(expr->isStrPrefixof()){
            return evaluateStrPrefixof(expr, model, result);
        }
        else if(expr->isStrSuffixof()){
            return evaluateStrSuffixof(expr, model, result);
        }
        else if(expr->isStrIndexof()){
            return evaluateStrIndexof(expr, model, result);
        }
        else if(expr->isStrCharat()){
            return evaluateStrCharat(expr, model, result);
        }
        else if(expr->isStrUpdate()){
            return evaluateStrUpdate(expr, model, result);
        }
        else if(expr->isStrReplace()){
            return evaluateStrReplace(expr, model, result);
        }
        else if(expr->isStrReplaceAll()){
            return evaluateStrReplaceAll(expr, model, result);
        }
        else if(expr->isStrToLower()){
            return evaluateStrToLower(expr, model, result);
        }
        else if(expr->isStrToUpper()){
            return evaluateStrToUpper(expr, model, result);
        }
        else if(expr->isStrRev()){
            return evaluateStrRev(expr, model, result);
        }
        else if(expr->isStrSplit()){
            return evaluateStrSplit(expr, model, result);
        }
        else if(expr->isStrSplitRest()){
            return evaluateStrSplitRest(expr, model, result);
        }
        else if(expr->isStrSplitAtRe()){
            return evaluateStrSplitAtRe(expr, model, result);
        }
        else if(expr->isStrSplitRestRe()){
            return evaluateStrSplitRestRe(expr, model, result);
        }
        else if(expr->isStrNumSplitsRe()){
            return evaluateStrNumSplitsRe(expr, model, result);
        }
        else if(expr->isStrLt()){
            return evaluateStrLt(expr, model, result);
        }
        else if(expr->isStrLe()){
            return evaluateStrLe(expr, model, result);
        }
        else if(expr->isStrGt()){
            return evaluateStrGt(expr, model, result);
        }
        else if(expr->isStrGe()){
            return evaluateStrGe(expr, model, result);
        }
        else if(expr->isStrInReg()){
            return evaluateStrInReg(expr, model, result);
        }
        else if(expr->isStrContains()){
            return evaluateStrContains(expr, model, result);
        }
        else if(expr->isStrIsDigit()){
            return evaluateStrIsDigit(expr, model, result);
        }
        else if(expr->isStrFromInt()){
            return evaluateStrFromInt(expr, model, result);
        }
        else if(expr->isStrToInt()){
            return evaluateStrToInt(expr, model, result);
        }
        else if(expr->isStrToReg()){
            return evaluateStrToReg(expr, model, result);
        }
        else if(expr->isStrToCode()){
            return evaluateStrToCode(expr, model, result);
        }
        else if(expr->isStrFromCode()){
            return evaluateStrFromCode(expr, model, result);
        }
        else if(expr->isRegConcat()){
            return evaluateRegConcat(expr, model, result);
        }
        else if(expr->isRegUnion()){
            return evaluateRegUnion(expr, model, result);
        }
        else if(expr->isRegInter()){
            return evaluateRegInter(expr, model, result);
        }
        else if(expr->isRegDiff()){
            return evaluateRegDiff(expr, model, result);
        }
        else if(expr->isRegStar()){
            return evaluateRegStar(expr, model, result);
        }
        else if(expr->isRegPlus()){
            return evaluateRegPlus(expr, model, result);
        }
        else if(expr->isRegOpt()){
            return evaluateRegOpt(expr, model, result);
        }
        else if(expr->isRegRange()){
            return evaluateRegRange(expr, model, result);
        }
        else if(expr->isRegRepeat()){
            return evaluateRegRepeat(expr, model, result);
        }
        else if(expr->isRegComplement()){
            return evaluateRegComplement(expr, model, result);
        }
        else if(expr->isRegLoop()){
            return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_REG_LOOP);
        }
        else if(expr->isConstArray()){
            return evaluateConstArray(expr, model, result);
        }
        else if(expr->getKind() == NODE_KIND::NT_STR_REPLACE_REG){
            return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_REPLACE_REG);
        }
        else if(expr->getKind() == NODE_KIND::NT_STR_REPLACE_REG_ALL){
            return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_REPLACE_REG_ALL);
        }
        else if(expr->getKind() == NODE_KIND::NT_STR_INDEXOF_REG){
            return evaluateSimpleOp(expr, model, result, NODE_KIND::NT_STR_INDEXOF_REG);
        }
        else if(expr->isConstructorApp()){
            return evaluateDtConstructor(expr, model, result);
        }
        else if(expr->isSelectorApp()){
            return evaluateDtSelector(expr, model, result);
        }
        else if(expr->isTesterApp()){
            return evaluateDtTester(expr, model, result);
        }
        else if(expr->isMatchApp()){
            return evaluateMatch(expr, model, result);
        }
        else if(expr->isUFApplication()){
            return evaluateUFApply(expr, model, result);
        }
        else if(expr->isFuncApplication()){
            return evaluateApplyFun(expr, model, result);
        }
        else if(expr->isLet()){
            return evaluateLet(expr, model, result);
        }
        result = expr;
        return false;
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
            case NODE_KIND::NT_STR_INDEXOF_REG:
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
            case NODE_KIND::NT_STR_REPLACE_REG:
            case NODE_KIND::NT_STR_REPLACE_REG_ALL:
            case NODE_KIND::NT_REG_LOOP:
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
            bool c = evaluate(child, model, eval);
            if(!c) eval = child;
            changed |= c;
            if(eval->isConst() || eval->isDtGroundValue()){
                const_vals.emplace_back(eval);
            }
            else{
                children.emplace_back(eval);
            }
        }
        if(!changed && const_vals.empty()){
            result = expr;
            return false;
        }
        if(const_vals.empty()){
            result = mkEq(children);
            return true;
        }
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
            else if(const_val->isDtGroundValue() && const_vals[i]->isDtGroundValue()){
                if(!areDtValuesEqual(const_val, const_vals[i])){
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
        std::vector<std::shared_ptr<DAGNode>> dt_vals;
        for(auto child : expr->getChildren()){
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            bool c = evaluate(child, model, eval);
            if(!c) eval = child;
            changed |= c;
            if(eval->isDtGroundValue()){
                for(auto& prev : dt_vals){
                    if(areDtValuesEqual(eval, prev)){
                        result = mkFalse();
                        return true;
                    }
                }
                dt_vals.push_back(eval);
            }
            else if(eval->isConst()){
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
        if(!changed && const_vals.empty() && dt_vals.empty()){
            result = expr;
            return false;
        }
        if(const_vals.empty() && dt_vals.empty()){
            result = mkDistinct(children);
            return true;
        }
        if(children.empty()){
            result = mkTrue();
        }
        else{
            children.insert(children.end(), const_vals.begin(), const_vals.end());
            children.insert(children.end(), dt_vals.begin(), dt_vals.end());
            if(children.size() <= 1){
                result = mkTrue();
            }
            else{
                result = mkDistinct(children);
            }
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
    bool Parser::evaluateBvExtract(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> hi = expr->getChildren()[1];
        std::shared_ptr<DAGNode> lo = expr->getChildren()[2];
        if(bv->isCBV()){
            Integer hi_val = toInt(hi);
            Integer lo_val = toInt(lo);
            size_t width = (hi_val - lo_val + 1).toULong();
            result = mkConstBv(BitVectorUtils::bvExtract(bv->toString(), hi_val, lo_val), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_EXTRACT, bv, hi, lo);
        }
        return true;
	}
    bool Parser::evaluateBvRepeat(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> cnt = expr->getChildren()[1];
        if(bv->isCBV() && cnt->isCInt()){
            Integer n = toInt(cnt);
            size_t width = bv->getSort()->getBitWidth() * n.toULong();
            result = mkConstBv(BitVectorUtils::bvRepeat(bv->toString(), n), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_REPEAT, bv, cnt);
        }
        return true;
	}
    bool Parser::evaluateBvZeroExt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> ext = expr->getChildren()[1];
        if(bv->isCBV() && ext->isCInt()){
            Integer n = toInt(ext);
            size_t width = bv->getSort()->getBitWidth() + n.toULong();
            result = mkConstBv(BitVectorUtils::bvZeroExtend(bv->toString(), n), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_ZERO_EXT, bv, ext);
        }
        return true;
	}
    bool Parser::evaluateBvSignExt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> ext = expr->getChildren()[1];
        if(bv->isCBV() && ext->isCInt()){
            Integer n = toInt(ext);
            size_t width = bv->getSort()->getBitWidth() + n.toULong();
            result = mkConstBv(BitVectorUtils::bvSignExtend(bv->toString(), n), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_SIGN_EXT, bv, ext);
        }
        return true;
	}
    bool Parser::evaluateBvRotLeft(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> amt = expr->getChildren()[1];
        if(bv->isCBV() && amt->isCInt()){
            size_t width = bv->getSort()->getBitWidth();
            result = mkConstBv(BitVectorUtils::bvRotateLeft(bv->toString(), toInt(amt)), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_ROTATE_LEFT, bv, amt);
        }
        return true;
	}
    bool Parser::evaluateBvRotRight(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, bv);
        if(!changed){ result = expr; return false; }
        std::shared_ptr<DAGNode> amt = expr->getChildren()[1];
        if(bv->isCBV() && amt->isCInt()){
            size_t width = bv->getSort()->getBitWidth();
            result = mkConstBv(BitVectorUtils::bvRotateRight(bv->toString(), toInt(amt)), width);
        } else {
            result = mkOper(expr->getSort(), NODE_KIND::NT_BV_ROTATE_RIGHT, bv, amt);
        }
        return true;
	}
    bool Parser::evaluateFpAbs(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        auto sort = fp1->getSort();
        if(fp1->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto v = fpNodeToValue(fp1);
            if(v){ result = mkConstFp(v->abs().toSMTFP(), e, s); return true; }
        }
        result = mkOper(fp1->getSort(), NODE_KIND::NT_FP_ABS, fp1);
        return true;
	}
    bool Parser::evaluateFpNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        auto sort = fp1->getSort();
        if(fp1->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto v = fpNodeToValue(fp1);
            if(v){ result = mkConstFp(v->neg().toSMTFP(), e, s); return true; }
        }
        result = mkOper(fp1->getSort(), NODE_KIND::NT_FP_NEG, fp1);
        return true;
	}
    bool Parser::evaluateFpAdd(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren(); // [rm, fp1, fp2]
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE, fp2 = NodeManager::NULL_NODE;
        bool c1 = evaluate(ch[1], model, fp1), c2 = evaluate(ch[2], model, fp2);
        if(!c1) fp1 = ch[1];
        if(!c2) fp2 = ch[2];
        auto rm = ch[0]; auto rnd = getFPRoundingModeMpfr(rm);
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpBinaryOp(*va, *vb, e, s, rnd, mpfr_add);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_ADD, {rm, fp1, fp2});
        return true;
	}
    bool Parser::evaluateFpSub(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren(); // [rm, fp1, fp2]
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE, fp2 = NodeManager::NULL_NODE;
        bool c1 = evaluate(ch[1], model, fp1), c2 = evaluate(ch[2], model, fp2);
        if(!c1) fp1 = ch[1];
        if(!c2) fp2 = ch[2];
        auto rm = ch[0]; auto rnd = getFPRoundingModeMpfr(rm);
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpBinaryOp(*va, *vb, e, s, rnd, mpfr_sub);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_SUB, {rm, fp1, fp2});
        return true;
	}
    bool Parser::evaluateFpMul(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren(); // [rm, fp1, fp2]
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE, fp2 = NodeManager::NULL_NODE;
        bool c1 = evaluate(ch[1], model, fp1), c2 = evaluate(ch[2], model, fp2);
        if(!c1) fp1 = ch[1];
        if(!c2) fp2 = ch[2];
        auto rm = ch[0]; auto rnd = getFPRoundingModeMpfr(rm);
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpBinaryOp(*va, *vb, e, s, rnd, mpfr_mul);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_MUL, {rm, fp1, fp2});
        return true;
	}
    bool Parser::evaluateFpDiv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren(); // [rm, fp1, fp2]
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE, fp2 = NodeManager::NULL_NODE;
        bool c1 = evaluate(ch[1], model, fp1), c2 = evaluate(ch[2], model, fp2);
        if(!c1) fp1 = ch[1];
        if(!c2) fp2 = ch[2];
        auto rm = ch[0]; auto rnd = getFPRoundingModeMpfr(rm);
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpBinaryOp(*va, *vb, e, s, rnd, mpfr_div);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_DIV, {rm, fp1, fp2});
        return true;
	}
    bool Parser::evaluateFpFma(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren(); // [rm, fp1, fp2, fp3]
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE, fp3=NodeManager::NULL_NODE;
        bool c1=evaluate(ch[1],model,fp1), c2=evaluate(ch[2],model,fp2), c3=evaluate(ch[3],model,fp3);
        if(!c1) fp1 = ch[1];
        if(!c2) fp2 = ch[2];
        if(!c3) fp3 = ch[3];
        auto rm = ch[0]; auto rnd = getFPRoundingModeMpfr(rm);
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && fp3->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2); auto vc = fpNodeToValue(fp3);
            if(va && vb && vc){
                auto r = FloatingPointUtils::fpTernaryOp(*va, *vb, *vc, e, s, rnd, mpfr_fma);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_FMA, {rm, fp1, fp2, fp3});
        return true;
	}
    bool Parser::evaluateFpSqrt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren();
        std::shared_ptr<DAGNode> rmOut;
        std::shared_ptr<DAGNode> fpIn;
        bool rmChanged = false;
        if(ch.size() == 2 && ch[0]->getSort()->isRoundingMode()) {
            rmOut = NodeManager::NULL_NODE;
            rmChanged = evaluate(ch[0], model, rmOut);
            if(!rmChanged) rmOut = ch[0];
            fpIn = ch[1];
        } else if(ch.size() == 1) {
            rmOut = mkRoundingMode("RNE");
            fpIn = ch[0];
        } else {
            result = expr;
            return false;
        }
        mpfr_rnd_t rnd = MPFR_RNDN;
        if(rmOut->isCRoundingMode()) {
            rnd = getFPRoundingModeMpfr(rmOut);
        }
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool fpChanged = evaluate(fpIn, model, fp1);
        if(!fpChanged) fp1 = fpIn;
        auto sort = fp1->getSort();
        if(fp1->isCFP() && sort && rmOut->isCRoundingMode()){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto v = fpNodeToValue(fp1);
            if(v){
                auto r = FloatingPointUtils::fpUnaryOp(*v, e, s, rnd, mpfr_sqrt);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_SQRT, {rmOut, fp1});
        const bool upgradedLegacyUnary = (ch.size() == 1);
        return rmChanged || fpChanged || upgradedLegacyUnary;
	}
    bool Parser::evaluateFpRem(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpRemainder(*va, *vb, e, s);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_REM, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpRoundToIntegral(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren();
        std::shared_ptr<DAGNode> rmOut;
        std::shared_ptr<DAGNode> fpIn;
        bool rmChanged = false;
        if(ch.size() == 2 && ch[0]->getSort()->isRoundingMode()) {
            rmOut = NodeManager::NULL_NODE;
            rmChanged = evaluate(ch[0], model, rmOut);
            if(!rmChanged) rmOut = ch[0];
            fpIn = ch[1];
        } else if(ch.size() == 1) {
            rmOut = mkRoundingMode("RNE");
            fpIn = ch[0];
        } else {
            result = expr;
            return false;
        }
        mpfr_rnd_t rnd = MPFR_RNDN;
        if(rmOut->isCRoundingMode()) {
            rnd = getFPRoundingModeMpfr(rmOut);
        }
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool fpChanged = evaluate(fpIn, model, fp1);
        if(!fpChanged) fp1 = fpIn;
        auto sort = fp1->getSort();
        if(fp1->isCFP() && sort && rmOut->isCRoundingMode()){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto v = fpNodeToValue(fp1);
            if(v){
                auto r = FloatingPointUtils::fpRoundToIntegral(*v, e, s, rnd);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_ROUND_TO_INTEGRAL, {rmOut, fp1});
        const bool upgradedLegacyUnary = (ch.size() == 1);
        return rmChanged || fpChanged || upgradedLegacyUnary;
	}
    bool Parser::evaluateFpMin(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpMin(*va, *vb);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_MIN, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpMax(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        auto sort = fp1->getSort();
        if(fp1->isCFP() && fp2->isCFP() && sort){
            size_t e = sort->getExponentWidth(), s = sort->getSignificandWidth();
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                auto r = FloatingPointUtils::fpMax(*va, *vb);
                if(r){ result = mkConstFp(r->toSMTFP(), e, s); return true; }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_MAX, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpLe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        if(fp1->isCFP() && fp2->isCFP()){
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                int cmp = FloatingPointUtils::fpCompare(*va, *vb);
                result = (cmp != 2 && cmp <= 0) ? mkTrue() : mkFalse(); return true;
            }
        }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_LE, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpLt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        if(fp1->isCFP() && fp2->isCFP()){
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                int cmp = FloatingPointUtils::fpCompare(*va, *vb);
                result = (cmp == -1) ? mkTrue() : mkFalse(); return true;
            }
        }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_LT, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpGe(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        if(fp1->isCFP() && fp2->isCFP()){
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                int cmp = FloatingPointUtils::fpCompare(*va, *vb);
                result = (cmp == 0 || cmp == 1) ? mkTrue() : mkFalse(); return true;
            }
        }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_GE, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpGt(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        if(fp1->isCFP() && fp2->isCFP()){
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                int cmp = FloatingPointUtils::fpCompare(*va, *vb);
                result = (cmp == 1) ? mkTrue() : mkFalse(); return true;
            }
        }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_GT, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpEq(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1=NodeManager::NULL_NODE, fp2=NodeManager::NULL_NODE;
        bool c1=evaluate(expr->getChildren()[0],model,fp1), c2=evaluate(expr->getChildren()[1],model,fp2);
        if(!c1) fp1 = expr->getChildren()[0];
        if(!c2) fp2 = expr->getChildren()[1];
        if(fp1->isCFP() && fp2->isCFP()){
            auto va = fpNodeToValue(fp1); auto vb = fpNodeToValue(fp2);
            if(va && vb){
                int cmp = FloatingPointUtils::fpCompare(*va, *vb);
                result = (cmp == 0) ? mkTrue() : mkFalse(); return true;
            }
        }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_EQ, fp1, fp2);
        return true;
	}
    bool Parser::evaluateFpToUbv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        // fp.to_ubv: children = [rm, fp], target BV width from expr sort
        auto ch = expr->getChildren();
        if(ch.size() < 2){ result = expr; return false; }
        auto rm = ch[0];
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(ch[1], model, fp1);
        if(!changed) fp1 = ch[1];
        if(fp1->isCFP()){
            auto sort = expr->getSort();
            if(!sort){ result = expr; return false; }
            size_t bv_width = sort->getBitWidth();
            auto v = fpNodeToValue(fp1);
            if(v){
                auto rnd = getFPRoundingModeMpfr(rm);
                auto bvStr = FloatingPointUtils::fpValueToUbv(*v, bv_width, rnd);
                if(bvStr){
                    result = mkConstBv(*bvStr, bv_width);
                    return true;
                }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_TO_UBV, {rm, fp1});
        return true;
	}
    bool Parser::evaluateFpToSbv(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        // fp.to_sbv: children = [rm, fp], target BV width from expr sort
        auto ch = expr->getChildren();
        if(ch.size() < 2){ result = expr; return false; }
        auto rm = ch[0];
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(ch[1], model, fp1);
        if(!changed) fp1 = ch[1];
        if(fp1->isCFP()){
            auto sort = expr->getSort();
            if(!sort){ result = expr; return false; }
            size_t bv_width = sort->getBitWidth();
            auto v = fpNodeToValue(fp1);
            if(v){
                auto rnd = getFPRoundingModeMpfr(rm);
                auto bvStr = FloatingPointUtils::fpValueToSbv(*v, bv_width, rnd);
                if(bvStr){
                    result = mkConstBv(*bvStr, bv_width);
                    return true;
                }
            }
        }
        result = mkOper(expr->getSort(), NODE_KIND::NT_FP_TO_SBV, {rm, fp1});
        return true;
	}
    bool Parser::evaluateFpToReal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){
            auto v = fpNodeToValue(fp1);
            if(v){
                auto d = FloatingPointUtils::fpToDouble(*v);
                if(d){
                    result = mkConstReal(Number(*d));
                    return true;
                }
            }
        }
        result = mkOper(SortManager::REAL_SORT, NODE_KIND::NT_FP_TO_REAL, fp1);
        return true;
	}
    bool Parser::evaluateToFp(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren();
        auto sort = expr->getSort();
        if(!sort){ result = expr; return false; }
        size_t target_eb = sort->getExponentWidth();
        size_t target_sb = sort->getSignificandWidth();

        if(ch.size() == 3) {
            // Case: ((_ to_fp eb sb) BitVec) — bit reinterpretation, children = [eb, sb, bv]
            std::shared_ptr<DAGNode> bv = NodeManager::NULL_NODE;
            bool changed = evaluate(ch[2], model, bv);
            if(!changed) bv = ch[2];
            if(bv->isCBV()) {
                auto fv = FloatingPointUtils::bvBitsToFpValue(bv->getName(), target_eb, target_sb);
                if(fv) {
                    result = mkConstFp(fv->toSMTFP(), target_eb, target_sb);
                    return true;
                }
            }
            result = mkOper(sort, NODE_KIND::NT_FP_TO_FP, ch[0], ch[1], bv);
            return true;
        } else if(ch.size() == 4) {
            // Children = [eb, sb, rm, param]
            auto rm = ch[2];
            std::shared_ptr<DAGNode> param = NodeManager::NULL_NODE;
            bool changed = evaluate(ch[3], model, param);
            if(!changed) param = ch[3];
            auto rnd = getFPRoundingModeMpfr(rm);

            if(param->isCFP()) {
                // Case: FP → FP (precision change)
                auto v = fpNodeToValue(param);
                if(v) {
                    mpfr_t m;
                    mpfr_init2(m, static_cast<mpfr_prec_t>(target_sb + 16));
                    v->toMpfr(m);
                    auto rv = FloatingPointUtils::FPValue::fromMpfr(m, target_eb, target_sb);
                    mpfr_clear(m);
                    result = mkConstFp(rv.toSMTFP(), target_eb, target_sb);
                    return true;
                }
            } else if(param->isCReal() || param->isCInt()) {
                // Case: Real/Integer → FP
                ensureNumberValue(param);
                auto val = param->getValue();
                if(val) {
                    auto num = val->getNumberValue();
                    auto fv = FloatingPointUtils::realToFpValue(num, target_eb, target_sb, rnd);
                    if(fv) {
                        result = mkConstFp(fv->toSMTFP(), target_eb, target_sb);
                        return true;
                    }
                }
            } else if(param->isCBV()) {
                // Case: Signed BV integer → FP (signed interpretation)
                size_t bv_width = param->getSort()->getBitWidth();
                uint64_t bv_val = FloatingPointUtils::parseBVBits(param->getName());
                auto fv = FloatingPointUtils::bvToFpValueSigned(bv_val, bv_width, target_eb, target_sb, rnd);
                if(fv) {
                    result = mkConstFp(fv->toSMTFP(), target_eb, target_sb);
                    return true;
                }
            }
            // Fallback: rebuild symbolic
            result = mkOper(sort, NODE_KIND::NT_FP_TO_FP, {ch[0], ch[1], rm, param});
            return true;
        }
        // Unknown arity — evaluate children and rebuild
        bool changed = false;
        std::vector<std::shared_ptr<DAGNode>> evaled;
        for(auto& child : ch){
            std::shared_ptr<DAGNode> ev = NodeManager::NULL_NODE;
            changed |= evaluate(child, model, ev);
            evaled.push_back(ev);
        }
        if(!changed){ result = expr; return false; }
        result = mkOper(sort, NODE_KIND::NT_FP_TO_FP, evaled);
        return true;
	}
    bool Parser::evaluateToFpUnsigned(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        // ((_ to_fp_unsigned eb sb) RoundingMode (_ BitVec m))
        // Children: [eb, sb, rm, bv_param]
        auto ch = expr->getChildren();
        if(ch.size() != 4){ result = expr; return false; }
        auto sort = expr->getSort();
        if(!sort){ result = expr; return false; }
        size_t target_eb = sort->getExponentWidth();
        size_t target_sb = sort->getSignificandWidth();
        auto rm = ch[2];
        std::shared_ptr<DAGNode> param = NodeManager::NULL_NODE;
        bool changed = evaluate(ch[3], model, param);
        if(!changed) param = ch[3];
        if(param->isCBV()) {
            auto rnd = getFPRoundingModeMpfr(rm);
            size_t bv_width = param->getSort()->getBitWidth();
            uint64_t bv_val = FloatingPointUtils::parseBVBits(param->getName());
            auto fv = FloatingPointUtils::bvToFpValueUnsigned(bv_val, bv_width, target_eb, target_sb, rnd);
            if(fv) {
                result = mkConstFp(fv->toSMTFP(), target_eb, target_sb);
                return true;
            }
        }
        result = mkOper(sort, NODE_KIND::NT_FP_TO_FP_UNSIGNED, {ch[0], ch[1], rm, param});
        return true;
	}
    bool Parser::evaluateFpIsNormal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsNormal(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_NORMAL, fp1);
        return true;
	}
    bool Parser::evaluateFpIsSubnormal(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsSubnormal(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_SUBNORMAL, fp1);
        return true;
	}
    bool Parser::evaluateFpIsZero(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsZero(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_ZERO, fp1);
        return true;
	}
    bool Parser::evaluateFpIsInf(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsInf(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_INF, fp1);
        return true;
	}
    bool Parser::evaluateFpIsNaN(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsNaN(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_NAN, fp1);
        return true;
	}
    bool Parser::evaluateFpIsNeg(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = fpNodeIsNeg(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_NEG, fp1);
        return true;
	}
    bool Parser::evaluateFpIsPos(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        std::shared_ptr<DAGNode> fp1 = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChildren()[0], model, fp1);
        if(!changed) fp1 = expr->getChildren()[0];
        if(fp1->isCFP()){ result = !fpNodeIsNaN(fp1) && !fpNodeIsNeg(fp1) ? mkTrue() : mkFalse(); return true; }
        result = mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FP_IS_POS, fp1);
        return true;
	}
    bool Parser::evaluateConstArray(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode> &result){
        auto ch = expr->getChildren();
        if(ch.empty()){ result = expr; return false; }
        std::shared_ptr<DAGNode> val = NodeManager::NULL_NODE;
        bool changed = evaluate(ch[0], model, val);
        if(!changed){ result = expr; return false; }
        result = getNodeManager()->createNode(expr->getSort(), NODE_KIND::NT_CONST_ARRAY, "const_array", {val});
        return true;
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
            if(simplified->isSelect()){
                // simplifyArray couldn't fully resolve; try model fallback
                auto sel_arr = simplified->getSelectArray();
                auto sel_idx = simplified->getSelectIndex();
                if(sel_arr->isVar() && model){
                    auto mv = model->getArraySelect(sel_arr->getName(), dumpSMTLIB2(sel_idx));
                    if(mv && !mv->isUnknown()){ result = mv; return true; }
                }
            }
            result = simplified;
            return true;
        }
        
        // If nothing changed, try to simplify the original select
        std::shared_ptr<DAGNode> simplified = simplifyArray(expr);
        if (simplified != expr) {
            if(simplified->isSelect()){
                auto sel_arr = simplified->getSelectArray();
                auto sel_idx = simplified->getSelectIndex();
                if(sel_arr->isVar() && model){
                    auto mv = model->getArraySelect(sel_arr->getName(), dumpSMTLIB2(sel_idx));
                    if(mv && !mv->isUnknown()){ result = mv; return true; }
                }
            }
            result = simplified;
            return true;
        }
        
        // Last resort: try model lookup on the original expression
        if(model && array->isVar()){
            auto mv = model->getArraySelect(array->getName(), dumpSMTLIB2(index));
            if(mv && !mv->isUnknown()){ result = mv; return true; }
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
    bool Parser::evaluateUFApply(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        // Helper: sanitize a UF key so it only contains valid SMT-LIB symbol characters
        auto sanitize = [](const std::string& s) -> std::string {
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                if (c == '(') out += "LP";
                else if (c == ')') out += "RP";
                else if (c == ' ') out += '_';
                else out += c;
            }
            return out;
        };

        // Strategy: try symbolic key first (matches collect_uf_apps convention),
        // then try evaluated key (for when arguments resolve to constants).

        // 1. Try symbolic key (from original unevaluated expression children)
        std::string sym_key = "uf@@" + expr->getName();
        for (size_t i = 0; i < expr->getChildrenSize(); i++) {
            sym_key += "@@" + sanitize(dumpSMTLIB2(expr->getChild(i)));
        }
        auto val = model->get(sym_key);
        if (val && !val->isUnknown()) {
            result = val;
            return true;
        }

        // 2. Evaluate all children (arguments)
        std::vector<std::shared_ptr<DAGNode>> eval_args;
        bool children_changed = false;
        for (size_t i = 0; i < expr->getChildrenSize(); i++) {
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            children_changed |= evaluate(expr->getChild(i), model, eval);
            eval_args.push_back(eval);
        }

        // 3. Try evaluated key (from resolved argument values)
        std::string eval_key = "uf@@" + expr->getName();
        for (const auto& a : eval_args) {
            eval_key += "@@";
            if (a->isConst() || a->isTrue() || a->isFalse()) {
                eval_key += sanitize(a->getName());
            } else {
                eval_key += sanitize(dumpSMTLIB2(a));
            }
        }
        if (eval_key != sym_key) {
            val = model->get(eval_key);
            if (val && !val->isUnknown()) {
                result = val;
                return true;
            }
        }

        // If children changed, create new UF application with evaluated children
        if (children_changed) {
            result = getNodeManager()->createNode(expr->getSort(), NODE_KIND::NT_UF_APPLY, expr->getName(), eval_args);
            return true;
        }

        result = expr;
        return false;
    }

    // ─── Datatype evaluation ────────────────────────────────────────────────

    bool Parser::areDtValuesEqual(const std::shared_ptr<DAGNode>& a, const std::shared_ptr<DAGNode>& b) {
        if(!a->isDtGroundValue() || !b->isDtGroundValue()) return false;
        if(a->getName() != b->getName()) return false;
        if(a->getChildrenSize() != b->getChildrenSize()) return false;
        for(size_t i = 0; i < a->getChildrenSize(); ++i){
            auto ca = a->getChild(i);
            auto cb = b->getChild(i);
            if(ca->isDtGroundValue() && cb->isDtGroundValue()){
                if(!areDtValuesEqual(ca, cb)) return false;
            }
            else if(ca->isConst() && cb->isConst()){
                if(ca->toString() != cb->toString()) return false;
            }
            else{
                return false;
            }
        }
        return true;
    }

    bool Parser::evaluateDtConstructor(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        // Evaluate all children (constructor arguments)
        std::vector<std::shared_ptr<DAGNode>> eval_args;
        bool changed = false;
        for(size_t i = 0; i < expr->getChildrenSize(); i++) {
            std::shared_ptr<DAGNode> eval = NodeManager::NULL_NODE;
            changed |= evaluate(expr->getChild(i), model, eval);
            eval_args.push_back(eval);
        }
        if(changed) {
            result = getNodeManager()->createNode(expr->getSort(), NODE_KIND::NT_DT_CONSTRUCTOR, expr->getName(), eval_args);
            return true;
        }
        result = expr;
        return false;
    }

    bool Parser::evaluateDtSelector(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        // A selector takes one argument: the DT value
        if(expr->getChildrenSize() != 1) { result = expr; return false; }
        std::shared_ptr<DAGNode> dt_val = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChild(0), model, dt_val);

        // If the DT value is a constructor application, extract the appropriate field
        if(dt_val->isConstructorApp()) {
            auto dt_sort = dt_val->getSort();
            if(dt_sort && dt_sort->hasDtConstructors()) {
                const auto* ctor = dt_sort->getDtConstructorByName(dt_val->getName());
                if(ctor) {
                    const std::string& sel_name = expr->getName();
                    for(size_t i = 0; i < ctor->selectors.size(); i++) {
                        if(ctor->selectors[i].name == sel_name && i < dt_val->getChildrenSize()) {
                            result = dt_val->getChild(i);
                            return true;
                        }
                    }
                }
            }
        }

        if(changed) {
            result = getNodeManager()->createNode(expr->getSort(), NODE_KIND::NT_DT_SELECTOR, expr->getName(), {dt_val});
            return true;
        }
        result = expr;
        return false;
    }

    bool Parser::evaluateDtTester(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        // A tester (is-<ctor>) takes one argument: the DT value
        if(expr->getChildrenSize() != 1) { result = expr; return false; }
        std::shared_ptr<DAGNode> dt_val = NodeManager::NULL_NODE;
        bool changed = evaluate(expr->getChild(0), model, dt_val);

        // If the DT value is a constructor application, check if constructor matches
        if(dt_val->isConstructorApp()) {
            const std::string& tester_name = expr->getName();
            // tester_name is "is-<ctor_name>"
            if(tester_name.size() > 3 && tester_name.rfind("is-", 0) == 0) {
                std::string expected_ctor = tester_name.substr(3);
                if(dt_val->getName() == expected_ctor) {
                    result = mkTrue();
                } else {
                    result = mkFalse();
                }
                return true;
            }
        }

        if(changed) {
            result = getNodeManager()->createNode(expr->getSort(), NODE_KIND::NT_DT_TESTER, expr->getName(), {dt_val});
            return true;
        }
        result = expr;
        return false;
    }

    bool Parser::evaluateMatch(const std::shared_ptr<DAGNode>& expr, const std::shared_ptr<Model>& model, std::shared_ptr<DAGNode>& result) {
        // NT_DT_MATCH: child(0) = scrutinee, then pairs (pattern, body)
        if(expr->getChildrenSize() < 3) { result = expr; return false; }

        // Evaluate the scrutinee
        std::shared_ptr<DAGNode> scrutinee = NodeManager::NULL_NODE;
        evaluate(expr->getChild(0), model, scrutinee);

        // Try each case (children[1]=pattern1, children[2]=body1, ...)
        for(size_t i = 1; i + 1 < expr->getChildrenSize(); i += 2) {
            auto pattern = expr->getChild(i);
            auto body = expr->getChild(i + 1);

            if(pattern->isConstructorApp()) {
                // Constructor pattern: check if scrutinee matches this constructor
                if(scrutinee->isConstructorApp() && scrutinee->getName() == pattern->getName()) {
                    // Bind pattern variables to scrutinee's fields via substitution
                    std::unordered_map<std::string, std::shared_ptr<DAGNode>> subst;
                    for(size_t j = 0; j < pattern->getChildrenSize() && j < scrutinee->getChildrenSize(); j++) {
                        subst[pattern->getChild(j)->getName()] = scrutinee->getChild(j);
                    }
                    auto expanded = substitute(body, subst);
                    return evaluate(expanded, model, result);
                }
            } else {
                // Catch-all variable pattern: bind the variable to the scrutinee
                std::unordered_map<std::string, std::shared_ptr<DAGNode>> subst;
                subst[pattern->getName()] = scrutinee;
                auto expanded = substitute(body, subst);
                return evaluate(expanded, model, result);
            }
        }

        // No case matched, return scrutinee (shouldn't happen with well-formed match)
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


