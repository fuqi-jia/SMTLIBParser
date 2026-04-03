/* -*- Header -*-
 *
 * The Util Functions
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
#ifndef UTIL_HEADER
#define UTIL_HEADER

#include "somtparser/core/kind.h"
#include "somtparser/ir/number.h"
#include "somtparser/core/asserting.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <cstdint>
#include <optional>
#include <cfenv>
#include <cmath>
#include <mpfr.h>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <limits>

namespace SOMTParser{

    // Forward declarations for DAG-dependent utilities
    class DAGNode;
    class Sort;

    // Type checking utilities
    class TypeChecker {
    public:
        static bool isInt(const std::string& str);
        static bool isReal(const std::string& str);
        static bool isBV(const std::string& str);
        static bool isFP(const std::string& str);
        static bool isString(const std::string& str);
        static bool isScientificNotation(const std::string& str);
        static bool isNumber(const std::string& str);
    };

    // Mathematical utilities
    class MathUtils {
    public:
        static Integer pow(const Integer& base, const Integer& exp);
        static Real pow(const Real& base, const Real& exp);
        static Integer gcd(const Integer& a, const Integer& b);
        static Integer lcm(const Integer& a, const Integer& b);
        static Real sqrt(const Integer& i);
        static Real sqrt(const Real& r);
        static Real safeSqrt(const Integer& i);
        static Real safeSqrt(const Real& r);
        static Integer ceil(const Real& r);
        static Integer floor(const Real& r);
        static Integer round(const Real& r);
        static bool isPrime(const Integer& n);
        static bool isEven(const Integer& n);
        static bool isOdd(const Integer& n);
        static Integer factorial(const Integer& n);
    };

    // Bit vector utilities
    class BitVectorUtils {
    public:
        static Integer bvToNat(const std::string& bv);
        static std::string natToBv(const Integer& i, const Integer& n);
        static std::string natToBv(const std::string& i, const Integer& n);
        static Integer bvToInt(const std::string& bv);
        static std::string intToBv(const Integer& i, const Integer& n);

        static std::string bvNot(const std::string& bv);
        static std::string bvAnd(const std::string& bv1, const std::string& bv2);
        static std::string bvOr(const std::string& bv1, const std::string& bv2);
        static std::string bvXor(const std::string& bv1, const std::string& bv2);
        static std::string bvNand(const std::string& bv1, const std::string& bv2);
        static std::string bvNor(const std::string& bv1, const std::string& bv2);
        static std::string bvXnor(const std::string& bv1, const std::string& bv2);

        static std::string bvNeg(const std::string& bv);
        static std::string bvAdd(const std::string& bv1, const std::string& bv2);
        static std::string bvSub(const std::string& bv1, const std::string& bv2);
        static std::string bvMul(const std::string& bv1, const std::string& bv2);

        static std::string bvUdiv(const std::string& bv1, const std::string& bv2);
        static std::string bvUrem(const std::string& bv1, const std::string& bv2);
        static std::string bvUmod(const std::string& bv1, const std::string& bv2);
        static std::string bvSdiv(const std::string& bv1, const std::string& bv2);
        static std::string bvSrem(const std::string& bv1, const std::string& bv2);
        static std::string bvSmod(const std::string& bv1, const std::string& bv2);

        static std::string bvShl(const std::string& bv, const std::string& n);
        static std::string bvLshr(const std::string& bv, const std::string& n);
        static std::string bvAshr(const std::string& bv, const std::string& n);

        static std::string bvConcat(const std::string& bv1, const std::string& bv2);
        static std::string bvExtract(const std::string& bv, const Integer& i, const Integer& j);
        static std::string bvRepeat(const std::string& bv, const Integer& n);
        static std::string bvZeroExtend(const std::string& bv, const Integer& n);
        static std::string bvSignExtend(const std::string& bv, const Integer& n);

        static std::string bvRotateLeft(const std::string& bv, const Integer& n);
        static std::string bvRotateRight(const std::string& bv, const Integer& n);

        static bool bvComp(const std::string& bv1, const std::string& bv2, const NODE_KIND& kind);
    };

    // Floating point utilities
    class FloatingPointUtils {
    public:
        static std::string fpToUbv(const std::string& fp, const Integer& n);
        static std::string fpToSbv(const std::string& fp, const Integer& n);

        // ─── Generic FP bit-level representation ──────────────────────────
        // Represents an arbitrary-width IEEE-754 FP value as (sign, exponent, significand).
        // eb = exponent bit width, sb = significand bit width (incl. hidden bit per SMT-LIB).
        struct FPValue {
            uint64_t sign;        // 0 or 1
            uint64_t exponent;    // exponent bits as unsigned integer
            uint64_t significand; // significand bits (without hidden bit)
            size_t eb;            // exponent bit width
            size_t sb;            // significand bit width (including hidden bit per SMT-LIB)

            bool isNaN() const;
            bool isInf() const;
            bool isZero() const;
            bool isNeg() const;
            bool isNormal() const;
            bool isSubnormal() const;

            std::string toSMTFP() const;               // → "(fp #b... #b... #b...)"
            std::optional<float> toFloat32() const;     // only if (eb,sb)==(8,24)
            std::optional<double> toFloat64() const;    // only if (eb,sb)==(11,53)

            static FPValue fromFloat32(float f);
            static FPValue fromFloat64(double d);

            // ─── MPFR conversion ─────────────────────────────────────
            // Load this FPValue into an mpfr_t (caller must mpfr_init2 first).
            // Returns false only for NaN (mpfr has no NaN arithmetic).
            bool toMpfr(mpfr_t out) const;
            // Extract FPValue from mpfr_t result, rounding to target (eb, sb).
            static FPValue fromMpfr(const mpfr_t val, size_t eb, size_t sb);

            // ─── Bit-level sign operations ───────────────────────────
            FPValue abs() const;  // copy with sign = 0
            FPValue neg() const;  // copy with sign flipped
        };

        // ─── Rounding mode (fesetround-based, legacy) ─────────────────
        static int smtRMToFeround(const std::string& rm_name);
        static int getFPRoundingMode(const std::shared_ptr<DAGNode>& rm_node);

        // ─── Rounding mode (MPFR-based, supports all 5 IEEE modes) ───
        static mpfr_rnd_t smtRMToMpfrRound(const std::string& rm_name);
        static mpfr_rnd_t getFPRoundingModeMpfr(const std::shared_ptr<DAGNode>& rm_node);

        // ─── Bit-level helpers ───────────────────────────────────────────
        static uint64_t parseBVBits(const std::string& bv_name);
        static std::string uint64ToBinStr(uint64_t val, size_t width);

        // ─── Float reconstruction ────────────────────────────────────────
        static float reconstructFloat32(uint32_t sign_bit, uint32_t exp_bits, uint32_t mant_bits);
        static double reconstructFloat64(uint64_t sign_bit, uint64_t exp_bits, uint64_t mant_bits);

        // ─── Float → SMT-LIB string ─────────────────────────────────────
        static std::string float32ToSMTFP(float f);
        static std::string float64ToSMTFP(double d);
        static std::string fpValueToSMTFP(const FPValue& v);

        // ─── DAGNode → native type extraction ───────────────────────────
        // Unified extraction for any FP width, returns bit-level FPValue
        static std::optional<FPValue> fpNodeToValue(const std::shared_ptr<DAGNode>& node);
        // Fast-path for common widths
        static std::optional<float> fpNodeToFloat32(const std::shared_ptr<DAGNode>& node);
        static std::optional<double> fpNodeToFloat64(const std::shared_ptr<DAGNode>& node);

        // ─── Special value checks (use node kinds, no string matching) ──
        static bool fpNodeIsNaN(const std::shared_ptr<DAGNode>& node);
        static bool fpNodeIsInf(const std::shared_ptr<DAGNode>& node);
        static bool fpNodeIsZero(const std::shared_ptr<DAGNode>& node);
        static bool fpNodeIsNeg(const std::shared_ptr<DAGNode>& node);
        static bool fpNodeIsNormal(const std::shared_ptr<DAGNode>& node);
        static bool fpNodeIsSubnormal(const std::shared_ptr<DAGNode>& node);

        // ─── Generic MPFR-based FP operations (any eb, sb) ──────────
        using MpfrUnaryFn  = int(*)(mpfr_t, const mpfr_t, mpfr_rnd_t);
        using MpfrBinaryFn = int(*)(mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t);
        using MpfrTernaryFn = int(*)(mpfr_t, const mpfr_t, const mpfr_t, const mpfr_t, mpfr_rnd_t);

        static std::optional<FPValue> fpUnaryOp(const FPValue& a, size_t eb, size_t sb,
                                                 mpfr_rnd_t rnd, MpfrUnaryFn op);
        static std::optional<FPValue> fpBinaryOp(const FPValue& a, const FPValue& b,
                                                  size_t eb, size_t sb,
                                                  mpfr_rnd_t rnd, MpfrBinaryFn op);
        static std::optional<FPValue> fpTernaryOp(const FPValue& a, const FPValue& b,
                                                   const FPValue& c, size_t eb, size_t sb,
                                                   mpfr_rnd_t rnd, MpfrTernaryFn op);

        // IEEE comparison: returns -1 (lt), 0 (eq), 1 (gt), 2 (unordered/NaN)
        static int fpCompare(const FPValue& a, const FPValue& b);

        // IEEE remainder (always rounds-to-nearest, no RM parameter)
        static std::optional<FPValue> fpRemainder(const FPValue& a, const FPValue& b,
                                                   size_t eb, size_t sb);

        // Round FP value to integral, keeping FP type
        static std::optional<FPValue> fpRoundToIntegral(const FPValue& a,
                                                         size_t eb, size_t sb, mpfr_rnd_t rnd);

        // IEEE min/max with NaN semantics (NaN → return the other operand)
        static std::optional<FPValue> fpMin(const FPValue& a, const FPValue& b);
        static std::optional<FPValue> fpMax(const FPValue& a, const FPValue& b);

        // Convert FPValue to double (for fp.to_real). Returns nullopt for NaN/Inf.
        static std::optional<double> fpToDouble(const FPValue& v);
    };

    // ─── Free function aliases (backward-compatible with fp_utils.h API) ────────
    inline int getFPRoundingMode(const std::shared_ptr<DAGNode>& rm) { return FloatingPointUtils::getFPRoundingMode(rm); }
    inline std::optional<float> fpNodeToFloat32(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeToFloat32(n); }
    inline std::optional<double> fpNodeToFloat64(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeToFloat64(n); }
    inline std::string float32ToSMTFP(float f) { return FloatingPointUtils::float32ToSMTFP(f); }
    inline std::string float64ToSMTFP(double d) { return FloatingPointUtils::float64ToSMTFP(d); }
    inline bool fpNodeIsNaN(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsNaN(n); }
    inline bool fpNodeIsInf(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsInf(n); }
    inline bool fpNodeIsZero(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsZero(n); }
    inline bool fpNodeIsNeg(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsNeg(n); }
    inline bool fpNodeIsNormal(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsNormal(n); }
    inline bool fpNodeIsSubnormal(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeIsSubnormal(n); }
    inline std::optional<FloatingPointUtils::FPValue> fpNodeToValue(const std::shared_ptr<DAGNode>& n) { return FloatingPointUtils::fpNodeToValue(n); }
    inline mpfr_rnd_t getFPRoundingModeMpfr(const std::shared_ptr<DAGNode>& rm) { return FloatingPointUtils::getFPRoundingModeMpfr(rm); }

    // String utilities
    class StringUtils {
    public:
        static std::string strSubstr(const std::string& s, const Integer& i, const Integer& j);
        static bool strPrefixof(const std::string& s, const std::string& t);
        static bool strSuffixof(const std::string& s, const std::string& t);
        static bool strContains(const std::string& s, const std::string& t);
        static Integer strIndexof(const std::string& s, const std::string& t, const Integer& i);
        static std::string strCharAt(const std::string& s, const Integer& i);
        static std::string strUpdate(const std::string& s, const Integer& i, const std::string& t);
        static std::string strReplace(const std::string& s, const std::string& t, const std::string& u);
        static std::string strReplaceAll(const std::string& s, const std::string& t, const std::string& u);
        static std::string strToLower(const std::string& s);
        static std::string strToUpper(const std::string& s);
        static std::string strRev(const std::string& s);
    };

    // Conversion utilities
    class ConversionUtils {
    public:
        static std::string toString(const Integer& i);
        static std::string toString(const Real& r);
        static std::string toString(const int& i);
        static std::string toString(const double& d);
        static std::string toString(const float& f);
        static std::string toString(const long& l);
        static std::string toString(const short& s);
        static std::string toString(const char& c);
        static std::string toString(const bool& b);
        static std::string parseScientificNotation(const std::string& str);
        static std::string escapeString(const std::string& s);
        static std::string unescapeString(const std::string& s); 
    };

    // ─── UF (Uninterpreted Function) utilities ──────────────────────────────

    /// Remove whitespace characters (space, tab, newline, CR) from a string.
    /// Used to normalise UF argument keys for map lookups.
    inline std::string sanitizeKey(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for(char c : s)
            if(c != ' ' && c != '\n' && c != '\r' && c != '\t')
                result += c;
        return result;
    }

    /// Represents a single application of an uninterpreted function.
    struct UFApplication {
        std::string func_name;
        std::vector<std::shared_ptr<DAGNode>> args;
        std::shared_ptr<DAGNode> application_node;
        std::shared_ptr<Sort> result_sort;
    };

    /// Collect all UF application nodes from a list of assertions.
    std::unordered_map<std::string, std::vector<UFApplication>>
    collectUFApplications(const std::vector<std::shared_ptr<DAGNode>>& assertions);

    /// A single (args → result) entry in a UF function table.
    struct UFTableEntry {
        std::vector<std::string> arg_values;
        std::string result_value;
    };

    /// Format a UF function table as a SMT-LIB2 `(define-fun ...)` string.
    std::string formatUFDefine(
        const std::string& func_name,
        const std::vector<std::string>& param_sorts,
        const std::string& result_sort,
        const std::vector<UFTableEntry>& entries,
        const std::string& default_value);
}

#endif
