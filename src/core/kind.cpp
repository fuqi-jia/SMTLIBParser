/* -*- Source -*-
 *
 * The Kind Enumeration
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

#include "somtparser/core/kind.h"
#include <iostream>

namespace SOMTParser{

    std::string kindToString(const NODE_KIND& nk){
        switch (nk)
        {
        case NODE_KIND::NT_UNKNOWN:
            return "UNKNOWN_KIND";
        case NODE_KIND::NT_ERROR:
            return "ERROR";
        case NODE_KIND::NT_NULL:
            return "NULL";
        case NODE_KIND::NT_CONST_TRUE:
            return "true";
        case NODE_KIND::NT_CONST_FALSE:
            return "false";
        case NODE_KIND::NT_CONST_ARRAY:
            return "const_array";
        case NODE_KIND::NT_PLACEHOLDER_VAR:
            return "placeholder_var";
        // CORE OPERATORS
        case NODE_KIND::NT_EQ:
            return "=";
        case NODE_KIND::NT_DISTINCT:
            return "distinct";
        case NODE_KIND::NT_EQ_BOOL:
            return "=";
        case NODE_KIND::NT_EQ_OTHER:
            return "=";
        case NODE_KIND::NT_DISTINCT_BOOL:
            return "distinct";
        case NODE_KIND::NT_DISTINCT_OTHER:
            return "distinct";
        // CONSTANT / VARIABLE
        case NODE_KIND::NT_CONST:
            return "CONST";
        case NODE_KIND::NT_VAR:
            return "VAR";
        case NODE_KIND::NT_TEMP_VAR:
            return "TEMP_VAR";
        // BOOLEAN
        case NODE_KIND::NT_NOT:
            return "not";
        case NODE_KIND::NT_AND:
            return "and";
        case NODE_KIND::NT_OR:
            return "or";
        case NODE_KIND::NT_IMPLIES:
            return "=>";
        case NODE_KIND::NT_XOR:
            return "xor";
        // UF
        case NODE_KIND::NT_UF_APPLY:
            return "UF_APPLY";
        // ARITHMATIC COMMON OPERATORS
        case NODE_KIND::NT_ADD:
            return "+";
        case NODE_KIND::NT_MUL:
            return "*";
        case NODE_KIND::NT_IAND:
            return "iand";
        case NODE_KIND::NT_POW2:
            return "pow2";
        case NODE_KIND::NT_POW:
            return "^";
        case NODE_KIND::NT_SUB:
            return "-";
        case NODE_KIND::NT_NEG:
            return "-";
        case NODE_KIND::NT_DIV_INT:
            return "div";
        case NODE_KIND::NT_DIV_REAL:
            return "/";
        case NODE_KIND::NT_MOD:
            return "mod";
        case NODE_KIND::NT_ABS:
            return "abs";
        case NODE_KIND::NT_SQRT:
            return "sqrt";
        case NODE_KIND::NT_SAFESQRT:
            return "safesqrt";
        case NODE_KIND::NT_CEIL:
            return "ceil";
        case NODE_KIND::NT_FLOOR:
            return "floor";
        case NODE_KIND::NT_ROUND:
            return "round";
        // TRANSCENDENTAL ARITHMATIC
        case NODE_KIND::NT_EXP:
            return "exp";
        case NODE_KIND::NT_LOG:
            return "log";
        case NODE_KIND::NT_LN:
            return "ln";
        case NODE_KIND::NT_LG:
            return "lg";
        case NODE_KIND::NT_LB:
            return "lb";
        case NODE_KIND::NT_SIN:
            return "sin";
        case NODE_KIND::NT_COS:
            return "cos";
        case NODE_KIND::NT_SEC:
            return "sec";
        case NODE_KIND::NT_CSC:
            return "csc";
        case NODE_KIND::NT_TAN:
            return "tan";
        case NODE_KIND::NT_COT:
            return "cot";
        case NODE_KIND::NT_ASIN:
            return "asin";
        case NODE_KIND::NT_ACOS:
            return "acos";
        case NODE_KIND::NT_ASEC:
            return "asec";
        case NODE_KIND::NT_ACSC:
            return "acsc";
        case NODE_KIND::NT_ATAN:
            return "atan";
        case NODE_KIND::NT_ACOT:
            return "acot";
        case NODE_KIND::NT_SINH:
            return "sinh";
        case NODE_KIND::NT_COSH:
            return "cosh";
        case NODE_KIND::NT_TANH:
            return "tanh";
        case NODE_KIND::NT_SECH:
            return "sech";
        case NODE_KIND::NT_CSCH:
            return "csch";
        case NODE_KIND::NT_COTH:
            return "coth";
        case NODE_KIND::NT_ASINH:
            return "asinh";
        case NODE_KIND::NT_ACOSH:
            return "acosh";
        case NODE_KIND::NT_ATANH:
            return "atanh";
        case NODE_KIND::NT_ASECH:
            return "asech";
        case NODE_KIND::NT_ACSCH:
            return "acsch";
        case NODE_KIND::NT_ACOTH:
            return "acoth";
        case NODE_KIND::NT_ATAN2:
            return "atan2";
        case NODE_KIND::NT_LE:
            return "<=";
        case NODE_KIND::NT_LT:
            return "<";
        case NODE_KIND::NT_GE:
            return ">=";
        case NODE_KIND::NT_GT:
            return ">";
        // ARITHMATIC CONVERSION
        case NODE_KIND::NT_TO_INT:
            return "to_int";
        case NODE_KIND::NT_TO_REAL:
            return "to_real";
        // ARITHMATIC PROPERTIES
        case NODE_KIND::NT_IS_INT:
            return "is_int";
        case NODE_KIND::NT_IS_DIVISIBLE:
            return "is_divisible";
        case NODE_KIND::NT_IS_PRIME:
            return "is_prime";
        case NODE_KIND::NT_IS_EVEN:
            return "is_even";
        case NODE_KIND::NT_IS_ODD:
            return "is_odd";
        // ARITHMATIC CONSTANTS
        case NODE_KIND::NT_CONST_PI:
            return "pi";
        case NODE_KIND::NT_CONST_E:
            return "e";
        case NODE_KIND::NT_INFINITY:
            return "inf";
        case NODE_KIND::NT_POS_INFINITY:
            return "+inf";
        case NODE_KIND::NT_NEG_INFINITY:
            return "-inf";
        case NODE_KIND::NT_NAN:
            return "NaN";
        case NODE_KIND::NT_EPSILON:
            return "epsilon";
        case NODE_KIND::NT_POS_EPSILON:
            return "+epsilon";
        case NODE_KIND::NT_NEG_EPSILON:
            return "-epsilon";
        // ARITHMATIC FUNCTIONS
        // case NODE_KIND::NT_SUM:
        //     return "sum";
        // case NODE_KIND::NT_PROD:
        //     return "prod";
        case NODE_KIND::NT_GCD:
            return "gcd";
        case NODE_KIND::NT_LCM:
            return "lcm";
        case NODE_KIND::NT_FACT:
            return "factorial";
        // BITVECTOR COMMON OPERATORS
        // Bit-wise operations
        case NODE_KIND::NT_BV_NOT:
            return "bvnot";
        case NODE_KIND::NT_BV_AND:
            return "bvand";
        case NODE_KIND::NT_BV_OR:
            return "bvor";
        case NODE_KIND::NT_BV_XOR:
            return "bvxor";
        case NODE_KIND::NT_BV_NAND:
            return "bvnand";
        case NODE_KIND::NT_BV_NOR:
            return "bvnor";
        case NODE_KIND::NT_BV_XNOR:
            return "bvxnor";
        case NODE_KIND::NT_BV_COMP:
            return "bvcomp";
        // Arithmetic operations
        case NODE_KIND::NT_BV_NEG:
            return "bvneg";
        case NODE_KIND::NT_BV_ADD:
            return "bvadd";
        case NODE_KIND::NT_BV_SUB:
            return "bvsub";
        case NODE_KIND::NT_BV_MUL:
            return "bvmul";
        case NODE_KIND::NT_BV_UDIV:
            return "bvudiv";
        case NODE_KIND::NT_BV_SDIV:
            return "bvsdiv";
        case NODE_KIND::NT_BV_UREM:
            return "bvurem";
        case NODE_KIND::NT_BV_SREM:
            return "bvsrem";
        case NODE_KIND::NT_BV_UMOD:
            return "bvumod";
        case NODE_KIND::NT_BV_SMOD:
            return "bvsmod";
        // Arithmetic operations with overflow
        case NODE_KIND::NT_BV_NEGO:
            return "bvnego";
        case NODE_KIND::NT_BV_SADDO:
            return "bvsaddo";
        case NODE_KIND::NT_BV_UADDO:
            return "bvuaddo";
        case NODE_KIND::NT_BV_SMULO:
            return "bvsmulo";
        case NODE_KIND::NT_BV_UMULO:
            return "bvumulo";
        case NODE_KIND::NT_BV_SDIVO:
            return "bvsdivo";
        case NODE_KIND::NT_BV_UDIVO:
            return "bvudivo";
        case NODE_KIND::NT_BV_SREMO:
            return "bvsremo";
        case NODE_KIND::NT_BV_UREMO:
            return "bvuremo";
        case NODE_KIND::NT_BV_SMODO:
            return "bvsmodo";
        case NODE_KIND::NT_BV_UMODO:
            return "bvumodo";
        // Shift operations
        case NODE_KIND::NT_BV_SHL:
            return "bvshl";
        case NODE_KIND::NT_BV_LSHR:
            return "bvlshr";
        case NODE_KIND::NT_BV_ASHR:
            return "bvashr";
        // Function
        case NODE_KIND::NT_BV_CONCAT:
            return "concat";
        case NODE_KIND::NT_BV_EXTRACT:
            return "extract";
        case NODE_KIND::NT_BV_REPEAT:
            return "repeat";
        case NODE_KIND::NT_BV_ZERO_EXT:
            return "zero_extend";
        case NODE_KIND::NT_BV_SIGN_EXT:
            return "sign_extend";
        case NODE_KIND::NT_BV_ROTATE_LEFT:
            return "rotate_left";
        case NODE_KIND::NT_BV_ROTATE_RIGHT:
            return "rotate_right";
        // BITVECTOR COMP
        case NODE_KIND::NT_BV_ULT:
            return "bvult";
        case NODE_KIND::NT_BV_ULE:
            return "bvule";
        case NODE_KIND::NT_BV_UGT:
            return "bvugt";
        case NODE_KIND::NT_BV_UGE:
            return "bvuge";
        case NODE_KIND::NT_BV_SLT:
            return "bvslt";
        case NODE_KIND::NT_BV_SLE:
            return "bvsle";
        case NODE_KIND::NT_BV_SGT:
            return "bvsgt";
        case NODE_KIND::NT_BV_SGE:
            return "bvsge";
        // BITVECTOR CONVERSION
        case NODE_KIND::NT_BV_TO_NAT:
            return "bv2nat";
        case NODE_KIND::NT_NAT_TO_BV:
            return "nat2bv";
        // FLOATING POINT COMMON OPERATORS
        case NODE_KIND::NT_FP_ADD:
            return "fp.add";
        case NODE_KIND::NT_FP_SUB:
            return "fp.sub";
        case NODE_KIND::NT_FP_MUL:
            return "fp.mul";
        case NODE_KIND::NT_FP_DIV:
            return "fp.div";
        case NODE_KIND::NT_FP_ABS:
            return "fp.abs";
        case NODE_KIND::NT_FP_NEG:
            return "fp.neg";
        case NODE_KIND::NT_FP_REM:
            return "fp.rem";
        case NODE_KIND::NT_FP_FMA:
            return "fp.fma";
        case NODE_KIND::NT_FP_SQRT:
            return "fp.sqrt";
        case NODE_KIND::NT_FP_ROUND_TO_INTEGRAL:
            return "fp.roundToIntegral";
        case NODE_KIND::NT_FP_MIN:
            return "fp.min";
        case NODE_KIND::NT_FP_MAX:
            return "fp.max";
        // FLOATING POINT COMP
        case NODE_KIND::NT_FP_LE:
            return "fp.leq";
        case NODE_KIND::NT_FP_LT:
            return "fp.lt";
        case NODE_KIND::NT_FP_GE:
            return "fp.geq";
        case NODE_KIND::NT_FP_GT:
            return "fp.gt";
        case NODE_KIND::NT_FP_EQ:
            return "fp.eq";
        // FLOATING POINT CONVERSION
        case NODE_KIND::NT_FP_TO_UBV:
            return "fp.to_ubv";
        case NODE_KIND::NT_FP_TO_SBV:
            return "fp.to_sbv";
        case NODE_KIND::NT_FP_TO_REAL:
            return "fp.to_real";
        case NODE_KIND::NT_FP_TO_FP:
            return "to_fp";
        case NODE_KIND::NT_FP_TO_FP_UNSIGNED:
            return "to_fp_unsigned";

        // FLOATING POINT PROPERTIES
        case NODE_KIND::NT_FP_IS_NORMAL:
            return "fp.isNormal";
        case NODE_KIND::NT_FP_IS_SUBNORMAL:
            return "fp.isSubnormal";
        case NODE_KIND::NT_FP_IS_ZERO:
            return "fp.isZero";
        case NODE_KIND::NT_FP_IS_INF:
            return "fp.isInfinite";
        case NODE_KIND::NT_FP_IS_NAN:
            return "fp.isNaN";
        case NODE_KIND::NT_FP_IS_NEG:
            return "fp.isNegative";
        case NODE_KIND::NT_FP_IS_POS:
            return "fp.isPositive";
        // ARRAY
        case NODE_KIND::NT_SELECT:
            return "select";
        case NODE_KIND::NT_STORE:
            return "store";
        // STRINGS COMMON OPERATORS
        case NODE_KIND::NT_STR_LEN:
            return "str.len";
        case NODE_KIND::NT_STR_CONCAT:
            return "str.++";
        case NODE_KIND::NT_STR_SUBSTR:
            return "str.substr";
        case NODE_KIND::NT_STR_PREFIXOF:
            return "str.prefixof";
        case NODE_KIND::NT_STR_SUFFIXOF:
            return "str.suffixof";
        case NODE_KIND::NT_STR_INDEXOF:
            return "str.indexof";
        case NODE_KIND::NT_STR_CHARAT: 
            return "str.at";
        case NODE_KIND::NT_STR_UPDATE:
            return "str.update";
        case NODE_KIND::NT_STR_REPLACE:
            return "str.replace";
        case NODE_KIND::NT_STR_REPLACE_ALL:
            return "str.replace_all";
        case NODE_KIND::NT_STR_TO_LOWER:
            return "str.to_lower";
        case NODE_KIND::NT_STR_TO_UPPER:
            return "str.to_upper";
        case NODE_KIND::NT_STR_REV:
            return "str.rev";
        case NODE_KIND::NT_STR_SPLIT:
            return "str.split";
        case NODE_KIND::NT_STR_SPLIT_AT:
            return "str.split_at";
        case NODE_KIND::NT_STR_SPLIT_REST:
            return "str.split_rest";
        case NODE_KIND::NT_STR_NUM_SPLITS:
            return "str.num_splits";
        case NODE_KIND::NT_STR_SPLIT_AT_RE:
            return "str.split_at_re";
        case NODE_KIND::NT_STR_SPLIT_REST_RE:
            return "str.split_rest_re";
        case NODE_KIND::NT_STR_NUM_SPLITS_RE:
            return "str.num_splits_re";
        // STRINGS COMP
        case NODE_KIND::NT_STR_LT:
            return "str.<";
        case NODE_KIND::NT_STR_LE:
            return "str.<=";
        case NODE_KIND::NT_STR_GT:
            return "str.>";
        case NODE_KIND::NT_STR_GE:
            return "str.>=";
        // STRINGS PROPERTIES
        case NODE_KIND::NT_STR_IN_REG:
            return "str.in_re";
        case NODE_KIND::NT_STR_CONTAINS:
            return "str.contains";
        case NODE_KIND::NT_STR_IS_DIGIT:
            return "str.is_digit";
        // STRINGS CONVERSION
        case NODE_KIND::NT_STR_FROM_INT:
            return "str.from_int";
        case NODE_KIND::NT_STR_TO_INT:
            return "str.to_int";
        case NODE_KIND::NT_STR_TO_REG:
            return "str.to_re";
        case NODE_KIND::NT_STR_TO_CODE:
            return "str.to_code";
        case NODE_KIND::NT_STR_FROM_CODE:
            return "str.from_code";
        case NODE_KIND::NT_REG_NONE:
            return "re.none";
        case NODE_KIND::NT_REG_ALL:
            return "re.all";
        case NODE_KIND::NT_REG_ALLCHAR:
            return "re.allchar";
        case NODE_KIND::NT_REG_CONCAT:
            return "re.++";
        case NODE_KIND::NT_REG_UNION:
            return "re.union";
        case NODE_KIND::NT_REG_INTER:
            return "re.inter";
        case NODE_KIND::NT_REG_DIFF:
            return "re.diff";
        case NODE_KIND::NT_REG_STAR:
            return "re.*";
        case NODE_KIND::NT_REG_PLUS:
            return "re.+";
        case NODE_KIND::NT_REG_OPT:
            return "re.opt";
        case NODE_KIND::NT_REG_RANGE:
            return "re.range";
        case NODE_KIND::NT_REG_REPEAT:
            return "re.^";
        case NODE_KIND::NT_REG_LOOP:
            return "re.loop";
        case NODE_KIND::NT_REG_COMPLEMENT:
            return "re.comp";
        // STRINGS RE FUNCTIONS
        case NODE_KIND::NT_STR_REPLACE_REG:
            return "str.replace_re";
        case NODE_KIND::NT_STR_REPLACE_REG_ALL:
            return "str.replace_re_all";
        case NODE_KIND::NT_STR_INDEXOF_REG:
            return "str.indexof";
        // INTERVAL
        case NODE_KIND::NT_MAX:
            return "max";
        case NODE_KIND::NT_MIN:
            return "min";
        // LET
        case NODE_KIND::NT_LET:
            return "let";
        case NODE_KIND::NT_LET_CHAIN:
            return "let_chain";
        case NODE_KIND::NT_LET_BIND_VAR:
            return "let_bind_var";
        case NODE_KIND::NT_LET_BIND_VAR_LIST:
            return "let_bind_var_list";
        // ITE
        case NODE_KIND::NT_ITE:
            return "ite";
        // QUANTIFIERS
        case NODE_KIND::NT_FORALL:
            return "forall";
        case NODE_KIND::NT_EXISTS:
            return "exists";
        case NODE_KIND::NT_QUANT_VAR:
            return "quant_var";
        // FUNC
        case NODE_KIND::NT_FUNC_DEC:
            return "func_dec";
        case NODE_KIND::NT_FUNC_DEF:
            return "func_def";
        case NODE_KIND::NT_FUNC_PARAM:
            return "func_param";
        case NODE_KIND::NT_FUNC_APPLY:
            return "func_apply";
        case NODE_KIND::NT_FUNC_REC:
            return "func_rec";
        case NODE_KIND::NT_FUNC_REC_APPLY:
            return "func_rec_apply";
        default:
            return "UNKNOWN_KIND";
        }
    }
    NODE_KIND getNegatedKind(const NODE_KIND& nk){
        switch (nk)
        {
            case NODE_KIND::NT_EQ:
                return NODE_KIND::NT_DISTINCT;
            case NODE_KIND::NT_DISTINCT:
                return NODE_KIND::NT_EQ;
            case NODE_KIND::NT_EQ_BOOL:
                return NODE_KIND::NT_DISTINCT_BOOL;
            case NODE_KIND::NT_DISTINCT_BOOL:
                return NODE_KIND::NT_EQ_BOOL;
            case NODE_KIND::NT_EQ_OTHER:
                return NODE_KIND::NT_DISTINCT_OTHER;
            case NODE_KIND::NT_DISTINCT_OTHER:
                return NODE_KIND::NT_EQ_OTHER;
            case NODE_KIND::NT_LE:
                return NODE_KIND::NT_GT;
            case NODE_KIND::NT_LT:
                return NODE_KIND::NT_GE;
            case NODE_KIND::NT_GE:
                return NODE_KIND::NT_LT;
            case NODE_KIND::NT_GT:
                return NODE_KIND::NT_LE;
            case NODE_KIND::NT_BV_ULT:
                return NODE_KIND::NT_BV_UGE;
            case NODE_KIND::NT_BV_UGT:
                return NODE_KIND::NT_BV_ULE;
            case NODE_KIND::NT_BV_ULE:
                return NODE_KIND::NT_BV_UGT;
            case NODE_KIND::NT_BV_UGE:
                return NODE_KIND::NT_BV_ULT;
            case NODE_KIND::NT_BV_SLT:
                return NODE_KIND::NT_BV_SGE;
            case NODE_KIND::NT_BV_SGT:
                return NODE_KIND::NT_BV_SLE;
            case NODE_KIND::NT_BV_SLE:
                return NODE_KIND::NT_BV_SGT;
            case NODE_KIND::NT_BV_SGE:
                return NODE_KIND::NT_BV_SLT;
            case NODE_KIND::NT_FP_LE:
                // std::cerr << "Warning: getNegatedKind(fp.leq) returns fp.gt, but note that not fp.leq is NOT equivalent to fp.gt due to NaN semantics." << std::endl;
                return NODE_KIND::NT_FP_GT;
            case NODE_KIND::NT_FP_LT:
                // std::cerr << "Warning: getNegatedKind(fp.lt) returns fp.geq, but note that not fp.lt is NOT equivalent to fp.geq due to NaN semantics." << std::endl;
                return NODE_KIND::NT_FP_GE;
            case NODE_KIND::NT_FP_GE:
                // std::cerr << "Warning: getNegatedKind(fp.geq) returns fp.lt, but note that not fp.geq is NOT equivalent to fp.lt due to NaN semantics." << std::endl;
                return NODE_KIND::NT_FP_LT;
            case NODE_KIND::NT_FP_GT:
                // std::cerr << "Warning: getNegatedKind(fp.gt) returns fp.leq, but note that not fp.gt is NOT equivalent to fp.leq due to NaN semantics." << std::endl;
                return NODE_KIND::NT_FP_LE;
            case NODE_KIND::NT_FP_EQ:
                // std::cerr << "Warning: getNegatedKind(fp.eq) - fp.eq cannot be negated to fp.ne (which doesn't exist in SMT-LIB) due to NaN semantics." << std::endl;
                return NODE_KIND::NT_UNKNOWN;
            case NODE_KIND::NT_STR_LT:
                return NODE_KIND::NT_STR_GE;
            case NODE_KIND::NT_STR_GT:
                return NODE_KIND::NT_STR_LE;
            case NODE_KIND::NT_STR_LE:
                return NODE_KIND::NT_STR_GT;
            case NODE_KIND::NT_STR_GE:
                return NODE_KIND::NT_STR_LT;
            default:
                return NODE_KIND::NT_UNKNOWN;
        }
    }
    NODE_KIND getFlipKind(const NODE_KIND& nk){
        switch (nk)
        {
            case NODE_KIND::NT_EQ:
                return NODE_KIND::NT_EQ;
            case NODE_KIND::NT_DISTINCT:
                return NODE_KIND::NT_DISTINCT;
            case NODE_KIND::NT_EQ_BOOL:
                return NODE_KIND::NT_EQ_BOOL;
            case NODE_KIND::NT_DISTINCT_BOOL:
                return NODE_KIND::NT_DISTINCT_BOOL;
            case NODE_KIND::NT_EQ_OTHER:
                return NODE_KIND::NT_EQ_OTHER;
            case NODE_KIND::NT_DISTINCT_OTHER:
                return NODE_KIND::NT_DISTINCT_OTHER;
            case NODE_KIND::NT_LE:
                return NODE_KIND::NT_GE;
            case NODE_KIND::NT_LT:
                return NODE_KIND::NT_GT;
            case NODE_KIND::NT_GE:
                return NODE_KIND::NT_LE;
            case NODE_KIND::NT_GT:
                return NODE_KIND::NT_LT;
            case NODE_KIND::NT_ADD:
                return NODE_KIND::NT_SUB;
            case NODE_KIND::NT_SUB:
                return NODE_KIND::NT_ADD;
            case NODE_KIND::NT_DIV_INT:
                return NODE_KIND::NT_MUL;
            case NODE_KIND::NT_DIV_REAL:
                return NODE_KIND::NT_MUL;
            case NODE_KIND::NT_BV_SUB:
                return NODE_KIND::NT_BV_ADD;
            case NODE_KIND::NT_BV_ADD:
                return NODE_KIND::NT_BV_SUB;
            case NODE_KIND::NT_BV_UDIV:
                return NODE_KIND::NT_BV_MUL;
            case NODE_KIND::NT_BV_SDIV:
                return NODE_KIND::NT_BV_MUL;
            case NODE_KIND::NT_BV_ULT:
                return NODE_KIND::NT_BV_UGT;
            case NODE_KIND::NT_BV_UGT:
                return NODE_KIND::NT_BV_ULT;
            case NODE_KIND::NT_BV_ULE:
                return NODE_KIND::NT_BV_UGE;
            case NODE_KIND::NT_BV_UGE:
                return NODE_KIND::NT_BV_ULE;
            case NODE_KIND::NT_BV_SLT:
                return NODE_KIND::NT_BV_SGE;
            case NODE_KIND::NT_BV_SGT:
                return NODE_KIND::NT_BV_SLT;
            case NODE_KIND::NT_BV_SLE:
                return NODE_KIND::NT_BV_SGE;
            case NODE_KIND::NT_BV_SGE:
                return NODE_KIND::NT_BV_SLE;
            case NODE_KIND::NT_FP_ADD:
                return NODE_KIND::NT_FP_SUB;
            case NODE_KIND::NT_FP_SUB:
                return NODE_KIND::NT_FP_ADD;
            case NODE_KIND::NT_FP_MUL:
                return NODE_KIND::NT_FP_DIV;
            case NODE_KIND::NT_FP_DIV:
                return NODE_KIND::NT_FP_MUL;
            case NODE_KIND::NT_FP_LE:
                return NODE_KIND::NT_FP_GE;
            case NODE_KIND::NT_FP_LT:
                return NODE_KIND::NT_FP_GT;
            case NODE_KIND::NT_FP_GE:
                return NODE_KIND::NT_FP_LE;
            case NODE_KIND::NT_FP_GT:
                return NODE_KIND::NT_FP_LT;
            case NODE_KIND::NT_STR_LT:
                return NODE_KIND::NT_STR_GT;
            case NODE_KIND::NT_STR_GT:
                return NODE_KIND::NT_STR_LT;
            case NODE_KIND::NT_STR_LE:
                return NODE_KIND::NT_STR_GE;
            case NODE_KIND::NT_STR_GE:
                return NODE_KIND::NT_STR_LE;
            default:
                return NODE_KIND::NT_UNKNOWN;
        }
    }

    NODE_KIND getOperKind(const std::string& s) {
        auto it = oper_key_map.find(s);
        if (it != oper_key_map.end()) {
            return it->second;
        }
        return NODE_KIND::NT_UNKNOWN;
    }
}
