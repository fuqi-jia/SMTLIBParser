// II-2a native mapping implementation.
#include "somtparser/arena/map.h"
#include "somtarena/Payload.h"

#include <cctype>
#include <string>

namespace xarena_cov {

// II-2b-3 (endgame step3): value+name overload — maps a THREADED value (+ the node's name, needed
// only to recover the BV literal) so the inline arena builder can produce a Const payload WITHOUT
// reading the DAGNode value/name fields. The DAGNode-taking overload below delegates here, sourcing
// value+name from its fields (used by the deferred non-NRA walk); the inline hook threads them in and
// calls this overload directly (field-free). Verdict-neutral: threaded value/name == the fields.
somtarena::Payload mapValue(const std::shared_ptr<SOMTParser::Value>& val,
                            const std::string& name, GapSink& g) {
    if (!val) return somtarena::payloadNone();
    switch (val->getType()) {
        case SOMTParser::BOOLEAN:
            return somtarena::payloadBool(val->getBooleanValue());
        case SOMTParser::STRING:
            return somtarena::payloadString(val->getStringValue());
        case SOMTParser::NUMBER: {
            SOMTParser::Number num = val->getNumberValue();
            switch (num.getType()) {
                case SOMTParser::Number::INT_TYPE:
                    return somtarena::payloadInt(num.getInteger().getMPZ());
                case SOMTParser::Number::RATIONAL_TYPE:
                    return somtarena::payloadRational(num.getRational().getMPQ());
                case SOMTParser::Number::REAL_TYPE:
                    // SMT-LIB decimal literals are EXACT rationals (3.7 == 37/10); the parser
                    // types them REAL_TYPE (MPFR), but approximateToRational recovers the value
                    // from realValue.toString() — the SAME 17-sig-digit path the adapter feeds
                    // to Xolver (extractPayload -> num.toString()), so it is verdict-identical.
                    return somtarena::payloadRational(num.approximateToRational().getMPQ());
                default:
                    g.hardGap("unknown Number type");
                    return somtarena::payloadNone();
            }
        }
        case SOMTParser::BV: {
            unsigned w = static_cast<unsigned>(val->getBvWidth());
            // The BV literal lives in the node name (#b../#x../decimal). Best-effort parse
            // to u64 for coverage; exact >64-bit BV fidelity is II-2b's concern.
            unsigned long long bits = 0;
            const std::string& nm = name;  // II-2b-3 (step3): threaded name, not the DAGNode field
            try {
                if (nm.size() > 2 && nm[0] == '#' && nm[1] == 'b')
                    bits = std::stoull(nm.substr(2), nullptr, 2);
                else if (nm.size() > 2 && nm[0] == '#' && nm[1] == 'x')
                    bits = std::stoull(nm.substr(2), nullptr, 16);
                else if (!nm.empty() && std::isdigit(static_cast<unsigned char>(nm[0])))
                    bits = std::stoull(nm);
                else
                    g.softGap("BV literal not u64-parsable: " + nm);
            } catch (...) {
                g.softGap("BV literal overflow >u64: " + nm);
            }
            return somtarena::payloadBitVec(static_cast<std::uint64_t>(bits), w);
        }
        default:
            g.hardGap("unmapped value type");
            return somtarena::payloadNone();
    }
}

somtarena::Payload mapValue(const SOMTParser::DAGNode& n, GapSink& g) {
    // II-2b-3 (endgame step3): delegate to the value+name overload, sourcing both from this node's
    // authoritative fields. This DAGNode-taking form is the deferred non-NRA walk's path; the inline
    // builder threads value+name and calls the overload above directly (field-free).
    return mapValue(n.getValueRaw(), n.getNameRaw(), g);  // II-2b-3 (P3.b/P3.e): fields, not the registry
}

somtarena::SortId mapSort(const std::shared_ptr<SOMTParser::Sort>& s,
                          somtarena::Arena& a, GapSink& g) {
    if (!s) { g.hardGap("null sort"); return somtarena::NullExpr; }
    if (s->isBool()) return a.boolSort();
    if (s->isInt())  return a.intSort();
    if (s->isReal()) return a.realSort();
    if (s->isIntOrReal()) {
        // SOMTParser's ambiguous numeric-literal sort. Default to Int (as the adapter
        // does); the exact int-vs-real resolution is context-dependent (II-2b concern).
        g.softGap("IntOrReal defaulted to Int (II-2b resolves by context)");
        return a.intSort();
    }
    if (s->isBv())   return a.bitVecSort(static_cast<unsigned>(s->getBitWidth()));
    if (s->isArray()) {
        somtarena::SortId idx = mapSort(s->getIndexSort(), a, g);
        somtarena::SortId el  = mapSort(s->getElemSort(),  a, g);
        return a.arraySort(idx, el);
    }
    if (s->isStr()) return a.stringSort();
    if (s->isRoundingMode()) return a.roundingModeSort();
    if (s->isDatatype()) {
        somtarena::SortId id = a.datatypeSort(s->toString(), {});
        g.dtSorts[id] = s;  // II-2b-2: carry the Sort (constructors/selectors) for the Xolver import
        return id;
    }
    // Uninterpreted / declared / defined sorts (declare-sort E 0, etc.). The parser may
    // also wrap the builtins as SK_DEC arity-0 — canonicalize those, else make an arena
    // uninterpreted sort keyed by name (params for higher-arity declared sorts).
    if (s->isDec() || s->isDef() || s->isUF()) {
        const std::string nm = s->toString();
        if (nm == "Int")  return a.intSort();
        if (nm == "Real") return a.realSort();
        if (nm == "Bool") return a.boolSort();
        return a.uninterpretedSort(nm);
    }
    // FP (width via sort children) is not yet mapped; the coverage run reports whether the
    // corpus exercises it.
    g.hardGap("unmapped sort: " + s->toString());
    return somtarena::NullExpr;
}

somtarena::Kind mapKind(SOMTParser::NODE_KIND k, bool& mapped) {
    using NK = SOMTParser::NODE_KIND;
    using K = somtarena::Kind;
    mapped = true;
    switch (k) {
        // --- constants / vars ---
        case NK::NT_CONST:        return K::Const;
        case NK::NT_VAR:          return K::Var;
        case NK::NT_CONST_TRUE:   return K::True;
        case NK::NT_CONST_FALSE:  return K::False;
        case NK::NT_CONST_ARRAY:  return K::ConstArray;
        // --- boolean / core ---
        case NK::NT_AND:      return K::And;
        case NK::NT_OR:       return K::Or;
        case NK::NT_NOT:      return K::Not;
        case NK::NT_IMPLIES:  return K::Implies;
        case NK::NT_XOR:      return K::Xor;
        case NK::NT_EQ: case NK::NT_EQ_BOOL: case NK::NT_EQ_OTHER:            return K::Eq;
        case NK::NT_DISTINCT: case NK::NT_DISTINCT_BOOL: case NK::NT_DISTINCT_OTHER: return K::Distinct;
        case NK::NT_ITE:      return K::Ite;
        // --- arithmetic ---
        case NK::NT_ADD:      return K::Add;
        case NK::NT_NEG:      return K::Neg;
        case NK::NT_SUB:      return K::Sub;
        case NK::NT_MUL:      return K::Mul;
        case NK::NT_IAND:     return K::IntAnd;
        case NK::NT_POW2:     return K::Pow2;
        case NK::NT_POW:      return K::Pow;
        case NK::NT_DIV_INT:  return K::IntDiv;
        case NK::NT_DIV_REAL: return K::RealDiv;
        case NK::NT_MOD:      return K::Mod;
        case NK::NT_ABS:      return K::Abs;
        case NK::NT_SQRT:     return K::Sqrt;
        case NK::NT_SAFESQRT: return K::SafeSqrt;
        case NK::NT_CEIL:     return K::Ceil;
        case NK::NT_FLOOR:    return K::Floor;
        case NK::NT_ROUND:    return K::Round;
        case NK::NT_GCD:      return K::Gcd;
        case NK::NT_LCM:      return K::Lcm;
        case NK::NT_FACT:     return K::Fact;
        case NK::NT_MAX:      return K::Max;
        case NK::NT_MIN:      return K::Min;
        // --- transcendental ---
        case NK::NT_EXP:  return K::Exp;   case NK::NT_LN:   return K::Ln;
        case NK::NT_LG:   return K::Lg;    case NK::NT_LB:   return K::Lb;
        case NK::NT_LOG:  return K::Log;
        case NK::NT_SIN:  return K::Sin;   case NK::NT_COS:  return K::Cos;
        case NK::NT_TAN:  return K::Tan;   case NK::NT_COT:  return K::Cot;
        case NK::NT_SEC:  return K::Sec;   case NK::NT_CSC:  return K::Csc;
        case NK::NT_ASIN: return K::Asin;  case NK::NT_ACOS: return K::Acos;
        case NK::NT_ATAN: return K::Atan;  case NK::NT_ACOT: return K::Acot;
        case NK::NT_ASEC: return K::Asec;  case NK::NT_ACSC: return K::Acsc;
        case NK::NT_SINH: return K::Sinh;  case NK::NT_COSH: return K::Cosh;
        case NK::NT_TANH: return K::Tanh;  case NK::NT_ASECH: return K::Asech;
        case NK::NT_ACSCH: return K::Acsch; case NK::NT_ACOTH: return K::Acoth;
        case NK::NT_ATAN2: return K::Atan2; case NK::NT_ASINH: return K::Asinh;
        case NK::NT_ACOSH: return K::Acosh; case NK::NT_ATANH: return K::Atanh;
        case NK::NT_SECH: return K::Sech;  case NK::NT_CSCH: return K::Csch;
        case NK::NT_COTH: return K::Coth;
        // --- predicates / comparisons / conversions ---
        case NK::NT_IS_DIVISIBLE: return K::IsDivisible;
        case NK::NT_IS_PRIME:     return K::IsPrime;
        case NK::NT_IS_EVEN:      return K::IsEven;
        case NK::NT_IS_ODD:       return K::IsOdd;
        case NK::NT_IS_INT:       return K::IsInt;
        case NK::NT_LE: return K::Le;   case NK::NT_LT: return K::Lt;
        case NK::NT_GE: return K::Ge;   case NK::NT_GT: return K::Gt;
        case NK::NT_TO_INT:  return K::ToInt;
        case NK::NT_TO_REAL: return K::ToReal;
        // --- bit-vectors ---
        case NK::NT_BV_NOT: return K::BvNot;   case NK::NT_BV_NEG: return K::BvNeg;
        case NK::NT_BV_AND: return K::BvAnd;   case NK::NT_BV_OR:  return K::BvOr;
        case NK::NT_BV_XOR: return K::BvXor;   case NK::NT_BV_NAND: return K::BvNand;
        case NK::NT_BV_NOR: return K::BvNor;   case NK::NT_BV_XNOR: return K::BvXnor;
        case NK::NT_BV_COMP: return K::BvComp; case NK::NT_BV_ADD: return K::BvAdd;
        case NK::NT_BV_SUB: return K::BvSub;   case NK::NT_BV_MUL: return K::BvMul;
        case NK::NT_BV_UDIV: return K::BvUdiv; case NK::NT_BV_SDIV: return K::BvSdiv;
        case NK::NT_BV_UREM: return K::BvUrem; case NK::NT_BV_SREM: return K::BvSrem;
        case NK::NT_BV_UMOD: return K::BvUmod; case NK::NT_BV_SMOD: return K::BvSmod;
        case NK::NT_BV_SHL: return K::BvShl;   case NK::NT_BV_LSHR: return K::BvLshr;
        case NK::NT_BV_ASHR: return K::BvAshr;
        case NK::NT_BV_ULT: return K::BvUlt;   case NK::NT_BV_ULE: return K::BvUle;
        case NK::NT_BV_UGT: return K::BvUgt;   case NK::NT_BV_UGE: return K::BvUge;
        case NK::NT_BV_SLT: return K::BvSlt;   case NK::NT_BV_SLE: return K::BvSle;
        case NK::NT_BV_SGT: return K::BvSgt;   case NK::NT_BV_SGE: return K::BvSge;
        case NK::NT_BV_CONCAT: return K::BvConcat;
        case NK::NT_BV_TO_NAT: return K::BvToNat; case NK::NT_NAT_TO_BV: return K::NatToBv;
        case NK::NT_BV_TO_INT: return K::BvToInt; case NK::NT_INT_TO_BV: return K::IntToBv;
        case NK::NT_BV_EXTRACT: return K::BvExtract; case NK::NT_BV_REPEAT: return K::BvRepeat;
        case NK::NT_BV_ZERO_EXT: return K::BvZeroExtend; case NK::NT_BV_SIGN_EXT: return K::BvSignExtend;
        case NK::NT_BV_ROTATE_LEFT: return K::BvRotateLeft; case NK::NT_BV_ROTATE_RIGHT: return K::BvRotateRight;
        case NK::NT_BV_NEGO: return K::BvNego; case NK::NT_BV_UADDO: return K::BvUaddo;
        case NK::NT_BV_SADDO: return K::BvSaddo; case NK::NT_BV_UMULO: return K::BvUmulo;
        case NK::NT_BV_SMULO: return K::BvSmulo; case NK::NT_BV_UDIVO: return K::BvUdivo;
        case NK::NT_BV_SDIVO: return K::BvSdivo; case NK::NT_BV_UREMO: return K::BvUremo;
        case NK::NT_BV_SREMO: return K::BvSremo; case NK::NT_BV_UMODO: return K::BvUmodo;
        case NK::NT_BV_SMODO: return K::BvSmodo;
        // --- floating point ---
        case NK::NT_FP_ADD: return K::FpAdd; case NK::NT_FP_SUB: return K::FpSub;
        case NK::NT_FP_MUL: return K::FpMul; case NK::NT_FP_DIV: return K::FpDiv;
        case NK::NT_FP_ABS: return K::FpAbs; case NK::NT_FP_NEG: return K::FpNeg;
        case NK::NT_FP_REM: return K::FpRem; case NK::NT_FP_FMA: return K::FpFma;
        case NK::NT_FP_SQRT: return K::FpSqrt; case NK::NT_FP_ROUND_TO_INTEGRAL: return K::FpRoundToIntegral;
        case NK::NT_FP_MIN: return K::FpMin; case NK::NT_FP_MAX: return K::FpMax;
        case NK::NT_FP_LE: return K::FpLe; case NK::NT_FP_LT: return K::FpLt;
        case NK::NT_FP_GE: return K::FpGe; case NK::NT_FP_GT: return K::FpGt;
        case NK::NT_FP_EQ: return K::FpEq;
        case NK::NT_FP_TO_UBV: return K::FpToUbv; case NK::NT_FP_TO_SBV: return K::FpToSbv;
        case NK::NT_FP_TO_REAL: return K::FpToReal; case NK::NT_FP_TO_FP: return K::FpToFp;
        case NK::NT_FP_TO_FP_UNSIGNED: return K::FpToFpUnsigned; case NK::NT_FP_TO_IEEE_BV: return K::FpToIeeeBv;
        case NK::NT_FP_IS_NORMAL: return K::FpIsNormal; case NK::NT_FP_IS_SUBNORMAL: return K::FpIsSubnormal;
        case NK::NT_FP_IS_ZERO: return K::FpIsZero; case NK::NT_FP_IS_INF: return K::FpIsInf;
        case NK::NT_FP_IS_NAN: return K::FpIsNan; case NK::NT_FP_IS_NEG: return K::FpIsNeg;
        case NK::NT_FP_IS_POS: return K::FpIsPos;
        // --- arrays ---
        case NK::NT_SELECT: return K::Select; case NK::NT_STORE: return K::Store;
        // --- strings ---
        case NK::NT_STR_LEN: return K::StrLen; case NK::NT_STR_CONCAT: return K::StrConcat;
        case NK::NT_STR_SUBSTR: return K::StrSubstr; case NK::NT_STR_INDEXOF: return K::StrIndexof;
        case NK::NT_STR_CHARAT: return K::StrCharat; case NK::NT_STR_UPDATE: return K::StrUpdate;
        case NK::NT_STR_REPLACE: return K::StrReplace; case NK::NT_STR_REPLACE_ALL: return K::StrReplaceAll;
        case NK::NT_STR_REPLACE_REG: return K::StrReplaceRe; case NK::NT_STR_REPLACE_REG_ALL: return K::StrReplaceReAll;
        case NK::NT_STR_INDEXOF_REG: return K::StrIndexofRe; case NK::NT_STR_TO_LOWER: return K::StrToLower;
        case NK::NT_STR_TO_UPPER: return K::StrToUpper; case NK::NT_STR_REV: return K::StrRev;
        case NK::NT_STR_SPLIT: return K::StrSplit; case NK::NT_STR_SPLIT_AT: return K::StrSplitAt;
        case NK::NT_STR_SPLIT_REST: return K::StrSplitRest; case NK::NT_STR_NUM_SPLITS: return K::StrNumSplits;
        case NK::NT_STR_SPLIT_AT_RE: return K::StrSplitAtRe; case NK::NT_STR_SPLIT_REST_RE: return K::StrSplitRestRe;
        case NK::NT_STR_NUM_SPLITS_RE: return K::StrNumSplitsRe; case NK::NT_STR_LT: return K::StrLt;
        case NK::NT_STR_LE: return K::StrLe; case NK::NT_STR_GT: return K::StrGt;
        case NK::NT_STR_GE: return K::StrGe; case NK::NT_STR_IN_REG: return K::StrInRe;
        case NK::NT_STR_CONTAINS: return K::StrContains; case NK::NT_STR_IS_DIGIT: return K::StrIsDigit;
        case NK::NT_STR_PREFIXOF: return K::StrPrefixof; case NK::NT_STR_SUFFIXOF: return K::StrSuffixof;
        case NK::NT_STR_FROM_INT: return K::StrFromInt; case NK::NT_STR_TO_INT: return K::StrToInt;
        case NK::NT_STR_TO_REG: return K::StrToRe; case NK::NT_STR_TO_CODE: return K::StrToCode;
        case NK::NT_STR_FROM_CODE: return K::StrFromCode;
        // --- regex ---
        case NK::NT_REG_CONCAT: return K::ReConcat; case NK::NT_REG_UNION: return K::ReUnion;
        case NK::NT_REG_INTER: return K::ReInter; case NK::NT_REG_DIFF: return K::ReDiff;
        case NK::NT_REG_STAR: return K::ReStar; case NK::NT_REG_PLUS: return K::RePlus;
        case NK::NT_REG_OPT: return K::ReOpt; case NK::NT_REG_RANGE: return K::ReRange;
        case NK::NT_REG_REPEAT: return K::ReRepeat; case NK::NT_REG_COMPLEMENT: return K::ReComplement;
        case NK::NT_REG_LOOP: return K::ReLoop; case NK::NT_REG_NONE: return K::ReNone;
        case NK::NT_REG_ALL: return K::ReAll; case NK::NT_REG_ALLCHAR: return K::ReAllchar;
        // --- functions / UF ---
        case NK::NT_FUNC_APPLY: case NK::NT_UF_APPLY: case NK::NT_FUNC_REC_APPLY: return K::Apply;
        case NK::NT_FUNC_DEC: return K::FuncDecl;
        // --- datatypes ---
        case NK::NT_DT_CONSTRUCTOR: return K::DtConstructor; case NK::NT_DT_SELECTOR: return K::DtSelector;
        case NK::NT_DT_TESTER: return K::DtTester; case NK::NT_DT_MATCH: return K::DtMatch;
        // --- transcendental / special constants ---
        case NK::NT_CONST_PI: return K::Pi;
        case NK::NT_INFINITY: return K::Infinity; case NK::NT_NAN: return K::Nan;
        case NK::NT_EPSILON: return K::Epsilon;
        case NK::NT_POS_INFINITY: return K::PosInfinity; case NK::NT_NEG_INFINITY: return K::NegInfinity;
        case NK::NT_POS_EPSILON: return K::PosEpsilon; case NK::NT_NEG_EPSILON: return K::NegEpsilon;
        // --- real-algebraic (Xolver NRA witnesses) ---
        case NK::NT_ROOT_OBJ: return K::RootObj;
        case NK::NT_ROOT_OF_WITH_INTERVAL: return K::RootOfWithInterval;
        case NK::NT_REAL_ALGEBRAIC_NUMBER: return K::RealAlgebraicNumber;
        // --- quantifiers (mapped, but the WALK builds them in de Bruijn form) ---
        case NK::NT_FORALL: return K::Forall;
        case NK::NT_EXISTS: return K::Exists;
        // --- everything else: scaffolding (NT_LET*/QUANT_VAR/TEMP/FUNC_DEF/PARAM/ERROR/
        //     NULL/UNKNOWN/PLACEHOLDER) the walk resolves, or a genuine gap (e.g. NT_CONST_E,
        //     NT_LG/NT_LB/NT_GCD if absent). Caller records the gap; the returned Kind is
        //     a placeholder (ignored when mapped==false). SOMTArena has no "Unknown" Kind.
        default: mapped = false; return K::Const;
    }
}

SOMTParser::NODE_KIND arenaKindToNodeKind(const somtarena::Arena& a, somtarena::ExprId id) {
    using NK = SOMTParser::NODE_KIND;
    using K = somtarena::Kind;
    // Eq/Distinct collapse: their bool-ness is derived from the FIRST operand's sort (Cap.3 §3
    // dropped the EqBool/EqOther tags from the arena vocabulary). Eq/Distinct are NOT Apply, so
    // the operands are the plain children (no funcDecl child-0 to skip). Empty child list ->
    // safe *_OTHER default.
    auto firstOperandIsBool = [&a](somtarena::ExprId n) -> bool {
        const auto ch = a.children(n);
        return !ch.empty() && a.sortOf(ch[0]) == a.boolSort();
    };
    switch (a.kind(id)) {
        // --- constants / vars ---
        case K::Const:        return NK::NT_CONST;
        case K::Var:          return NK::NT_VAR;
        case K::True:         return NK::NT_CONST_TRUE;
        case K::False:        return NK::NT_CONST_FALSE;
        case K::ConstArray:   return NK::NT_CONST_ARRAY;
        // --- boolean / core ---
        case K::And:      return NK::NT_AND;
        case K::Or:       return NK::NT_OR;
        case K::Not:      return NK::NT_NOT;
        case K::Implies:  return NK::NT_IMPLIES;
        case K::Xor:      return NK::NT_XOR;
        case K::Eq:       return firstOperandIsBool(id) ? NK::NT_EQ_BOOL : NK::NT_EQ_OTHER;
        case K::Distinct: return firstOperandIsBool(id) ? NK::NT_DISTINCT_BOOL : NK::NT_DISTINCT_OTHER;
        case K::Ite:      return NK::NT_ITE;
        // --- arithmetic ---
        case K::Add:      return NK::NT_ADD;
        case K::Neg:      return NK::NT_NEG;
        case K::Sub:      return NK::NT_SUB;
        case K::Mul:      return NK::NT_MUL;
        case K::IntAnd:   return NK::NT_IAND;
        case K::Pow2:     return NK::NT_POW2;
        case K::Pow:      return NK::NT_POW;
        case K::IntDiv:   return NK::NT_DIV_INT;
        case K::RealDiv:  return NK::NT_DIV_REAL;
        case K::Mod:      return NK::NT_MOD;
        case K::Abs:      return NK::NT_ABS;
        case K::Sqrt:     return NK::NT_SQRT;
        case K::SafeSqrt: return NK::NT_SAFESQRT;
        case K::Ceil:     return NK::NT_CEIL;
        case K::Floor:    return NK::NT_FLOOR;
        case K::Round:    return NK::NT_ROUND;
        case K::Gcd:      return NK::NT_GCD;
        case K::Lcm:      return NK::NT_LCM;
        case K::Fact:     return NK::NT_FACT;
        case K::Max:      return NK::NT_MAX;
        case K::Min:      return NK::NT_MIN;
        // --- transcendental ---
        case K::Exp:  return NK::NT_EXP;   case K::Ln:   return NK::NT_LN;
        case K::Lg:   return NK::NT_LG;    case K::Lb:   return NK::NT_LB;
        case K::Log:  return NK::NT_LOG;
        case K::Sin:  return NK::NT_SIN;   case K::Cos:  return NK::NT_COS;
        case K::Tan:  return NK::NT_TAN;   case K::Cot:  return NK::NT_COT;
        case K::Sec:  return NK::NT_SEC;   case K::Csc:  return NK::NT_CSC;
        case K::Asin: return NK::NT_ASIN;  case K::Acos: return NK::NT_ACOS;
        case K::Atan: return NK::NT_ATAN;  case K::Acot: return NK::NT_ACOT;
        case K::Asec: return NK::NT_ASEC;  case K::Acsc: return NK::NT_ACSC;
        case K::Sinh: return NK::NT_SINH;  case K::Cosh: return NK::NT_COSH;
        case K::Tanh: return NK::NT_TANH;  case K::Asech: return NK::NT_ASECH;
        case K::Acsch: return NK::NT_ACSCH; case K::Acoth: return NK::NT_ACOTH;
        case K::Atan2: return NK::NT_ATAN2; case K::Asinh: return NK::NT_ASINH;
        case K::Acosh: return NK::NT_ACOSH; case K::Atanh: return NK::NT_ATANH;
        case K::Sech: return NK::NT_SECH;  case K::Csch: return NK::NT_CSCH;
        case K::Coth: return NK::NT_COTH;
        // --- predicates / comparisons / conversions ---
        case K::IsDivisible: return NK::NT_IS_DIVISIBLE;
        case K::IsPrime:     return NK::NT_IS_PRIME;
        case K::IsEven:      return NK::NT_IS_EVEN;
        case K::IsOdd:       return NK::NT_IS_ODD;
        case K::IsInt:       return NK::NT_IS_INT;
        case K::Le: return NK::NT_LE;   case K::Lt: return NK::NT_LT;
        case K::Ge: return NK::NT_GE;   case K::Gt: return NK::NT_GT;
        case K::ToInt:  return NK::NT_TO_INT;
        case K::ToReal: return NK::NT_TO_REAL;
        // --- bit-vectors ---
        case K::BvNot: return NK::NT_BV_NOT;   case K::BvNeg: return NK::NT_BV_NEG;
        case K::BvAnd: return NK::NT_BV_AND;   case K::BvOr:  return NK::NT_BV_OR;
        case K::BvXor: return NK::NT_BV_XOR;   case K::BvNand: return NK::NT_BV_NAND;
        case K::BvNor: return NK::NT_BV_NOR;   case K::BvXnor: return NK::NT_BV_XNOR;
        case K::BvComp: return NK::NT_BV_COMP; case K::BvAdd: return NK::NT_BV_ADD;
        case K::BvSub: return NK::NT_BV_SUB;   case K::BvMul: return NK::NT_BV_MUL;
        case K::BvUdiv: return NK::NT_BV_UDIV; case K::BvSdiv: return NK::NT_BV_SDIV;
        case K::BvUrem: return NK::NT_BV_UREM; case K::BvSrem: return NK::NT_BV_SREM;
        case K::BvUmod: return NK::NT_BV_UMOD; case K::BvSmod: return NK::NT_BV_SMOD;
        case K::BvShl: return NK::NT_BV_SHL;   case K::BvLshr: return NK::NT_BV_LSHR;
        case K::BvAshr: return NK::NT_BV_ASHR;
        case K::BvUlt: return NK::NT_BV_ULT;   case K::BvUle: return NK::NT_BV_ULE;
        case K::BvUgt: return NK::NT_BV_UGT;   case K::BvUge: return NK::NT_BV_UGE;
        case K::BvSlt: return NK::NT_BV_SLT;   case K::BvSle: return NK::NT_BV_SLE;
        case K::BvSgt: return NK::NT_BV_SGT;   case K::BvSge: return NK::NT_BV_SGE;
        case K::BvConcat: return NK::NT_BV_CONCAT;
        case K::BvToNat: return NK::NT_BV_TO_NAT; case K::NatToBv: return NK::NT_NAT_TO_BV;
        case K::BvToInt: return NK::NT_BV_TO_INT; case K::IntToBv: return NK::NT_INT_TO_BV;
        case K::BvExtract: return NK::NT_BV_EXTRACT; case K::BvRepeat: return NK::NT_BV_REPEAT;
        case K::BvZeroExtend: return NK::NT_BV_ZERO_EXT; case K::BvSignExtend: return NK::NT_BV_SIGN_EXT;
        case K::BvRotateLeft: return NK::NT_BV_ROTATE_LEFT; case K::BvRotateRight: return NK::NT_BV_ROTATE_RIGHT;
        case K::BvNego: return NK::NT_BV_NEGO; case K::BvUaddo: return NK::NT_BV_UADDO;
        case K::BvSaddo: return NK::NT_BV_SADDO; case K::BvUmulo: return NK::NT_BV_UMULO;
        case K::BvSmulo: return NK::NT_BV_SMULO; case K::BvUdivo: return NK::NT_BV_UDIVO;
        case K::BvSdivo: return NK::NT_BV_SDIVO; case K::BvUremo: return NK::NT_BV_UREMO;
        case K::BvSremo: return NK::NT_BV_SREMO; case K::BvUmodo: return NK::NT_BV_UMODO;
        case K::BvSmodo: return NK::NT_BV_SMODO;
        // --- floating point ---
        case K::FpAdd: return NK::NT_FP_ADD; case K::FpSub: return NK::NT_FP_SUB;
        case K::FpMul: return NK::NT_FP_MUL; case K::FpDiv: return NK::NT_FP_DIV;
        case K::FpAbs: return NK::NT_FP_ABS; case K::FpNeg: return NK::NT_FP_NEG;
        case K::FpRem: return NK::NT_FP_REM; case K::FpFma: return NK::NT_FP_FMA;
        case K::FpSqrt: return NK::NT_FP_SQRT; case K::FpRoundToIntegral: return NK::NT_FP_ROUND_TO_INTEGRAL;
        case K::FpMin: return NK::NT_FP_MIN; case K::FpMax: return NK::NT_FP_MAX;
        case K::FpLe: return NK::NT_FP_LE; case K::FpLt: return NK::NT_FP_LT;
        case K::FpGe: return NK::NT_FP_GE; case K::FpGt: return NK::NT_FP_GT;
        case K::FpEq: return NK::NT_FP_EQ;
        case K::FpToUbv: return NK::NT_FP_TO_UBV; case K::FpToSbv: return NK::NT_FP_TO_SBV;
        case K::FpToReal: return NK::NT_FP_TO_REAL; case K::FpToFp: return NK::NT_FP_TO_FP;
        case K::FpToFpUnsigned: return NK::NT_FP_TO_FP_UNSIGNED; case K::FpToIeeeBv: return NK::NT_FP_TO_IEEE_BV;
        case K::FpIsNormal: return NK::NT_FP_IS_NORMAL; case K::FpIsSubnormal: return NK::NT_FP_IS_SUBNORMAL;
        case K::FpIsZero: return NK::NT_FP_IS_ZERO; case K::FpIsInf: return NK::NT_FP_IS_INF;
        case K::FpIsNan: return NK::NT_FP_IS_NAN; case K::FpIsNeg: return NK::NT_FP_IS_NEG;
        case K::FpIsPos: return NK::NT_FP_IS_POS;
        // --- arrays ---
        case K::Select: return NK::NT_SELECT; case K::Store: return NK::NT_STORE;
        // --- strings ---
        case K::StrLen: return NK::NT_STR_LEN; case K::StrConcat: return NK::NT_STR_CONCAT;
        case K::StrSubstr: return NK::NT_STR_SUBSTR; case K::StrIndexof: return NK::NT_STR_INDEXOF;
        case K::StrCharat: return NK::NT_STR_CHARAT; case K::StrUpdate: return NK::NT_STR_UPDATE;
        case K::StrReplace: return NK::NT_STR_REPLACE; case K::StrReplaceAll: return NK::NT_STR_REPLACE_ALL;
        case K::StrReplaceRe: return NK::NT_STR_REPLACE_REG; case K::StrReplaceReAll: return NK::NT_STR_REPLACE_REG_ALL;
        case K::StrIndexofRe: return NK::NT_STR_INDEXOF_REG; case K::StrToLower: return NK::NT_STR_TO_LOWER;
        case K::StrToUpper: return NK::NT_STR_TO_UPPER; case K::StrRev: return NK::NT_STR_REV;
        case K::StrSplit: return NK::NT_STR_SPLIT; case K::StrSplitAt: return NK::NT_STR_SPLIT_AT;
        case K::StrSplitRest: return NK::NT_STR_SPLIT_REST; case K::StrNumSplits: return NK::NT_STR_NUM_SPLITS;
        case K::StrSplitAtRe: return NK::NT_STR_SPLIT_AT_RE; case K::StrSplitRestRe: return NK::NT_STR_SPLIT_REST_RE;
        case K::StrNumSplitsRe: return NK::NT_STR_NUM_SPLITS_RE; case K::StrLt: return NK::NT_STR_LT;
        case K::StrLe: return NK::NT_STR_LE; case K::StrGt: return NK::NT_STR_GT;
        case K::StrGe: return NK::NT_STR_GE; case K::StrInRe: return NK::NT_STR_IN_REG;
        case K::StrContains: return NK::NT_STR_CONTAINS; case K::StrIsDigit: return NK::NT_STR_IS_DIGIT;
        case K::StrPrefixof: return NK::NT_STR_PREFIXOF; case K::StrSuffixof: return NK::NT_STR_SUFFIXOF;
        case K::StrFromInt: return NK::NT_STR_FROM_INT; case K::StrToInt: return NK::NT_STR_TO_INT;
        case K::StrToRe: return NK::NT_STR_TO_REG; case K::StrToCode: return NK::NT_STR_TO_CODE;
        case K::StrFromCode: return NK::NT_STR_FROM_CODE;
        // --- regex ---
        case K::ReConcat: return NK::NT_REG_CONCAT; case K::ReUnion: return NK::NT_REG_UNION;
        case K::ReInter: return NK::NT_REG_INTER; case K::ReDiff: return NK::NT_REG_DIFF;
        case K::ReStar: return NK::NT_REG_STAR; case K::RePlus: return NK::NT_REG_PLUS;
        case K::ReOpt: return NK::NT_REG_OPT; case K::ReRange: return NK::NT_REG_RANGE;
        case K::ReRepeat: return NK::NT_REG_REPEAT; case K::ReComplement: return NK::NT_REG_COMPLEMENT;
        case K::ReLoop: return NK::NT_REG_LOOP; case K::ReNone: return NK::NT_REG_NONE;
        case K::ReAll: return NK::NT_REG_ALL; case K::ReAllchar: return NK::NT_REG_ALLCHAR;
        // --- functions / UF ---
        case K::Apply: return NK::NT_UF_APPLY;  // collapse family: uniform UF apply (see header)
        // K::FuncDecl is in mapKind's forward image (NT_FUNC_DEC) but is a pure symbol node, NOT a
        // term a DAGNode carries through getKind(); treat it as outside the image -> NT_UNKNOWN.
        case K::FuncDecl: return NK::NT_UNKNOWN;
        // --- datatypes ---
        case K::DtConstructor: return NK::NT_DT_CONSTRUCTOR; case K::DtSelector: return NK::NT_DT_SELECTOR;
        case K::DtTester: return NK::NT_DT_TESTER; case K::DtMatch: return NK::NT_DT_MATCH;
        // --- transcendental / special constants ---
        case K::Pi: return NK::NT_CONST_PI;
        case K::Infinity: return NK::NT_INFINITY; case K::Nan: return NK::NT_NAN;
        case K::Epsilon: return NK::NT_EPSILON;
        case K::PosInfinity: return NK::NT_POS_INFINITY; case K::NegInfinity: return NK::NT_NEG_INFINITY;
        case K::PosEpsilon: return NK::NT_POS_EPSILON; case K::NegEpsilon: return NK::NT_NEG_EPSILON;
        // --- real-algebraic (Xolver NRA witnesses) ---
        case K::RootObj: return NK::NT_ROOT_OBJ;
        case K::RootOfWithInterval: return NK::NT_ROOT_OF_WITH_INTERVAL;
        case K::RealAlgebraicNumber: return NK::NT_REAL_ALGEBRAIC_NUMBER;
        // --- quantifiers ---
        case K::Forall: return NK::NT_FORALL;
        case K::Exists: return NK::NT_EXISTS;
        // Arena-internal / non-DAGNode kinds (E, BoundVar, Sort*, ConstArray already handled above,
        // etc.) a getKind() call never legitimately reaches: defensive NT_UNKNOWN.
        default: return NK::NT_UNKNOWN;
    }
}

}  // namespace xarena_cov
