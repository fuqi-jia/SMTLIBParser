/* -*- Header -*-
 *
 * Operator predicate wrappers (IR API Phase 3)
 * All DAGNode::is*() exposed as free functions with null check.
 * Implementations remain in DAGNode; this file only delegates.
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 */

#ifndef OP_UTILS_HEADER
#define OP_UTILS_HEADER

#include "somtparser/ir/node.h"

namespace SOMTParser {

#define IR_IS(name) inline bool name(Node n) { return n && n->name(); }

IR_IS(isLeaf) IR_IS(isInternal) IR_IS(isNull) IR_IS(isErr) IR_IS(isUnknown)
IR_IS(isCBool) IR_IS(isTrue) IR_IS(isFalse) IR_IS(isConst) IR_IS(isNumeral)
IR_IS(isCInt) IR_IS(isCReal) IR_IS(isCBV) IR_IS(isCFP) IR_IS(isCRoundingMode)
IR_IS(isCRootOfWithInterval) IR_IS(isCRootObj) IR_IS(isCRealAlgebraicNumber) IR_IS(isCStr)
IR_IS(isVBool) IR_IS(isLiteral) IR_IS(isVInt) IR_IS(isVReal) IR_IS(isVBV) IR_IS(isVFP)
IR_IS(isVRoundingMode) IR_IS(isVStr) IR_IS(isTempVar) IR_IS(isQuantVar) IR_IS(isLetBindVar)
IR_IS(isPlaceholderVar) IR_IS(isVar) IR_IS(isMax) IR_IS(isMin)
IR_IS(isArray) IR_IS(isConstArray) IR_IS(isAssignableVar)
IR_IS(isAnd) IR_IS(isOr) IR_IS(isNot) IR_IS(isImplies) IR_IS(isXor)
IR_IS(isEqBool) IR_IS(isEqOther) IR_IS(isEq) IR_IS(isDistinctBool) IR_IS(isDistinctOther)
IR_IS(isDistinct) IR_IS(isNeq) IR_IS(isUFApplication)
IR_IS(isConstructorApp) IR_IS(isSelectorApp) IR_IS(isTesterApp)
IR_IS(isAdd) IR_IS(isSub) IR_IS(isMul) IR_IS(isNeg) IR_IS(isDivInt) IR_IS(isDivReal)
IR_IS(isMod) IR_IS(isAbs) IR_IS(isCeil) IR_IS(isFloor) IR_IS(isRound) IR_IS(isArithOp)
IR_IS(isIAnd) IR_IS(isPow2) IR_IS(isPow) IR_IS(isSqrt) IR_IS(isSafeSqrt) IR_IS(isRealNonlinearOp)
IR_IS(isExp) IR_IS(isLog) IR_IS(isLn) IR_IS(isLb) IR_IS(isLg)
IR_IS(isSin) IR_IS(isCos) IR_IS(isSec) IR_IS(isCsc) IR_IS(isTan) IR_IS(isCot)
IR_IS(isAsin) IR_IS(isAcos) IR_IS(isAsec) IR_IS(isAcsc) IR_IS(isAtan) IR_IS(isAcot)
IR_IS(isSinh) IR_IS(isCosh) IR_IS(isTanh) IR_IS(isSech) IR_IS(isCsch) IR_IS(isCoth)
IR_IS(isAsinh) IR_IS(isAcosh) IR_IS(isAtanh) IR_IS(isAsech) IR_IS(isAcsch) IR_IS(isAcoth)
IR_IS(isAtan2) IR_IS(isTranscendentalOp)
IR_IS(isLe) IR_IS(isLt) IR_IS(isGe) IR_IS(isGt) IR_IS(isArithTerm) IR_IS(isArithComp)
IR_IS(isToReal) IR_IS(isToInt) IR_IS(isArithConv)
IR_IS(isInt) IR_IS(isDivisible) IR_IS(isPrime) IR_IS(isEven) IR_IS(isOdd)
IR_IS(isArithProp) IR_IS(isArithAtom)
IR_IS(isPi) IR_IS(isE) IR_IS(isInfinity) IR_IS(isPosInfinity) IR_IS(isNegInfinity)
IR_IS(isNaN) IR_IS(isEpsilon) IR_IS(isPosEpsilon) IR_IS(isNegEpsilon)
IR_IS(isGcd) IR_IS(isLcm) IR_IS(isFact)
IR_IS(isBVNot) IR_IS(isBVAnd) IR_IS(isBVOr) IR_IS(isBVXor) IR_IS(isBVNand) IR_IS(isBVNor)
IR_IS(isBVXnor) IR_IS(isBVComp) IR_IS(isBVNeg) IR_IS(isBVAdd) IR_IS(isBVSub) IR_IS(isBVMul)
IR_IS(isBVUDiv) IR_IS(isBVURem) IR_IS(isBVSDiv) IR_IS(isBVSRem) IR_IS(isBVUMod) IR_IS(isBVSMod)
IR_IS(isBVNegO) IR_IS(isBVUAddO) IR_IS(isBVSAddO) IR_IS(isBVUMulO) IR_IS(isBVSMulO)
IR_IS(isBVUDivO) IR_IS(isBVSDivO) IR_IS(isBVURemO) IR_IS(isBVSRemO) IR_IS(isBVUModO) IR_IS(isBVSModO)
IR_IS(isBVShl) IR_IS(isBVLSHR) IR_IS(isBVASHR) IR_IS(isBVConcat) IR_IS(isBVExtract) IR_IS(isBVRepeat)
IR_IS(isBVZeroExt) IR_IS(isBVSignExt) IR_IS(isBVRotLeft) IR_IS(isBVRotRight) IR_IS(isBVOp)
IR_IS(isBVUlt) IR_IS(isBVUle) IR_IS(isBVUgt) IR_IS(isBVUge)
IR_IS(isBVSlt) IR_IS(isBVSle) IR_IS(isBVSgt) IR_IS(isBVSge)
IR_IS(isBVTerm) IR_IS(isBVCompOp) IR_IS(isBVAtom)
IR_IS(isBVToNat) IR_IS(isNatToBV) IR_IS(isBVToInt) IR_IS(isIntToBV) IR_IS(isBVConv)
IR_IS(isFPAdd) IR_IS(isFPSub) IR_IS(isFPMul) IR_IS(isFPDiv) IR_IS(isFPAbs) IR_IS(isFPNeg)
IR_IS(isFPRem) IR_IS(isFPFMA) IR_IS(isFPSqrt) IR_IS(isFPRoundToIntegral) IR_IS(isFPRoToInt)
IR_IS(isFPMin) IR_IS(isFPMax) IR_IS(isFPOp)
IR_IS(isFPLe) IR_IS(isFPLt) IR_IS(isFPGe) IR_IS(isFPGt) IR_IS(isFPEq) IR_IS(isFPComp)
IR_IS(isFPToUBV) IR_IS(isFPToSBV) IR_IS(isFPToReal) IR_IS(isToFP) IR_IS(isToFPUnsigned) IR_IS(isFPConv)
IR_IS(isFPTerm) IR_IS(isFPIsNormal) IR_IS(isFPIsSubnormal) IR_IS(isFPIsZero) IR_IS(isFPIsInf)
IR_IS(isFPIsNaN) IR_IS(isFPIsNeg) IR_IS(isFPIsPos) IR_IS(isFPProp) IR_IS(isFPAtom)
IR_IS(isSelect) IR_IS(isStore) IR_IS(isArrayOp)
IR_IS(isStrLen) IR_IS(isStrConcat) IR_IS(isStrSubstr) IR_IS(isStrPrefixof) IR_IS(isStrSuffixof)
IR_IS(isStrIndexof) IR_IS(isStrCharat) IR_IS(isStrUpdate) IR_IS(isStrReplace) IR_IS(isStrReplaceAll)
IR_IS(isStrToLower) IR_IS(isStrToUpper) IR_IS(isStrRev) IR_IS(isStrSplit) IR_IS(isStrSplitAt)
IR_IS(isStrSplitRest) IR_IS(isStrNumSplits) IR_IS(isStrSplitAtRe) IR_IS(isStrSplitRestRe) IR_IS(isStrNumSplitsRe)
IR_IS(isStrOp) IR_IS(isStrLt) IR_IS(isStrLe) IR_IS(isStrGt) IR_IS(isStrGe) IR_IS(isStrEq) IR_IS(isStrComp)
IR_IS(isStrInReg) IR_IS(isStrContains) IR_IS(isStrIsDigit) IR_IS(isStrProp) IR_IS(isStrAtom)
IR_IS(isStrFromInt) IR_IS(isStrToInt) IR_IS(isStrToReg) IR_IS(isStrToCode) IR_IS(isStrFromCode) IR_IS(isStrConv)
IR_IS(isRegNone) IR_IS(isRegAll) IR_IS(isRegAllChar) IR_IS(isRegConcat) IR_IS(isRegUnion) IR_IS(isRegInter)
IR_IS(isRegDiff) IR_IS(isRegStar) IR_IS(isRegPlus) IR_IS(isRegOpt) IR_IS(isRegRange)
IR_IS(isRegRepeat) IR_IS(isRegLoop) IR_IS(isRegComplement)
IR_IS(isAtom) IR_IS(isLet) IR_IS(isLetChain) IR_IS(isLetBindVarList) IR_IS(isIte)
IR_IS(isFuncDec) IR_IS(isFuncDef) IR_IS(isFuncRec) IR_IS(isFuncParam)
IR_IS(isFuncApplication) IR_IS(isFuncRecApplication)

#undef IR_IS

}  // namespace SOMTParser

#endif /* OP_UTILS_HEADER */
