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

namespace SOMTParser{

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
    };

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
}

#endif
