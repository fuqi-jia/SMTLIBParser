/* -*- Source -*-
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

#include "somtparser/core/util.h"
#include "somtparser/ir/dag.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <mpfr.h>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace SOMTParser{

    static bool isSpaceChar(unsigned char c) { return std::isspace(c) != 0; }

    // ── RegexUtils internal helpers ──────────────────────────────────────────────
    //
    // toEcmaPattern  : translates a ground SMT-LIB regex DAGNode to an ECMAScript
    //                   pattern string understood by std::regex_match.
    // strInReHelper   : handles structural operators (re.inter, re.diff, re.comp,
    //                   re.none / re.all) and calls toEcmaPattern + std::regex_match
    //                   for the remaining cases.
    //
    // Only "leaves" of the structural decomposition ever call std::regex — the
    // library itself does all real matching.  No custom NFA is implemented.

    static std::string regexEscapeForPattern(const std::string& s) {
        static const std::string kSpecials = ".^$*+?()[]{}\\|";
        std::string out;
        out.reserve(s.size() * 2);
        for (unsigned char c : s) {
            if (kSpecials.find(static_cast<char>(c)) != std::string::npos)
                out += '\\';
            out += static_cast<char>(c);
        }
        return out;
    }

    static std::string regexEscapeForClass(char c) {
        if (c == ']' || c == '\\' || c == '^' || c == '-')
            return std::string("\\") + c;
        return std::string(1, c);
    }

    // Forward declaration.
    static std::optional<bool> strInReHelper(
        const std::string& raw,
        const std::shared_ptr<DAGNode>& regex);

    static std::optional<std::string> toEcmaPattern(
        const std::shared_ptr<DAGNode>& regex)
    {
        using NK = NODE_KIND;
        if (!regex) return std::nullopt;

        switch (regex->getKind()) {

        // re.none / re.all / re.allchar are stored as NT_CONST nodes by mkConstReg.
        case NK::NT_CONST: {
            const std::string& nm = regex->getName();
            if (nm == "re.all")     return std::string("[\\s\\S]*");
            if (nm == "re.allchar") return std::string("[\\s\\S]");
            return std::nullopt;
        }

        case NK::NT_REG_ALL:
            return std::string("[\\s\\S]*");

        case NK::NT_REG_ALLCHAR:
            return std::string("[\\s\\S]");

        case NK::NT_STR_TO_REG: {
            auto child = regex->getChild(0);
            if (!child->isCStr()) return std::nullopt;
            return regexEscapeForPattern(child->getStringLiteral());
        }

        case NK::NT_REG_RANGE: {
            auto lo_n = regex->getChild(0);
            auto hi_n = regex->getChild(1);
            if (!lo_n->isCStr() || !hi_n->isCStr()) return std::nullopt;
            std::string lo = lo_n->getStringLiteral();
            std::string hi = hi_n->getStringLiteral();
            if (lo.empty() || hi.empty()) return std::nullopt;
            if (lo[0] == hi[0])
                return "[" + regexEscapeForClass(lo[0]) + "]";
            return "[" + regexEscapeForClass(lo[0]) + "-" + regexEscapeForClass(hi[0]) + "]";
        }

        case NK::NT_REG_STAR: {
            auto sub = toEcmaPattern(regex->getChild(0));
            if (!sub) return std::nullopt;
            return "(?:" + *sub + ")*";
        }

        case NK::NT_REG_PLUS: {
            auto sub = toEcmaPattern(regex->getChild(0));
            if (!sub) return std::nullopt;
            return "(?:" + *sub + ")+";
        }

        case NK::NT_REG_OPT: {
            auto sub = toEcmaPattern(regex->getChild(0));
            if (!sub) return std::nullopt;
            return "(?:" + *sub + ")?";
        }

        case NK::NT_REG_CONCAT: {
            std::string result;
            for (size_t i = 0; i < regex->getChildrenSize(); ++i) {
                auto sub = toEcmaPattern(regex->getChild(i));
                if (!sub) return std::nullopt;
                result += "(?:" + *sub + ")";
            }
            return result;
        }

        case NK::NT_REG_UNION: {
            if (regex->getChildrenSize() == 0) return std::nullopt;
            std::string result = "(?:";
            for (size_t i = 0; i < regex->getChildrenSize(); ++i) {
                auto sub = toEcmaPattern(regex->getChild(i));
                if (!sub) return std::nullopt;
                if (i > 0) result += '|';
                result += *sub;
            }
            return result + ")";
        }

        case NK::NT_REG_REPEAT: {
            if (regex->getChildrenSize() < 2) return std::nullopt;
            auto sub = toEcmaPattern(regex->getChild(0));
            if (!sub) return std::nullopt;
            long n = 0;
            try { n = std::stol(regex->getChild(1)->getName()); }
            catch (...) { return std::nullopt; }
            if (n < 0) return std::nullopt;
            return "(?:" + *sub + "){" + std::to_string(n) + "}";
        }

        case NK::NT_REG_LOOP: {
            if (regex->getChildrenSize() < 3) return std::nullopt;
            auto sub = toEcmaPattern(regex->getChild(0));
            if (!sub) return std::nullopt;
            long lo = 0, hi = 0;
            try {
                lo = std::stol(regex->getChild(1)->getName());
                hi = std::stol(regex->getChild(2)->getName());
            } catch (...) { return std::nullopt; }
            if (lo < 0 || hi < lo) return std::nullopt;
            return "(?:" + *sub + "){" + std::to_string(lo) + "," + std::to_string(hi) + "}";
        }

        default:
            return std::nullopt;
        }
    }

    static std::optional<bool> strInReHelper(
        const std::string& raw,
        const std::shared_ptr<DAGNode>& regex)
    {
        using NK = NODE_KIND;
        if (!regex) return std::nullopt;

        switch (regex->getKind()) {

        case NK::NT_REG_NONE:
            return false;

        case NK::NT_REG_ALL:
            return true;

        case NK::NT_REG_ALLCHAR:
            return (raw.size() == 1);

        // re.none / re.all / re.allchar are stored as NT_CONST nodes by mkConstReg.
        case NK::NT_CONST: {
            const std::string& nm = regex->getName();
            if (nm == "re.none")    return false;
            if (nm == "re.all")     return true;
            if (nm == "re.allchar") return (raw.size() == 1);
            return std::nullopt;
        }

        case NK::NT_REG_INTER: {
            for (size_t i = 0; i < regex->getChildrenSize(); ++i) {
                auto r = strInReHelper(raw, regex->getChild(i));
                if (!r)  return std::nullopt;
                if (!*r) return false;
            }
            return true;
        }

        case NK::NT_REG_DIFF: {
            if (regex->getChildrenSize() < 2) return std::nullopt;
            auto r1 = strInReHelper(raw, regex->getChild(0));
            if (!r1 || !*r1) return r1;
            auto r2 = strInReHelper(raw, regex->getChild(1));
            if (!r2) return std::nullopt;
            return !*r2;
        }

        case NK::NT_REG_COMPLEMENT: {
            auto r = strInReHelper(raw, regex->getChild(0));
            if (!r) return std::nullopt;
            return !*r;
        }

        default: {
            auto pat = toEcmaPattern(regex);
            if (!pat) return std::nullopt;
            try {
                std::regex re(*pat, std::regex_constants::ECMAScript);
                return std::regex_match(raw, re);
            } catch (const std::regex_error&) {
                return std::nullopt;
            }
        }
        }
    }

    bool TypeChecker::isNumber(const std::string& str){
        return isInt(str) || isReal(str);
    }

    bool TypeChecker::isInt(const std::string& str){
        if (str.empty()) return false;
        for (size_t i = 0; i < str.size(); i++){
            if (i == 0 && (str[i] == '-' || str[i] == '+')) continue;
            if (!isdigit(str[i])) return false;
        }
        return true;

    }
    bool TypeChecker::isReal(const std::string& str){
        if (str.empty()) return false;
        bool has_dot = false;
        bool has_slash = false;
        for (size_t i = 0; i < str.size(); i++){
            if (i == 0 && (str[i] == '-' || str[i] == '+')) continue;
            if (str[i] == '.' && !has_dot && !has_slash){
                has_dot = true;
                continue;
            }
            if (str[i] == '/' && !has_dot && !has_slash){
                has_slash = true;
                continue;
            }
            if (!isdigit(str[i])) return false;
        }
        if (has_slash) {
            size_t sl = str.find('/');
            if (sl == std::string::npos || sl == 0 || sl == str.size() - 1) return false;
            std::string num = str.substr(0, sl);
            std::string den = str.substr(sl + 1);
            if (!isInt(num) || !isInt(den)) return false;
            // reject denominator zero
            try {
                mpz_class den_z(den);
                if (den_z == 0) return false;
            } catch (...) {
                return false;
            }
        }
        return true;
    }

    bool TypeChecker::isScientificNotation(const std::string& str){
        if (str.empty()) return false;
        
        // find 'E' or 'e' character
        size_t e_pos = str.find_first_of("Ee");
        if (e_pos == std::string::npos || e_pos == 0) 
            return false;
            
        // check if the part before E is a valid real number
        std::string mantissa = str.substr(0, e_pos);
        if (!TypeChecker::isReal(mantissa)) 
            return false;
        
        // extract the part after E
        std::string exponent = str.substr(e_pos + 1);
        
        // if the exponent part is empty, not a valid scientific notation
        if (exponent.empty())
            return false;
        
        // create a copy without spaces for checking
        std::string exponent_no_spaces = exponent;
        exponent_no_spaces.erase(std::remove_if(exponent_no_spaces.begin(), exponent_no_spaces.end(), 
                                     isSpaceChar), 
                      exponent_no_spaces.end());
        
        // if the exponent part is empty after removing spaces, not a valid scientific notation
        if (exponent_no_spaces.empty())
            return false;
        
        // handle possible parentheses
        if (exponent_no_spaces[0] == '(') {
            // find right parenthesis
            size_t close_pos = exponent_no_spaces.find(')');
            if (close_pos != std::string::npos) {
                // extract the content inside parentheses
                exponent_no_spaces = exponent_no_spaces.substr(1, close_pos - 1);
            } else {
                // no right parenthesis found, possibly incomplete expression
                exponent_no_spaces = exponent_no_spaces.substr(1);
            }
        }
        
        // if the exponent part is empty after handling parentheses, not a valid scientific notation
        if (exponent_no_spaces.empty())
            return false;
        
        // check if the exponent part is a valid integer
        if (exponent_no_spaces[0] == '+' || exponent_no_spaces[0] == '-') {
            // if "E-" or "E+", there must be a digit after it
            if (exponent_no_spaces.size() == 1) 
                return false;
            // check if the part after "E-" or "E+" is all digits
            for (size_t i = 1; i < exponent_no_spaces.size(); i++) {
                if (!isdigit(exponent_no_spaces[i])) 
                    return false;
            }
        } else {
            // if there is no sign, the whole exponent part must be all digits
            for (char c : exponent_no_spaces) {
                if (!isdigit(c)) 
                    return false;
            }
        }
        
        return true;
    }

    std::string ConversionUtils::parseScientificNotation(const std::string& str){
        size_t e_pos = str.find_first_of("Ee");
        if (e_pos == std::string::npos)
            return str;

        try {
            std::string mantissa = str.substr(0, e_pos);
            if (!TypeChecker::isReal(mantissa))
                return str;

            std::string exponent = str.substr(e_pos + 1);
            if (exponent.empty())
                return str;

            std::string exponent_no_spaces = exponent;
            exponent_no_spaces.erase(std::remove_if(exponent_no_spaces.begin(), exponent_no_spaces.end(),
                                         isSpaceChar),
                          exponent_no_spaces.end());

            if (exponent_no_spaces.empty())
                return str;

            if (exponent_no_spaces[0] == '(') {
                size_t close_pos = exponent_no_spaces.find(')');
                if (close_pos != std::string::npos) {
                    exponent_no_spaces = exponent_no_spaces.substr(1, close_pos - 1);
                } else {
                    exponent_no_spaces = exponent_no_spaces.substr(1);
                }
            }

            if (exponent_no_spaces.empty())
                return str;

            int exp_val = std::stoi(exponent_no_spaces);

            // Parse mantissa into sign + integer part + fractional part
            bool neg = false;
            std::string m = mantissa;
            if (!m.empty() && m[0] == '-') { neg = true; m = m.substr(1); }
            else if (!m.empty() && m[0] == '+') { m = m.substr(1); }

            size_t dot_pos = m.find('.');
            std::string intPart = (dot_pos == std::string::npos) ? m : m.substr(0, dot_pos);
            std::string fracPart = (dot_pos == std::string::npos) ? "" : m.substr(dot_pos + 1);

            // Remove leading zeros from intPart (keep at least "0")
            size_t first_nonzero = intPart.find_first_not_of('0');
            if (first_nonzero == std::string::npos) intPart = "0";
            else if (first_nonzero > 0) intPart = intPart.substr(first_nonzero);

            std::string digits = intPart + fracPart;
            int frac_len = static_cast<int>(fracPart.length());
            int effective_exp = exp_val - frac_len;

            std::string result;
            if (effective_exp >= 0) {
                result = digits + std::string(effective_exp, '0');
            } else {
                int shift = -effective_exp;
                if (shift < static_cast<int>(digits.length())) {
                    result = digits.substr(0, digits.length() - shift) + "." +
                             digits.substr(digits.length() - shift);
                } else {
                    int leading_zeros = shift - static_cast<int>(digits.length());
                    result = "0." + std::string(leading_zeros, '0') + digits;
                }
            }
            if (neg && result != "0") result = "-" + result;
            return result;
        } catch (const std::exception& e) {
            return str;
        }
    }

    bool TypeChecker::isBV(const std::string& str){
        if (str.empty()) return false;
        if (str.size() < 3) return false;
        if (str[0] != '#') return false;
        if (str[1] != 'b' && str[1] != 'x' && str[1] != 'd' &&
            str[1] != 'B' && str[1] != 'X' && str[1] != 'D') return false;
        for (size_t i = 2; i < str.size(); i++){
            if ((str[1] == 'b' || str[1] == 'B') && 
                (str[i] != '0' && str[i] != '1')) return false;
            if ((str[1] == 'x' || str[1] == 'X') &&
                (str[i] != '0' && 
                str[i] != '1' && 
                str[i] != '2' && 
                str[i] != '3' && 
                str[i] != '4' && 
                str[i] != '5' && 
                str[i] != '6' && 
                str[i] != '7' && 
                str[i] != '8' && 
                str[i] != '9' && 
                str[i] != 'a' && 
                str[i] != 'A' && 
                str[i] != 'b' && 
                str[i] != 'B' && 
                str[i] != 'c' && 
                str[i] != 'C' && 
                str[i] != 'd' && 
                str[i] != 'D' &&
                str[i] != 'e' &&
                str[i] != 'E' &&
                str[i] != 'f' &&
                str[i] != 'F')) return false;
            if ((str[1] == 'd' || str[1] == 'D') && 
                (str[i] != '0' && 
                str[i] != '1' && 
                str[i] != '2' && 
                str[i] != '3' && 
                str[i] != '4' && 
                str[i] != '5' && 
                str[i] != '6' && 
                str[i] != '7' && 
                str[i] != '8' && 
                str[i] != '9')) return false;
        }
        return true;
    }
    bool TypeChecker::isFP(const std::string& str){
        if (str.empty()) return false;
        if (str.size() < 4) return false;
        if (str.substr(0, 3) != "(fp") return false;
        if (str[str.size()-1] != ')') return false;
        return true;
    }
    bool TypeChecker::isString(const std::string& str){
        if (str.empty()) return false;
        if (str[0] != '"' || str[str.size()-1] != '"') return false;
        return true;
    }


    Integer MathUtils::pow(const Integer& base, const Integer& exp){
        if(exp == 0) return 1;
        Integer result = base;
        for(Integer i = 1; i < exp; i++){
            result *= base;
        }
        return result;
    }
    Real MathUtils::pow(const Real& base, const Real& exp){
        return base.pow(exp);
    }

    Integer MathUtils::gcd(const Integer& a, const Integer& b){
        if(b == 0) return a;
        return MathUtils::gcd(b, a % b);
    }

    Integer MathUtils::lcm(const Integer& a, const Integer& b){
        return a * b / SOMTParser::MathUtils::gcd(a, b);
    }


    Real MathUtils::sqrt(const Integer& i){
        if(i < 0){
            std::cerr << "Error: MathUtils::sqrt of negative number" << std::endl;
            exit(1);
        }
        return HighPrecisionReal(i).sqrt();
    }
    Real MathUtils::sqrt(const Real& r){
        if(r < 0){
            std::cerr << "Error: MathUtils::sqrt of negative number" << std::endl;
            exit(1);
        }
        return r.sqrt();
    }

    Real MathUtils::safeSqrt(const Integer& i){
        if(i < 0){
            return Real(0);
        }
        return HighPrecisionReal(i).sqrt();
    }
    
    Real MathUtils::safeSqrt(const Real& r){
        if(r < 0){
            return Real(0);
        }
        return r.sqrt();
    }

    Integer MathUtils::ceil(const Real& r){
        return r.ceil().toInteger();
    }
    Integer MathUtils::floor(const Real& r){
        return r.floor().toInteger();
    }
    Integer MathUtils::round(const Real& r){
        return r.round().toInteger();
    }

    bool MathUtils::isPrime(const Integer& n){
        if(n <= 1) return false;
        if(n == 2) return true;
        if(n % 2 == 0) return false;
        for(Integer i = 3; i * i <= n; i += 2){
            if(n % i == 0) return false;
        }
        return true;
    }

    bool MathUtils::isEven(const Integer& n){
        return n % 2 == 0;
    }

    bool MathUtils::isOdd(const Integer& n){
        return n % 2 != 0;
    }


    Integer MathUtils::factorial(const Integer& n){
        Integer res = 1;
        for(Integer i = 1; i <= n; i++){
            res *= i;
        }
        return res;
    }

    std::string BitVectorUtils::bvNot(const std::string& bv){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvNot: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv.size(); i++){
            res += bv[i] == '0' ? '1' : '0';
        }
        return res;
    }
    std::string BitVectorUtils::bvAnd(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvAnd: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvAnd: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] == '1' && bv2[i] == '1' ? '1' : '0';
        }
        return res;
    }
    std::string BitVectorUtils::bvOr(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvOr: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvOr: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] == '1' || bv2[i] == '1' ? '1' : '0';
        }
        return res;
    }
    std::string BitVectorUtils::bvXor(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvXor: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvXor: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] != bv2[i] ? '1' : '0';
        }
        return res;
    }
    std::string BitVectorUtils::bvNand(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvNand: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvNand: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] == '1' && bv2[i] == '1' ? '0' : '1';
        }
        return res;
    }
    std::string BitVectorUtils::bvNor(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvNor: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvNor: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] == '0' && bv2[i] == '0' ? '1' : '0';
        }
        return res;
    }
    std::string BitVectorUtils::bvXnor(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvXnor: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvXnor: invalid bitvector");
        std::string res = "#b";
        for(size_t i = 2; i < bv1.size(); i++){
            res += bv1[i] == bv2[i] ? '1' : '0';
        }
        return res;
    }

    std::string BitVectorUtils::bvNeg(const std::string& bv){
        condAssert(bv.size() >= 2 && bv[0] == '#' && bv[1] == 'b', "invalid bitvector");
    
        std::string res = bv;
    
        // 1. bitwise NOT
        for (size_t i = 2; i < res.size(); ++i) {
            res[i] = (res[i] == '0') ? '1' : '0';
        }
    
        // 2. add 1
        bool carry = true;
        for (size_t i = res.size(); i-- > 2 && carry; ) {
            if (res[i] == '0') {
                res[i] = '1';
                carry = false;
            } else {
                res[i] = '0';
            }
        }
    
        return res;
    }    

    std::string BitVectorUtils::bvAdd(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvAdd: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvAdd: invalid bitvector");
        const size_t w = std::max(bv1.size(), bv2.size()) - 2;
        // Fast path: native 64-bit arithmetic
        if (w <= 64) {
            const uint64_t mask = (w < 64) ? ((uint64_t(1) << w) - 1) : ~uint64_t(0);
            uint64_t u1 = 0, u2 = 0;
            for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) | (bv1[i] == '1' ? 1u : 0u);
            for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) | (bv2[i] == '1' ? 1u : 0u);
            const uint64_t r = (u1 + u2) & mask;
            std::string bin(w, '0');
            uint64_t tmp = r;
            for (size_t i = 0; i < w; ++i) { bin[w-1-i] = (tmp & 1) ? '1' : '0'; tmp >>= 1; }
            return "#b" + bin;
        }
        // GMP path: arbitrary precision
        Integer u1 = 0, u2 = 0;
        for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) + (bv1[i] == '1' ? 1 : 0);
        for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) + (bv2[i] == '1' ? 1 : 0);
        return intToBv(u1 + u2, Integer(w));
    }
    std::string BitVectorUtils::bvSub(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvSub: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvSub: invalid bitvector");
        const size_t w = std::max(bv1.size(), bv2.size()) - 2;
        // Fast path: native 64-bit arithmetic
        if (w <= 64) {
            const uint64_t mask = (w < 64) ? ((uint64_t(1) << w) - 1) : ~uint64_t(0);
            uint64_t u1 = 0, u2 = 0;
            for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) | (bv1[i] == '1' ? 1u : 0u);
            for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) | (bv2[i] == '1' ? 1u : 0u);
            const uint64_t r = (u1 - u2) & mask;
            std::string bin(w, '0');
            uint64_t tmp = r;
            for (size_t i = 0; i < w; ++i) { bin[w-1-i] = (tmp & 1) ? '1' : '0'; tmp >>= 1; }
            return "#b" + bin;
        }
        // GMP path
        Integer u1 = 0, u2 = 0;
        for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) + (bv1[i] == '1' ? 1 : 0);
        for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) + (bv2[i] == '1' ? 1 : 0);
        return intToBv(u1 - u2, Integer(w));
    }
    std::string BitVectorUtils::bvMul(const std::string& bv1, const std::string& bv2) {
        condAssert(bv1.rfind("#b", 0) == 0, "BitVectorUtils::bvMul: invalid bitvector");
        condAssert(bv2.rfind("#b", 0) == 0, "BitVectorUtils::bvMul: invalid bitvector");
        const size_t w = std::max(bv1.size(), bv2.size()) - 2;
        // Fast path: native 64-bit arithmetic
        if (w <= 64) {
            const uint64_t mask = (w < 64) ? ((uint64_t(1) << w) - 1) : ~uint64_t(0);
            uint64_t u1 = 0, u2 = 0;
            for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) | (bv1[i] == '1' ? 1u : 0u);
            for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) | (bv2[i] == '1' ? 1u : 0u);
            const uint64_t r = (u1 * u2) & mask;
            std::string bin(w, '0');
            uint64_t tmp = r;
            for (size_t i = 0; i < w; ++i) { bin[w-1-i] = (tmp & 1) ? '1' : '0'; tmp >>= 1; }
            return "#b" + bin;
        }
        // GMP path: O(n log n log log n) via GMP's optimized multiplication
        Integer u1 = 0, u2 = 0;
        for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) + (bv1[i] == '1' ? 1 : 0);
        for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) + (bv2[i] == '1' ? 1 : 0);
        return intToBv(u1 * u2, Integer(w));
    }
    


    std::string BitVectorUtils::bvUdiv(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvUdiv: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvUdiv: invalid bitvector");
        const size_t w = bv1.size() - 2;
        // Divisor = 0 → return all-ones
        bool bv2IsZero = true;
        for (size_t i = 2; i < bv2.size(); ++i) { if (bv2[i] == '1') { bv2IsZero = false; break; } }
        if (bv2IsZero) return "#b" + std::string(w, '1');
        // Fast path: native 64-bit arithmetic
        if (w <= 64) {
            uint64_t u1 = 0, u2 = 0;
            for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) | (bv1[i] == '1' ? 1u : 0u);
            for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) | (bv2[i] == '1' ? 1u : 0u);
            const uint64_t r = u1 / u2;
            std::string bin(w, '0');
            uint64_t tmp = r;
            for (size_t i = 0; i < w; ++i) { bin[w-1-i] = (tmp & 1) ? '1' : '0'; tmp >>= 1; }
            return "#b" + bin;
        }
        // GMP path
        Integer u1 = 0, u2 = 0;
        for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) + (bv1[i] == '1' ? 1 : 0);
        for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) + (bv2[i] == '1' ? 1 : 0);
        return intToBv(u1 / u2, Integer(w));
    }
    std::string BitVectorUtils::bvUrem(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvUrem: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvUrem: invalid bitvector");
        const size_t w = bv1.size() - 2;
        // Divisor = 0 → return bv1
        bool bv2IsZero = true;
        for (size_t i = 2; i < bv2.size(); ++i) { if (bv2[i] == '1') { bv2IsZero = false; break; } }
        if (bv2IsZero) return bv1;
        // Fast path: native 64-bit arithmetic
        if (w <= 64) {
            uint64_t u1 = 0, u2 = 0;
            for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) | (bv1[i] == '1' ? 1u : 0u);
            for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) | (bv2[i] == '1' ? 1u : 0u);
            const uint64_t r = u1 % u2;
            std::string bin(w, '0');
            uint64_t tmp = r;
            for (size_t i = 0; i < w; ++i) { bin[w-1-i] = (tmp & 1) ? '1' : '0'; tmp >>= 1; }
            return "#b" + bin;
        }
        // GMP path
        Integer u1 = 0, u2 = 0;
        for (size_t i = 2; i < bv1.size(); ++i) u1 = (u1 << 1) + (bv1[i] == '1' ? 1 : 0);
        for (size_t i = 2; i < bv2.size(); ++i) u2 = (u2 << 1) + (bv2[i] == '1' ? 1 : 0);
        return intToBv(u1 % u2, Integer(w));
    }
    std::string BitVectorUtils::bvUmod(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvUmod: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvUmod: invalid bitvector");
        std::string res = SOMTParser::BitVectorUtils::bvUrem(bv1, bv2);
        return res;
    }
    // truncate-toward-zero division for arbitrary Integer
    static Integer truncDiv(const Integer& a, const Integer& b) {
        bool neg = (a < 0) ^ (b < 0);
        Integer aa = (a < 0 ? -a : a);
        Integer bb = (b < 0 ? -b : b);
    
        Integer q = aa / bb;
        return neg ? -q : q;
    }

    // mathematical modulo (always non-negative)
    static Integer mathMod(const Integer& a, const Integer& b) {
        // assumes b != 0
        Integer m = a % b;
        if (m < 0) m += (b > 0 ? b : -b);
        return m;
    }
    std::string BitVectorUtils::bvSdiv(const std::string& bv1,
        const std::string& bv2){
        size_t n = bv1.size() - 2;
        Integer a = bvToInt(bv1);
        Integer b = bvToInt(bv2);

        // divisor = 0
        if (b == 0) {
            // special case: 1-bit crafted semantics
            if (n == 1) {
                // all-ones
                return "#b1";
            }

            // n >= 2 : crafted semantics
            if (a < 0)
                return intToBv(Integer(1), n);
            else
                return intToBv(a, n);
        }

        // special overflow: min / -1 = min
        Integer minVal = -(Integer(1) << (n - 1));
        if (a == minVal && b == -1)
            return intToBv(minVal, n);

        // signed division truncate-toward-zero
        Integer q = truncDiv(a, b);
        return intToBv(q, n);
    }


    std::string BitVectorUtils::bvSrem(const std::string& bv1, const std::string& bv2){
        size_t n = bv1.size() - 2;
        Integer a = bvToInt(bv1);
        Integer b = bvToInt(bv2);

        if (b == 0)
        return bv1;

        Integer q = truncDiv(a, b);
        Integer r = a - q * b;   // signed remainder
        return intToBv(r, n);
    }

    std::string BitVectorUtils::bvSmod(const std::string& bv1,
        const std::string& bv2){
        size_t n = bv1.size() - 2;
        Integer a = bvToInt(bv1);
        Integer b = bvToInt(bv2);

        if (b == 0)
            return bv1;

        Integer absb = (b > 0 ? b : -b);
        Integer m = mathMod(a, absb);   // 0 <= m < |b|

        if (m == 0)
            return intToBv(Integer(0), n);

        // same sign
        if ((a > 0 && b > 0) || (a < 0 && b < 0))
            return intToBv(m, n);

        // opposite sign → sign follows b
        Integer r = absb - m;
        if (b < 0)
            r = -r;

        return intToBv(r, n);
    }


    // Normalize a BV literal (#b... or #x...) to canonical #b binary form.
    // The string-manipulating BV utilities below operate on the bit string, so
    // a hex literal must first be widened to 4 bits per hex digit.
    static std::string normBvBin(const std::string& s){
        if(s.size() >= 2 && s[0] == '#' && s[1] == 'b') return s;
        if(s.size() >= 2 && s[0] == '#' && s[1] == 'x'){
            std::string r = "#b";
            for(size_t i = 2; i < s.size(); ++i){
                char c = s[i];
                int d = (c >= '0' && c <= '9') ? c - '0'
                      : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                      : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : 0;
                for(int b = 3; b >= 0; --b) r += ((d >> b) & 1) ? '1' : '0';
            }
            return r;
        }
        return s;
    }

    std::string BitVectorUtils::bvShl(const std::string& bv_in, const std::string& n_in){
        // logical left shift: value << shift (mod 2^width)
        const std::string bv = normBvBin(bv_in);
        const std::string n  = normBvBin(n_in);
        const size_t width = bv.size() - 2;
        const size_t shift = bvToNat(n).toULong();   // shift = unsigned VALUE of n
        if(shift >= width){
            return "#b" + std::string(width, '0');
        }
        // bit string is MSB-first: a left shift drops the top `shift` bits and
        // pads `shift` zeros on the low (LSB) end.
        return "#b" + bv.substr(2 + shift) + std::string(shift, '0');
    }
    std::string BitVectorUtils::bvLshr(const std::string& bv_in, const std::string& n_in){
        // logical right shift: value >> shift (zero-fill)
        const std::string bv = normBvBin(bv_in);
        const std::string n  = normBvBin(n_in);
        const size_t width = bv.size() - 2;
        const size_t shift = bvToNat(n).toULong();
        if(shift >= width){
            return "#b" + std::string(width, '0');
        }
        // prepend `shift` zeros, keep the top `width - shift` bits.
        return "#b" + std::string(shift, '0') + bv.substr(2, width - shift);
    }
    std::string BitVectorUtils::bvAshr(const std::string& bv_in, const std::string& n_in){
        // arithmetic right shift: value >> shift (sign-fill with the MSB)
        const std::string bv = normBvBin(bv_in);
        const std::string n  = normBvBin(n_in);
        const size_t width = bv.size() - 2;
        const size_t shift = bvToNat(n).toULong();
        const char sign = bv[2];   // MSB
        if(shift >= width){
            return "#b" + std::string(width, sign);
        }
        return "#b" + std::string(shift, sign) + bv.substr(2, width - shift);
    }

    std::string BitVectorUtils::bvConcat(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvConcat: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvConcat: invalid bitvector");
        return "#b" + bv1.substr(2, bv1.size() - 2) + bv2.substr(2, bv2.size() - 2);
    }
    std::string BitVectorUtils::bvExtract(const std::string& bv, const Integer& i, const Integer& j){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvExtract: invalid bitvector");
        condAssert(i >= j, "BitVectorUtils::bvExtract: i must be greater than or equal to j");
        
        // for bitvector "#b1010", bit3=1, bit2=0, bit1=1, bit0=0
        size_t bit_width = bv.size() - 2;  // actual bit width
        size_t start_pos = 2 + (bit_width - 1 - i.toULong());  // start position from left
        size_t length = i.toULong() - j.toULong() + 1;         // length of extracted bits
        
        return "#b" + bv.substr(start_pos, length);
    }
    std::string BitVectorUtils::bvRepeat(const std::string& bv, const Integer& n){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvRepeat: invalid bitvector");
        std::string res = "";
        for(size_t i = 0; i < n.toULong(); i++){
            res += bv.substr(2, bv.size() - 2);
        }
        return "#b" + res;
    }
    std::string BitVectorUtils::bvZeroExtend(const std::string& bv, const Integer& n){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvZeroExtend: invalid bitvector");
        return "#b" + std::string(n.toULong(), '0') + bv.substr(2, bv.size() - 2);
    }
    std::string BitVectorUtils::bvSignExtend(const std::string& bv, const Integer& n){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvSignExtend: invalid bitvector");
        return "#b" + std::string(n.toULong(), bv[2]) + bv.substr(2, bv.size() - 2);
    }

    std::string BitVectorUtils::bvRotateLeft(const std::string& bv, const Integer& n){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvRotateLeft: invalid bitvector");
        Integer real_n = n % (bv.size() - 2);
        return "#b" + bv.substr(2 + n.toULong(), bv.size() - 2 - n.toULong()) + bv.substr(2, n.toULong());
    }
    std::string BitVectorUtils::bvRotateRight(const std::string& bv, const Integer& n){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvRotateRight: invalid bitvector");
        Integer real_n = n % (bv.size() - 2);
        return "#b" + bv.substr(2 + bv.size() - 2 - n.toULong(), n.toULong()) + bv.substr(2, bv.size() - 2 - n.toULong());
    }

    bool BitVectorUtils::bvComp(const std::string& bv1, const std::string& bv2, const NODE_KIND& kind){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvComp: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvComp: invalid bitvector");
        switch(kind){
            case NODE_KIND::NT_EQ_OTHER:
                return bv1 == bv2;
            case NODE_KIND::NT_DISTINCT_OTHER:
                return bv1 != bv2;
            case NODE_KIND::NT_BV_ULT:
                return SOMTParser::BitVectorUtils::bvToNat(bv1) < SOMTParser::BitVectorUtils::bvToNat(bv2);
            case NODE_KIND::NT_BV_ULE:
                return SOMTParser::BitVectorUtils::bvToNat(bv1) <= SOMTParser::BitVectorUtils::bvToNat(bv2);
            case NODE_KIND::NT_BV_UGT:
                return SOMTParser::BitVectorUtils::bvToNat(bv1) > SOMTParser::BitVectorUtils::bvToNat(bv2);
            case NODE_KIND::NT_BV_UGE:
                return SOMTParser::BitVectorUtils::bvToNat(bv1) >= SOMTParser::BitVectorUtils::bvToNat(bv2);
            case NODE_KIND::NT_BV_SLT:
                return SOMTParser::BitVectorUtils::bvToInt(bv1) < SOMTParser::BitVectorUtils::bvToInt(bv2);
            case NODE_KIND::NT_BV_SLE:
                return SOMTParser::BitVectorUtils::bvToInt(bv1) <= SOMTParser::BitVectorUtils::bvToInt(bv2);
            case NODE_KIND::NT_BV_SGT:
                return SOMTParser::BitVectorUtils::bvToInt(bv1) > SOMTParser::BitVectorUtils::bvToInt(bv2);
            case NODE_KIND::NT_BV_SGE:
                return SOMTParser::BitVectorUtils::bvToInt(bv1) >= SOMTParser::BitVectorUtils::bvToInt(bv2);
            default:
                return false;
        }
    }

    Integer BitVectorUtils::bvToNat(const std::string& bv){
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvToNat: invalid bitvector");
        Integer res = 0;
        for(size_t i = 2; i < bv.size(); i++){
            res = res * 2 + (bv[i] == '1' ? 1 : 0);
        }
        return res;
    }
    std::string BitVectorUtils::natToBv(const Integer& i, const Integer& n){
        std::string res = "#b";
        std::string bin = i.toString(2);
        if(bin.size() < n.toULong()){
            res += std::string(n.toULong() - bin.size(), '0') + bin;
        }
        else{
            res += bin.substr(bin.size() - n.toULong(), n.toULong());
        }
        return res;
    }
    std::string hexToBv(const std::string& hex){
        std::string res = "#b";
        for(size_t i = 0; i < hex.size(); i++){
            switch(hex[i]){
                case '0':
                    res += "0000";
                    break;
                case '1':
                    res += "0001";
                    break;
                case '2':
                    res += "0010";
                    break;
                case '3':
                    res += "0011";
                    break;
                case '4':
                    res += "0100";
                    break;
                case '5':
                    res += "0101";
                    break;
                case '6':
                    res += "0110";
                    break;
                case '7':
                    res += "0111";
                    break;
                case '8':
                    res += "1000";
                    break;
                case '9':
                    res += "1001";
                    break;
                case 'a':
                    res += "1010";
                    break;
                case 'A':
                    res += "1010";
                    break;
                case 'b':
                    res += "1011";
                    break;
                case 'B':
                    res += "1011";
                    break;
                case 'c':
                    res += "1100";
                    break;
                case 'C':
                    res += "1100";
                    break;
                case 'd':
                    res += "1101";
                    break;
                case 'D':
                    res += "1101";
                    break;
                case 'e':
                    res += "1110";
                    break;
                case 'E':
                    res += "1110";
                    break;
                case 'f':
                    res += "1111";
                    break;
                case 'F':
                    res += "1111";
                    break;
                default:
                    condAssert(false, "hexToBv: invalid hex character");
            }
        }
        return res;
    }
    std::string decToBv(const std::string& dec){
        std::string res = "#b";
        Integer i = Integer(dec);
        std::string bin = i.toString(2);
        return res + bin;
    }
    
    std::string BitVectorUtils::natToBv(const std::string& i, const Integer& n){
        if(i.size() > 2 && i[0] == '#' && i[1] == 'b'){
            // zero-extend
            std::string res = "#b";
            std::string bin = i.substr(2, i.size() - 2);
            if(bin.size() < n.toULong()){
                res += std::string(n.toULong() - bin.size(), '0') + bin;
            }
            else{
                res += bin.substr(bin.size() - n.toULong(), n.toULong());
            }
            return res;
        }
        else if(i.size() > 2 && i[0] == '#' && i[1] == 'x'){
            // #x -> #b
            return hexToBv(i.substr(2, i.size() - 2));
        }
        else if(i.size() > 2 && i[0] == '#' && i[1] == 'd'){
            // #d -> #b
            return decToBv(i.substr(2, i.size() - 2));
        }
        else{
            return BitVectorUtils::natToBv(Integer(i), n);
        }
    }
    Integer BitVectorUtils::bvToInt(const std::string& bv) {
        condAssert(bv.substr(0,2) == "#b", "invalid bitvector");
        const size_t n = bv.size() - 2;

        // build unsigned integer
        Integer u = 0;
        for (size_t i = 2; i < bv.size(); ++i) {
            u = (u << 1) + (bv[i] == '1' ? 1 : 0);
        }

        // two's complement signed value
        if (bv[2] == '1') {
            // MSB = 1 → negative: u - 2^n
            Integer two_n = (Integer(1) << n);
            return u - two_n;
        } else {
            // MSB = 0 → non-negative
            return u;
        }
    }

    std::string BitVectorUtils::intToBv(const Integer& i, const Integer& n) {
        const uint64_t bits = n.toULong();
        condAssert(bits > 0, "bit-width must be positive");
    
        // 1. Compute value modulo 2^n
        Integer mod = i;
        Integer two_n = Integer(1) << bits;      // 2^n
        mod = mod % two_n;                        // wrap into [-(2^n), 2^n)
        if (mod < 0) mod += two_n;               // ensure in [0, 2^n)
    
        // 2. Convert to binary string
        std::string bin = mod.toString(2);       // binary, no leading zeros
    
        // 3. Pad with zeros to exactly n bits
        if (bin.size() < bits) {
            bin = std::string(bits - bin.size(), '0') + bin;
        } else if (bin.size() > bits) {
            bin = bin.substr(bin.size() - bits, bits);  // keep low bits
        }
    
        // 4. Prepend #b
        return std::string("#b") + bin;
    }
    

    // TODO??
    std::string FloatingPointUtils::fpToUbv(const std::string& fp, const Integer& n){
        condAssert(fp[0] == '#' && fp[1] == 'x', "FloatingPointUtils::fpToUbv: invalid floating point");
        std::string res = "";
        bool isNeg = fp[2] == '1';
        if(!isNeg){
            res = fp.substr(3, fp.size() - 3);
        }
        else{
            res = fp.substr(3, fp.size() - 3);
        }
        if(res.size() < n.toULong() - 1){
            res = std::string(n.toULong() - res.size() - 1, '0') + res;
        }
        else{
            res = res.substr(res.size() - n.toULong() + 1, n.toULong() - 1);
        }
        if(isNeg){
            res = "b1" + res;
        }
        else{
            res = "b0" + res;
        }
        return res;
    }
    std::string FloatingPointUtils::fpToSbv(const std::string& fp, const Integer& n){
        condAssert(fp[0] == '#' && fp[1] == 'x', "FloatingPointUtils::fpToSbv: invalid floating point");
        std::string res = "";
        bool isNeg = fp[2] == '1';
        if(!isNeg){
            res = fp.substr(3, fp.size() - 3);
        }
        else{
            res = "b1" + fp.substr(3, fp.size() - 3);
        }
        if(res.size() < n.toULong() - 1){
            res = std::string(n.toULong() - res.size() - 1, '0') + res;
        }
        else{
            res = res.substr(res.size() - n.toULong() + 1, n.toULong() - 1);
        }
        if(isNeg){
            res = "b1" + res;
        }
        else{
            res = "b0" + res;
        }
        return res;
    }

    std::string StringUtils::strSubstr(const std::string& s, const Integer& i, const Integer& j){
        // remove the quotes
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        
        // extract the substring
        size_t start = i.toULong();
        size_t length = j.toULong();
        
        // ensure not out of range
        if (start >= s_clean.length()) {
            return "\"\"";
        }
        if (start + length > s_clean.length()) {
            length = s_clean.length() - start;
        }
        
        std::string result = s_clean.substr(start, length);
        return "\"" + result + "\"";
    }
    bool StringUtils::strPrefixof(const std::string& s, const std::string& t){
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        
        // check if s is a prefix of t
        if (s_clean.size() > t_clean.size()) return false;
        return t_clean.substr(0, s_clean.size()) == s_clean;
    }
    bool StringUtils::strSuffixof(const std::string& s, const std::string& t){
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        
        // check if s is a suffix of t
        if (s_clean.size() > t_clean.size()) return false;
        return t_clean.substr(t_clean.size() - s_clean.size(), s_clean.size()) == s_clean;
    }
    bool StringUtils::strContains(const std::string& s, const std::string& t){
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        return s_clean.find(t_clean) != std::string::npos;
    }
    Integer StringUtils::strIndexof(const std::string& s, const std::string& t, const Integer& i){
        // remove the quotes from the string
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        
        // if i is out of range, return -1
        if (i.toULong() > s_clean.length()) {
            return -1;
        }
        
        size_t pos = s_clean.find(t_clean, i.toULong());
        return (pos == std::string::npos) ? Integer(-1) : Integer(pos);
    }
    std::string StringUtils::strCharAt(const std::string& s, const Integer& i){
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        return "\"" + s_clean.substr(i.toULong(), 1) + "\"";
    }
    std::string StringUtils::strUpdate(const std::string& s, const Integer& i, const std::string& t){
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        return "\"" + s_clean.substr(0, i.toULong()) + t_clean + s_clean.substr(i.toULong() + t_clean.size(), s_clean.size() - i.toULong() - t_clean.size()) + "\"";
    }
    std::string StringUtils::strReplace(const std::string& s, const std::string& t, const std::string& u){
        // remove the quotes from the string
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        std::string u_clean = (u[0] == '"' && u[u.length()-1] == '"') ? u.substr(1, u.length()-2) : u;
        
        size_t pos = s_clean.find(t_clean);
        if(pos == std::string::npos) return s;
        std::string result = s_clean.substr(0, pos) + u_clean + s_clean.substr(pos + t_clean.length());
        // add the quotes and return
        return "\"" + result + "\"";
    }
    std::string StringUtils::strReplaceAll(const std::string& s, const std::string& t, const std::string& u){
        // remove the quotes from the string
        std::string s_clean = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        std::string t_clean = (t[0] == '"' && t[t.length()-1] == '"') ? t.substr(1, t.length()-2) : t;
        std::string u_clean = (u[0] == '"' && u[u.length()-1] == '"') ? u.substr(1, u.length()-2) : u;
        
        std::string res = s_clean;
        size_t pos = res.find(t_clean);
        while(pos != std::string::npos){
            res = res.substr(0, pos) + u_clean + res.substr(pos + t_clean.length());
            pos = res.find(t_clean, pos + u_clean.size());
        }
        // add the quotes and return
        return "\"" + res + "\"";
    }
    std::string StringUtils::strToLower(const std::string& s){
        std::string res = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        for(char& c : res){
            c = tolower(c);
        }
        return "\"" + res + "\"";
    }
    std::string StringUtils::strToUpper(const std::string& s){
        std::string res = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        for(char& c : res){
            c = toupper(c);
        }
        return "\"" + res + "\"";
    }
    std::string StringUtils::strRev(const std::string& s){
        std::string res = (s[0] == '"' && s[s.length()-1] == '"') ? s.substr(1, s.length()-2) : s;
        return "\"" + std::string(res.rbegin(), res.rend()) + "\"";
    }

    // ─── RegexUtils ─────────────────────────────────────────────────────────────
    std::optional<bool> RegexUtils::strInRe(
        const std::shared_ptr<DAGNode>& str_node,
        const std::shared_ptr<DAGNode>& regex)
    {
        if (!str_node || !str_node->isCStr()) return std::nullopt;
        if (!regex) return std::nullopt;
        return strInReHelper(str_node->getStringLiteral(), regex);
    }

    // toString
    std::string ConversionUtils::toString(const Integer& i){
        return i.toString();
    }
    std::string ConversionUtils::toString(const Real& r){
        return r.toString();
    }
    std::string ConversionUtils::toString(const int& i){
        return std::to_string(i);
    }
    std::string ConversionUtils::toString(const double& d){
        return std::to_string(d);
    }
    std::string ConversionUtils::toString(const float& f){
        return std::to_string(f);
    }
    std::string ConversionUtils::toString(const long& l){
        return std::to_string(l);
    }
    std::string ConversionUtils::toString(const short& s){
        return std::to_string(s);
    }
    std::string ConversionUtils::toString(const char& c){
        return std::string(1, c);
    }
    std::string ConversionUtils::toString(const bool& b){
        return b ? "true" : "false";
    }

    std::string ConversionUtils::escapeString(const std::string& s){
        std::string result = "";
        for(char c : s){
            // SMT-LIB2 string escaping rules:
            //   - double quote is escaped as "" (two consecutive double-quotes)
            //   - backslash is escaped as two backslashes
            //   - single quotes and other printable chars need no escaping
            //   - non-printable/control characters use \u{X} Unicode escape
            switch(c) {
                case '"':    // double quote: SMT-LIB2 standard encoding
                    result += "\"\"";
                    break;
                case '\\':   // backslash: emit as \u{5c} — under SMT-LIB 2.6 a
                             // bare backslash is literal EXCEPT when it forms \u,
                             // so this is the only unambiguous round-trip encoding
                    result += "\\u{5c}";
                    break;
                default:
                    // printable ASCII: no escaping needed
                    if(c >= 32 && c <= 126) {
                        result += c;
                    } else {
                        // non-printable: use SMT-LIB2 Unicode escape \u{X}
                        std::ostringstream oss;
                        oss << "\\u{" << std::hex << static_cast<unsigned int>((unsigned char)c) << "}";
                        result += oss.str();
                    }
                    break;
            }
        }
        return result;
    }

    std::string ConversionUtils::unescapeString(const std::string& s){
        // SMT-LIB 2.6 string literal semantics: the ONLY escape sequences are
        //   ""      -> one literal double quote (outer quotes already stripped)
        //   \u{H+}  -> Unicode code point, 1..5 hex digits
        //   \uHHHH  -> Unicode code point, exactly 4 hex digits
        // Everything else, INCLUDING \n, \r, \\, is literal characters.
        // (The previous C-style decoding shrank literals like "\r\n" to CRLF,
        // making string-length objectives disagree with the standard and z3.)
        std::string result = "";
        auto isHex = [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                   (c >= 'A' && c <= 'F');
        };
        auto appendCodePoint = [&result](unsigned long cp) {
            if (cp <= 0x7F) {
                result += static_cast<char>(cp);
            } else if (cp <= 0x7FF) {
                result += static_cast<char>(0xC0 | (cp >> 6));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else if (cp <= 0xFFFF) {
                result += static_cast<char>(0xE0 | (cp >> 12));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (cp >> 18));
                result += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (cp & 0x3F));
            }
        };
        size_t i = 0;
        while (i < s.length()) {
            if (s[i] == '"' && i + 1 < s.length() && s[i + 1] == '"') {
                result += '"';
                i += 2;
                continue;
            }
            if (s[i] == '\\' && i + 1 < s.length() && s[i + 1] == 'u') {
                if (i + 2 < s.length() && s[i + 2] == '{') {
                    size_t close = s.find('}', i + 3);
                    if (close != std::string::npos && close > i + 3 &&
                        close - (i + 3) <= 5) {
                        std::string hex = s.substr(i + 3, close - (i + 3));
                        bool allHex = true;
                        for (char h : hex) { if (!isHex(h)) { allHex = false; break; } }
                        if (allHex) {
                            appendCodePoint(std::stoul(hex, nullptr, 16));
                            i = close + 1;
                            continue;
                        }
                    }
                } else if (i + 5 < s.length() && isHex(s[i + 2]) && isHex(s[i + 3]) &&
                           isHex(s[i + 4]) && isHex(s[i + 5])) {
                    appendCodePoint(std::stoul(s.substr(i + 2, 4), nullptr, 16));
                    i += 6;
                    continue;
                }
            }
            result += s[i];
            i++;
        }
        return result;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — FPValue methods
    // ═══════════════════════════════════════════════════════════════════════

    bool FloatingPointUtils::FPValue::isNaN() const {
        uint64_t max_exp = (1ULL << eb) - 1;
        return exponent == max_exp && significand != 0;
    }
    bool FloatingPointUtils::FPValue::isInf() const {
        uint64_t max_exp = (1ULL << eb) - 1;
        return exponent == max_exp && significand == 0;
    }
    bool FloatingPointUtils::FPValue::isZero() const {
        return exponent == 0 && significand == 0;
    }
    bool FloatingPointUtils::FPValue::isNeg() const {
        return sign == 1;
    }
    bool FloatingPointUtils::FPValue::isNormal() const {
        uint64_t max_exp = (1ULL << eb) - 1;
        return exponent != 0 && exponent != max_exp;
    }
    bool FloatingPointUtils::FPValue::isSubnormal() const {
        return exponent == 0 && significand != 0;
    }

    std::string FloatingPointUtils::FPValue::toSMTFP() const {
        return "(fp " + uint64ToBinStr(sign, 1)
             + " "    + uint64ToBinStr(exponent, eb)
             + " "    + uint64ToBinStr(significand, sb - 1) + ")";
    }

    std::optional<float> FloatingPointUtils::FPValue::toFloat32() const {
        if(eb != 8 || sb != 24) return std::nullopt;
        uint32_t bits = (static_cast<uint32_t>(sign) << 31)
                      | (static_cast<uint32_t>(exponent) << 23)
                      | static_cast<uint32_t>(significand);
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }

    std::optional<double> FloatingPointUtils::FPValue::toFloat64() const {
        if(eb != 11 || sb != 53) return std::nullopt;
        uint64_t bits = (sign << 63) | (exponent << 52) | significand;
        double d;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }

    FloatingPointUtils::FPValue FloatingPointUtils::FPValue::fromFloat32(float f) {
        uint32_t bits;
        std::memcpy(&bits, &f, sizeof(bits));
        FPValue v;
        v.sign = (bits >> 31) & 1u;
        v.exponent = (bits >> 23) & 0xFFu;
        v.significand = bits & 0x7FFFFFu;
        v.eb = 8;
        v.sb = 24;
        return v;
    }

    FloatingPointUtils::FPValue FloatingPointUtils::FPValue::fromFloat64(double d) {
        uint64_t bits;
        std::memcpy(&bits, &d, sizeof(bits));
        FPValue v;
        v.sign = (bits >> 63) & 1ull;
        v.exponent = (bits >> 52) & 0x7FFull;
        v.significand = bits & 0x000FFFFFFFFFFFFFull;
        v.eb = 11;
        v.sb = 53;
        return v;
    }

    FloatingPointUtils::FPValue FloatingPointUtils::FPValue::abs() const {
        FPValue r = *this;
        r.sign = 0;
        return r;
    }

    FloatingPointUtils::FPValue FloatingPointUtils::FPValue::neg() const {
        FPValue r = *this;
        r.sign = sign ^ 1;
        return r;
    }

    bool FloatingPointUtils::FPValue::toMpfr(mpfr_t out) const {
        if(isNaN()) {
            mpfr_set_nan(out);
            return false; // caller must handle NaN specially
        }
        if(isInf()) {
            mpfr_set_inf(out, sign ? -1 : 1);
            return true;
        }
        if(isZero()) {
            mpfr_set_zero(out, sign ? -1 : 1);
            return true;
        }

        // bias = 2^(eb-1) - 1
        int64_t bias = (1LL << (eb - 1)) - 1;
        size_t mant_bits = sb - 1; // number of explicit significand bits

        if(isSubnormal()) {
            // value = (-1)^sign × 0.significand × 2^(1 - bias)
            // = (-1)^sign × significand × 2^(1 - bias - mant_bits)
            mpfr_set_ui(out, 0, MPFR_RNDN);
            if(significand != 0) {
                mpfr_set_uj(out, significand, MPFR_RNDN);
                // exponent for subnormal: 1 - bias - mant_bits
                int64_t exp_val = 1 - bias - static_cast<int64_t>(mant_bits);
                mpfr_mul_2si(out, out, exp_val, MPFR_RNDN);
            }
        } else {
            // Normal: value = (-1)^sign × 1.significand × 2^(exponent - bias)
            // = (-1)^sign × (2^mant_bits + significand) × 2^(exponent - bias - mant_bits)
            uint64_t full_mant = (1ULL << mant_bits) | significand;
            mpfr_set_uj(out, full_mant, MPFR_RNDN);
            int64_t exp_val = static_cast<int64_t>(exponent) - bias - static_cast<int64_t>(mant_bits);
            mpfr_mul_2si(out, out, exp_val, MPFR_RNDN);
        }

        if(sign) mpfr_neg(out, out, MPFR_RNDN);
        return true;
    }

    FloatingPointUtils::FPValue FloatingPointUtils::FPValue::fromMpfr(const mpfr_t val, size_t eb, size_t sb) {
        FPValue v;
        v.eb = eb;
        v.sb = sb;
        size_t mant_bits = sb - 1;
        uint64_t max_exp = (1ULL << eb) - 1;
        int64_t bias = (1LL << (eb - 1)) - 1;

        // NaN
        if(mpfr_nan_p(val)) {
            v.sign = 0;
            v.exponent = max_exp;
            v.significand = 1; // canonical quiet NaN
            return v;
        }

        // Inf
        if(mpfr_inf_p(val)) {
            v.sign = mpfr_signbit(val) ? 1 : 0;
            v.exponent = max_exp;
            v.significand = 0;
            return v;
        }

        // Zero
        if(mpfr_zero_p(val)) {
            v.sign = mpfr_signbit(val) ? 1 : 0;
            v.exponent = 0;
            v.significand = 0;
            return v;
        }

        v.sign = mpfr_signbit(val) ? 1 : 0;

        // Work with absolute value
        mpfr_t abs_val;
        mpfr_init2(abs_val, mpfr_get_prec(val));
        mpfr_abs(abs_val, val, MPFR_RNDN);

        // Get the exponent: val = m × 2^exp where 0.5 <= m < 1
        mpfr_exp_t raw_exp;
        mpfr_t frac;
        mpfr_init2(frac, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_frexp(&raw_exp, frac, abs_val, MPFR_RNDN);
        // Now frac ∈ [0.5, 1), val = frac × 2^raw_exp
        // IEEE: val = 1.mantissa × 2^(biased_exp - bias)
        // So: 1.mantissa = frac × 2, and biased_exp = raw_exp - 1 + bias

        int64_t biased_exp = raw_exp - 1 + bias;

        if(biased_exp >= static_cast<int64_t>(max_exp)) {
            // Overflow → infinity
            v.exponent = max_exp;
            v.significand = 0;
        } else if(biased_exp >= 1) {
            // Normal number
            v.exponent = static_cast<uint64_t>(biased_exp);
            // Extract mantissa bits: frac × 2 gives 1.xxxx, we want the xxxx part
            // frac × 2^(mant_bits+1) gives (1.xxxx) × 2^mant_bits = integer with hidden bit
            mpfr_mul_2ui(frac, frac, mant_bits + 1, MPFR_RNDN);
            uint64_t full_mant = mpfr_get_uj(frac, MPFR_RNDZ);
            v.significand = full_mant & ((1ULL << mant_bits) - 1); // remove hidden bit
        } else {
            // Subnormal or underflow to zero
            // Subnormal: biased_exp would be 0, actual exp is 1 - bias
            // val = 0.significand × 2^(1-bias)
            // significand = val × 2^(bias-1+mant_bits) = val × 2^(bias-1+mant_bits)
            int64_t shift = bias - 1 + static_cast<int64_t>(mant_bits);
            mpfr_t sub_val;
            mpfr_init2(sub_val, static_cast<mpfr_prec_t>(sb + 4));
            mpfr_mul_2si(sub_val, abs_val, shift, MPFR_RNDN);
            uint64_t sub_mant = mpfr_get_uj(sub_val, MPFR_RNDZ);
            mpfr_clear(sub_val);
            if(sub_mant == 0) {
                // Underflow to zero
                v.exponent = 0;
                v.significand = 0;
            } else {
                v.exponent = 0;
                v.significand = sub_mant & ((1ULL << mant_bits) - 1);
            }
        }

        mpfr_clear(abs_val);
        mpfr_clear(frac);
        return v;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — rounding mode
    // ═══════════════════════════════════════════════════════════════════════

    int FloatingPointUtils::smtRMToFeround(const std::string& rm_name) {
        if(rm_name == "roundNearestTiesToEven" || rm_name == "RNE") return FE_TONEAREST;
        if(rm_name == "roundNearestTiesToAway" || rm_name == "RNA") return FE_TONEAREST; // best effort
        if(rm_name == "roundTowardPositive"    || rm_name == "RTP") return FE_UPWARD;
        if(rm_name == "roundTowardNegative"    || rm_name == "RTN") return FE_DOWNWARD;
        if(rm_name == "roundTowardZero"        || rm_name == "RTZ") return FE_TOWARDZERO;
        return FE_TONEAREST;
    }

    int FloatingPointUtils::getFPRoundingMode(const std::shared_ptr<DAGNode>& rm_node) {
        if(!rm_node || rm_node->isNull()) return FE_TONEAREST;
        return smtRMToFeround(rm_node->getName());
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — MPFR rounding mode
    // ═══════════════════════════════════════════════════════════════════════

    mpfr_rnd_t FloatingPointUtils::smtRMToMpfrRound(const std::string& rm_name) {
        if(rm_name == "roundNearestTiesToEven" || rm_name == "RNE") return MPFR_RNDN;
        if(rm_name == "roundNearestTiesToAway" || rm_name == "RNA") return MPFR_RNDA;
        if(rm_name == "roundTowardPositive"    || rm_name == "RTP") return MPFR_RNDU;
        if(rm_name == "roundTowardNegative"    || rm_name == "RTN") return MPFR_RNDD;
        if(rm_name == "roundTowardZero"        || rm_name == "RTZ") return MPFR_RNDZ;
        return MPFR_RNDN;
    }

    mpfr_rnd_t FloatingPointUtils::getFPRoundingModeMpfr(const std::shared_ptr<DAGNode>& rm_node) {
        if(!rm_node || rm_node->isNull()) return MPFR_RNDN;
        return smtRMToMpfrRound(rm_node->getName());
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — Generic MPFR-based FP operations
    // ═══════════════════════════════════════════════════════════════════════

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpUnaryOp(const FPValue& a, size_t eb, size_t sb,
                                   mpfr_rnd_t rnd, MpfrUnaryFn op) {
        // Handle NaN input
        if(a.isNaN()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        mpfr_t ma, mr;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mr, static_cast<mpfr_prec_t>(sb + 4));
        a.toMpfr(ma);
        op(mr, ma, rnd);
        FPValue result = FPValue::fromMpfr(mr, eb, sb);
        mpfr_clear(ma);
        mpfr_clear(mr);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpBinaryOp(const FPValue& a, const FPValue& b,
                                    size_t eb, size_t sb,
                                    mpfr_rnd_t rnd, MpfrBinaryFn op) {
        // NaN propagation: if either is NaN, result is NaN
        if(a.isNaN() || b.isNaN()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        mpfr_t ma, mb, mr;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mb, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mr, static_cast<mpfr_prec_t>(sb + 4));
        a.toMpfr(ma);
        b.toMpfr(mb);
        op(mr, ma, mb, rnd);
        FPValue result = FPValue::fromMpfr(mr, eb, sb);
        mpfr_clear(ma);
        mpfr_clear(mb);
        mpfr_clear(mr);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpTernaryOp(const FPValue& a, const FPValue& b,
                                     const FPValue& c, size_t eb, size_t sb,
                                     mpfr_rnd_t rnd, MpfrTernaryFn op) {
        if(a.isNaN() || b.isNaN() || c.isNaN()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        mpfr_t ma, mb, mc, mr;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mb, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mc, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mr, static_cast<mpfr_prec_t>(sb + 4));
        a.toMpfr(ma);
        b.toMpfr(mb);
        c.toMpfr(mc);
        op(mr, ma, mb, mc, rnd);
        FPValue result = FPValue::fromMpfr(mr, eb, sb);
        mpfr_clear(ma);
        mpfr_clear(mb);
        mpfr_clear(mc);
        mpfr_clear(mr);
        return result;
    }

    int FloatingPointUtils::fpCompare(const FPValue& a, const FPValue& b) {
        // NaN is unordered with everything (including itself)
        if(a.isNaN() || b.isNaN()) return 2;

        // Both zeros: +0 == -0
        if(a.isZero() && b.isZero()) return 0;

        mpfr_t ma, mb;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(std::max(a.sb, b.sb) + 4));
        mpfr_init2(mb, static_cast<mpfr_prec_t>(std::max(a.sb, b.sb) + 4));
        a.toMpfr(ma);
        b.toMpfr(mb);
        int cmp = mpfr_cmp(ma, mb);
        mpfr_clear(ma);
        mpfr_clear(mb);
        if(cmp < 0) return -1;
        if(cmp > 0) return 1;
        return 0;
    }

    bool FloatingPointUtils::fpValueIdentical(const FPValue& a, const FPValue& b) {
        return a.eb == b.eb && a.sb == b.sb && a.sign == b.sign && a.exponent == b.exponent &&
               a.significand == b.significand;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpRemainder(const FPValue& a, const FPValue& b,
                                     size_t eb, size_t sb) {
        if(a.isNaN() || b.isNaN()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        // remainder(±Inf, y) = NaN; remainder(x, ±0) = NaN
        if(a.isInf() || b.isZero()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        mpfr_t ma, mb, mr;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mb, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mr, static_cast<mpfr_prec_t>(sb + 4));
        a.toMpfr(ma);
        b.toMpfr(mb);
        mpfr_remainder(mr, ma, mb, MPFR_RNDN);
        FPValue result = FPValue::fromMpfr(mr, eb, sb);
        mpfr_clear(ma);
        mpfr_clear(mb);
        mpfr_clear(mr);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpRoundToIntegral(const FPValue& a,
                                           size_t eb, size_t sb, mpfr_rnd_t rnd) {
        if(a.isNaN()) {
            FPValue nan_v;
            nan_v.eb = eb; nan_v.sb = sb; nan_v.sign = 0;
            nan_v.exponent = (1ULL << eb) - 1; nan_v.significand = 1;
            return nan_v;
        }
        if(a.isInf() || a.isZero()) return a; // ±Inf and ±0 round to themselves
        mpfr_t ma, mr;
        mpfr_init2(ma, static_cast<mpfr_prec_t>(sb + 4));
        mpfr_init2(mr, static_cast<mpfr_prec_t>(sb + 4));
        a.toMpfr(ma);
        mpfr_rint(mr, ma, rnd);
        FPValue result = FPValue::fromMpfr(mr, eb, sb);
        mpfr_clear(ma);
        mpfr_clear(mr);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpMin(const FPValue& a, const FPValue& b) {
        // IEEE 754 / SMT-LIB: if either is NaN, return the other
        if(a.isNaN() && b.isNaN()) return a; // both NaN → NaN
        if(a.isNaN()) return b;
        if(b.isNaN()) return a;

        int cmp = fpCompare(a, b);
        if(cmp <= 0) return a; // a <= b or a == b
        return b;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::fpMax(const FPValue& a, const FPValue& b) {
        if(a.isNaN() && b.isNaN()) return a;
        if(a.isNaN()) return b;
        if(b.isNaN()) return a;

        int cmp = fpCompare(a, b);
        if(cmp >= 0) return a; // a >= b or a == b
        return b;
    }

    std::optional<double> FloatingPointUtils::fpToDouble(const FPValue& v) {
        if(v.isNaN() || v.isInf()) return std::nullopt;
        if(v.isZero()) return v.sign ? -0.0 : 0.0;
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(v.sb + 4));
        v.toMpfr(m);
        double d = mpfr_get_d(m, MPFR_RNDN);
        mpfr_clear(m);
        return d;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — FP conversion helpers
    // ═══════════════════════════════════════════════════════════════════════

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::realToFpValue(const Number& real, size_t eb, size_t sb, mpfr_rnd_t rnd) {
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(sb + 16)); // extra precision for rounding
        // Use storage kind, not mathematical isInteger(): REAL_TYPE values like 10.0
        // satisfy isInteger() but must use getReal(), not getInteger().
        if(real.getType() == Number::INT_TYPE) {
            auto& intVal = real.getInteger();
            mpfr_set_z(m, intVal.getMPZ().get_mpz_t(), rnd);
        } else if(real.getType() == Number::REAL_TYPE) {
            mpfr_srcptr src = real.getReal().getMPFR();
            mpfr_set(m, src, rnd);
        } else {
            mpfr_clear(m);
            return std::nullopt;
        }
        // Now round m to target FP format
        FPValue result = FPValue::fromMpfr(m, eb, sb);
        mpfr_clear(m);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::bvToFpValueSigned(uint64_t bv_val, size_t bv_width, size_t eb, size_t sb, mpfr_rnd_t rnd) {
        // Interpret BV as signed 2's complement integer
        int64_t signed_val;
        if(bv_width < 64 && (bv_val & (1ULL << (bv_width - 1)))) {
            // Negative: sign-extend
            signed_val = static_cast<int64_t>(bv_val | (~0ULL << bv_width));
        } else {
            signed_val = static_cast<int64_t>(bv_val);
        }
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(sb + 16));
        mpfr_set_sj(m, signed_val, rnd);
        FPValue result = FPValue::fromMpfr(m, eb, sb);
        mpfr_clear(m);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::bvToFpValueUnsigned(uint64_t bv_val, size_t bv_width, size_t eb, size_t sb, mpfr_rnd_t rnd) {
        (void)bv_width; // bv_val is already masked to width by parseBVBits
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(sb + 16));
        mpfr_set_uj(m, bv_val, rnd);
        FPValue result = FPValue::fromMpfr(m, eb, sb);
        mpfr_clear(m);
        return result;
    }

    std::optional<FloatingPointUtils::FPValue>
    FloatingPointUtils::bvBitsToFpValue(const std::string& bv_name, size_t eb, size_t sb) {
        // Parse all bits from BV string
        uint64_t all_bits = parseBVBits(bv_name);
        size_t total_bits = 1 + eb + (sb - 1); // sign + exponent + mantissa
        // Extract fields from MSB to LSB: [sign(1)] [exponent(eb)] [significand(sb-1)]
        size_t mant_bits = sb - 1;
        FPValue v;
        v.eb = eb;
        v.sb = sb;
        v.significand = all_bits & ((1ULL << mant_bits) - 1);
        all_bits >>= mant_bits;
        v.exponent = all_bits & ((1ULL << eb) - 1);
        all_bits >>= eb;
        v.sign = all_bits & 1;
        (void)total_bits;
        return v;
    }

    std::optional<std::string>
    FloatingPointUtils::fpValueToUbv(const FPValue& v, size_t bv_width, mpfr_rnd_t rnd) {
        if(v.isNaN() || v.isInf()) return std::nullopt;
        if(v.isZero()) return "0"; // +0 and -0 → 0
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(v.sb + 4));
        v.toMpfr(m);
        // Check negative → undefined for unsigned
        if(mpfr_sgn(m) < 0) { mpfr_clear(m); return std::nullopt; }
        // Round to integer in target rounding mode
        mpfr_rint(m, m, rnd);
        // Check range [0, 2^bv_width)
        mpfr_t max_val;
        mpfr_init2(max_val, static_cast<mpfr_prec_t>(bv_width + 4));
        mpfr_set_ui(max_val, 1, MPFR_RNDN);
        mpfr_mul_2ui(max_val, max_val, bv_width, MPFR_RNDN); // max_val = 2^bv_width
        if(mpfr_cmp(m, max_val) >= 0) { mpfr_clear(m); mpfr_clear(max_val); return std::nullopt; }
        mpfr_clear(max_val);
        uint64_t val = mpfr_get_uj(m, MPFR_RNDZ);
        mpfr_clear(m);
        return std::to_string(val);
    }

    std::optional<std::string>
    FloatingPointUtils::fpValueToSbv(const FPValue& v, size_t bv_width, mpfr_rnd_t rnd) {
        if(v.isNaN() || v.isInf()) return std::nullopt;
        if(v.isZero()) return "0";
        mpfr_t m;
        mpfr_init2(m, static_cast<mpfr_prec_t>(v.sb + 4));
        v.toMpfr(m);
        // Round to integer in target rounding mode
        mpfr_rint(m, m, rnd);
        // Check range [-2^(bv_width-1), 2^(bv_width-1))
        int64_t min_val = -(1LL << (bv_width - 1));
        int64_t max_val = (1LL << (bv_width - 1));
        int64_t int_val = mpfr_get_sj(m, MPFR_RNDZ);
        mpfr_clear(m);
        if(int_val < min_val || int_val >= max_val) return std::nullopt;
        // Convert to unsigned 2's complement representation
        uint64_t unsigned_val;
        if(int_val >= 0) {
            unsigned_val = static_cast<uint64_t>(int_val);
        } else {
            unsigned_val = static_cast<uint64_t>(int_val + (1LL << bv_width));
        }
        return std::to_string(unsigned_val);
    }

    uint64_t FloatingPointUtils::parseBVBits(const std::string& bv_name) {
        if(bv_name.size() < 2) return 0;
        if(bv_name[0] == '#' && bv_name[1] == 'b') {
            uint64_t val = 0;
            for(size_t i = 2; i < bv_name.size(); ++i)
                val = (val << 1) | (bv_name[i] == '1' ? 1u : 0u);
            return val;
        }
        if(bv_name[0] == '#' && bv_name[1] == 'x') {
            uint64_t val = 0;
            for(size_t i = 2; i < bv_name.size(); ++i) {
                val <<= 4;
                char c = bv_name[i];
                if(c >= '0' && c <= '9') val |= static_cast<uint64_t>(c - '0');
                else if(c >= 'a' && c <= 'f') val |= static_cast<uint64_t>(c - 'a' + 10);
                else if(c >= 'A' && c <= 'F') val |= static_cast<uint64_t>(c - 'A' + 10);
            }
            return val;
        }
        return 0;
    }

    std::string FloatingPointUtils::uint64ToBinStr(uint64_t val, size_t width) {
        std::string s(width, '0');
        for(size_t i = 0; i < width; ++i)
            s[width - 1 - i] = ((val >> i) & 1u) ? '1' : '0';
        return "#b" + s;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — float reconstruction
    // ═══════════════════════════════════════════════════════════════════════

    float FloatingPointUtils::reconstructFloat32(uint32_t sign_bit, uint32_t exp_bits, uint32_t mant_bits) {
        uint32_t bits = (sign_bit << 31) | (exp_bits << 23) | mant_bits;
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return f;
    }

    double FloatingPointUtils::reconstructFloat64(uint64_t sign_bit, uint64_t exp_bits, uint64_t mant_bits) {
        uint64_t bits = (sign_bit << 63) | (exp_bits << 52) | mant_bits;
        double d;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — float → SMT-LIB string
    // ═══════════════════════════════════════════════════════════════════════

    std::string FloatingPointUtils::float32ToSMTFP(float f) {
        return FPValue::fromFloat32(f).toSMTFP();
    }

    std::string FloatingPointUtils::float64ToSMTFP(double d) {
        return FPValue::fromFloat64(d).toSMTFP();
    }

    std::string FloatingPointUtils::fpValueToSMTFP(const FPValue& v) {
        return v.toSMTFP();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — DAGNode → FPValue / float / double
    // ═══════════════════════════════════════════════════════════════════════

    // Generic: parse any FP constant node into an FPValue
    std::optional<FloatingPointUtils::FPValue> FloatingPointUtils::fpNodeToValue(const std::shared_ptr<DAGNode>& node) {
        if(!node) return std::nullopt;
        auto sort = node->getSort();
        if(!sort || !sort->isFp()) return std::nullopt;

        size_t eb = sort->getExponentWidth();
        size_t sb = sort->getSignificandWidth();
        FPValue v;
        v.eb = eb;
        v.sb = sb;

        // Check node kinds for special constants
        if(node->isNaN()) {
            v.sign = 0;
            v.exponent = (1ULL << eb) - 1;
            v.significand = 1; // canonical quiet NaN
            return v;
        }
        if(node->isPosInfinity()) {
            v.sign = 0;
            v.exponent = (1ULL << eb) - 1;
            v.significand = 0;
            return v;
        }
        if(node->isNegInfinity()) {
            v.sign = 1;
            v.exponent = (1ULL << eb) - 1;
            v.significand = 0;
            return v;
        }

        if(!node->isCFP() && !node->isConst()) return std::nullopt;

        const std::string& name = node->getName();

        // Check for +zero/-zero by name prefix (these are NT_CONST with FP sort)
        if(name.size() > 8 && name.rfind("(_ +zero", 0) == 0) {
            v.sign = 0; v.exponent = 0; v.significand = 0;
            return v;
        }
        if(name.size() > 8 && name.rfind("(_ -zero", 0) == 0) {
            v.sign = 1; v.exponent = 0; v.significand = 0;
            return v;
        }

        // (a) bit_representation form with BV children
        if(name == "(fp_bit_representation)" && node->getChildren().size() == 3) {
            auto ch = node->getChildren();
            v.sign = parseBVBits(ch[0]->getName());
            v.exponent = parseBVBits(ch[1]->getName());
            v.significand = parseBVBits(ch[2]->getName());
            return v;
        }

        // (b) "(fp #bS #bEEE #bMMM...)" form
        if(name.size() > 4 && name.rfind("(fp ", 0) == 0) {
            std::istringstream iss(name.substr(1, name.size() - 2));
            std::string tok;
            std::vector<std::string> tokens;
            while(iss >> tok) tokens.push_back(tok);
            if(tokens.size() == 4) {
                v.sign = parseBVBits(tokens[1]);
                v.exponent = parseBVBits(tokens[2]);
                v.significand = parseBVBits(tokens[3]);
                return v;
            }
        }

        return std::nullopt;
    }

    std::optional<float> FloatingPointUtils::fpNodeToFloat32(const std::shared_ptr<DAGNode>& node) {
        auto v = fpNodeToValue(node);
        if(!v) return std::nullopt;
        return v->toFloat32();
    }

    std::optional<double> FloatingPointUtils::fpNodeToFloat64(const std::shared_ptr<DAGNode>& node) {
        auto v = fpNodeToValue(node);
        if(!v) return std::nullopt;
        return v->toFloat64();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // FloatingPointUtils — special value checks (node-kind based, no string matching)
    // ═══════════════════════════════════════════════════════════════════════

    bool FloatingPointUtils::fpNodeIsNaN(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        if(node->isNaN()) return true;  // NT_NAN kind — direct check
        auto v = fpNodeToValue(node);
        return v && v->isNaN();
    }

    bool FloatingPointUtils::fpNodeIsInf(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        if(node->isPosInfinity() || node->isNegInfinity()) return true;
        auto v = fpNodeToValue(node);
        return v && v->isInf();
    }

    bool FloatingPointUtils::fpNodeIsZero(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        auto v = fpNodeToValue(node);
        return v && v->isZero();
    }

    bool FloatingPointUtils::fpNodeIsNeg(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        if(node->isNaN()) return false;  // NaN has no sign per SMT-LIB
        if(node->isNegInfinity()) return true;
        auto v = fpNodeToValue(node);
        return v && !v->isNaN() && v->isNeg();
    }

    bool FloatingPointUtils::fpNodeIsNormal(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        auto v = fpNodeToValue(node);
        return v && v->isNormal();
    }

    bool FloatingPointUtils::fpNodeIsSubnormal(const std::shared_ptr<DAGNode>& node) {
        if(!node) return false;
        auto v = fpNodeToValue(node);
        return v && v->isSubnormal();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // UF utilities
    // ═══════════════════════════════════════════════════════════════════════

    namespace detail {
        static void collectUFRec(
            const std::shared_ptr<DAGNode>& node,
            std::unordered_map<std::string, std::vector<UFApplication>>& out,
            std::unordered_set<const DAGNode*>& visited)
        {
            if(!node || node->isNull()) return;
            if(!visited.insert(node.get()).second) return;

            if(node->isUFApplication()) {
                UFApplication app;
                app.func_name        = node->getName();
                app.args             = node->getChildren();
                app.application_node = node;
                app.result_sort      = node->getSort();
                out[app.func_name].push_back(std::move(app));
            }

            for(const auto& child : node->getChildren())
                collectUFRec(child, out, visited);
        }
    } // namespace detail

    std::unordered_map<std::string, std::vector<UFApplication>>
    collectUFApplications(const std::vector<std::shared_ptr<DAGNode>>& assertions) {
        std::unordered_map<std::string, std::vector<UFApplication>> result;
        std::unordered_set<const DAGNode*> visited;
        for(const auto& a : assertions)
            detail::collectUFRec(a, result, visited);
        return result;
    }

    std::string formatUFDefine(
        const std::string& func_name,
        const std::vector<std::string>& param_sorts,
        const std::string& result_sort,
        const std::vector<UFTableEntry>& entries,
        const std::string& default_value)
    {
        std::string params;
        for(size_t i = 0; i < param_sorts.size(); ++i) {
            if(i > 0) params += ' ';
            params += "(x" + std::to_string(i) + " " + param_sorts[i] + ")";
        }

        std::string body = default_value;
        for(int i = static_cast<int>(entries.size()) - 1; i >= 0; --i) {
            const auto& e = entries[static_cast<size_t>(i)];
            std::string cond;
            if(e.arg_values.size() == 1) {
                cond = "(= x0 " + e.arg_values[0] + ")";
            } else {
                cond = "(and";
                for(size_t j = 0; j < e.arg_values.size(); ++j)
                    cond += " (= x" + std::to_string(j) + " " + e.arg_values[j] + ")";
                cond += ")";
            }
            body = "(ite " + cond + " " + e.result_value + "\n  " + body + ")";
        }

        return "(define-fun " + func_name
             + " (" + params + ") " + result_sort
             + "\n  " + body + ")";
    }

}
