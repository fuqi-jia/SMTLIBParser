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
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <string>
#include <mpfr.h>
namespace SOMTParser{

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
        for (size_t i = 0; i < str.size(); i++){
            if (i == 0 && (str[i] == '-' || str[i] == '+')) continue;
            if (str[i] == '.' && !has_dot){
                has_dot = true;
                continue;
            }
            if (!isdigit(str[i])) return false;
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
                                     [](unsigned char c) { return std::isspace(c); }), 
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
        // find 'E' or 'e' character
        size_t e_pos = str.find_first_of("Ee");
        if (e_pos == std::string::npos) 
            return str;
            
        try {
            // extract the mantissa part
            std::string mantissa = str.substr(0, e_pos);
            
            // check if the mantissa part is a valid real number
            if (!TypeChecker::isReal(mantissa))
                return str;
            
            // extract the exponent part
            std::string exponent = str.substr(e_pos + 1);
            
            // if the exponent part is empty, return the original string
            if (exponent.empty())
                return str;
            
            // create a copy without spaces for processing
            std::string exponent_no_spaces = exponent;
            exponent_no_spaces.erase(std::remove_if(exponent_no_spaces.begin(), exponent_no_spaces.end(), 
                                         [](unsigned char c) { return std::isspace(c); }), 
                          exponent_no_spaces.end());
            
            // if the exponent part is empty after removing spaces, return the original string
            if (exponent_no_spaces.empty())
                return str;
            
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
            
            // if the exponent part is empty after handling parentheses, return the original string
            if (exponent_no_spaces.empty())
                return str;
            
            // convert scientific notation to normal real number
            // TODO!!
            Real mantissa_val = Real(mantissa);
            Real exponent_val = Real(exponent_no_spaces);
            
            // calculate the result
            Real result = mantissa_val * SOMTParser::MathUtils::pow(Real(10.0), exponent_val);
            
            // convert to string
            std::ostringstream oss;
            oss << std::setprecision(16) << toString(result);
            return oss.str();
        } catch (const std::exception& e) {
            // conversion failed, return the original string
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
        std::string bv1_ = bv1.substr(2, bv1.size() - 2);
        std::string bv2_ = bv2.substr(2, bv2.size() - 2);
        if(bv1_.size() != bv2_.size()){
            // add prefix 0 to the shorter one
            if(bv1_.size() < bv2_.size()){
                bv1_ = "#b" + std::string(bv2_.size() - bv1_.size(), '0') + bv1_;
                bv2_ = "#b" + bv2_;
            }
            else{
                bv2_ = "#b" + std::string(bv1_.size() - bv2_.size(), '0') + bv2_;
                bv1_ = "#b" + bv1_;
            }
        }
        else{
            bv1_ = "#b" + bv1_;
            bv2_ = "#b" + bv2_;
        }   
        std::string res = "";
        bool carry = false;
        for(size_t i = bv1_.size() - 1; i >= 2; i--){
            if(bv1_[i] == '0' && bv2_[i] == '0'){
                res += carry ? '1' : '0';
                carry = false;
            }
            else if(bv1_[i] == '1' && bv2_[i] == '1'){
                res += carry ? '1' : '0';
                carry = true;
            }
            else{
                res += carry ? '0' : '1';
            }
        }
        // add #b prefix and reverse
        res = std::string(res.rbegin(), res.rend());
        res = "#b" + res;
        return res;
    }
    std::string BitVectorUtils::bvSub(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvSub: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvSub: invalid bitvector");
        std::string bv1_ = bv1.substr(2, bv1.size() - 2);
        std::string bv2_ = bv2.substr(2, bv2.size() - 2);
        if(bv1_.size() != bv2_.size()){
            // add prefix 0 to the shorter one
            if(bv1_.size() < bv2_.size()){
                bv1_ = "#b" + std::string(bv2_.size() - bv1_.size(), '0') + bv1_;
                bv2_ = "#b" + bv2_;
            }
            else{
                bv2_ = "#b" + std::string(bv1_.size() - bv2_.size(), '0') + bv2_;
                bv1_ = "#b" + bv1_;
            }
        }
        else{
            bv1_ = "#b" + bv1_;
            bv2_ = "#b" + bv2_;
        }
        std::string res = "";
        bool borrow = false;
        for(size_t i = bv1_.size() - 1; i >= 2; i--){
            if(bv1_[i] == '0' && bv2_[i] == '0'){
                res += borrow ? '1' : '0';
                borrow = false;
            }
            else if(bv1_[i] == '1' && bv2_[i] == '1'){
                res += borrow ? '0' : '1';
                borrow = true;
            }
            else{
                res += borrow ? '1' : '0';
            }
        }
        res = std::string(res.rbegin(), res.rend());
        res = "#b" + res;
        return res;
    }
    std::string BitVectorUtils::bvMul(const std::string& bv1, const std::string& bv2) {
        condAssert(bv1.rfind("#b", 0) == 0, "BitVectorUtils::bvMul: invalid bitvector");
        condAssert(bv2.rfind("#b", 0) == 0, "BitVectorUtils::bvMul: invalid bitvector");
    
        std::string bin1 = bv1.substr(2);
        std::string bin2 = bv2.substr(2);
    
        // Padding to same length
        size_t N = std::max(bin1.size(), bin2.size());
        if (bin1.size() < N) bin1 = std::string(N - bin1.size(), '0') + bin1;
        if (bin2.size() < N) bin2 = std::string(N - bin2.size(), '0') + bin2;
    
        // Init result as 0
        std::vector<int> result(N * 2, 0);
    
        // Manual binary multiplication (like pen-and-paper)
        for (size_t i = N; i > 0; --i) {
            if (bin2[i - 1] == '1') {
                for (size_t j = N; j > 0; --j) {
                    if (bin1[j - 1] == '1') {
                        result[i - 1 + j - 1 + 1] += 1;
                    }
                }
            }
        }
    
        // Carry handling
        for (size_t k = result.size() - 1; k > 0; --k) {
            if (result[k] >= 2) {
                result[k - 1] += result[k] / 2;
                result[k] %= 2;
            }
        }
    
        // Convert to binary string
        std::string resBin;
        for (size_t i = result.size() - N; i < result.size(); ++i) {
            resBin += (result[i] ? '1' : '0');
        }
    
        return "#b" + resBin;
    }
    


    std::string BitVectorUtils::bvUdiv(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvUdiv: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvUdiv: invalid bitvector");

        // div 0, return all ones
        bool isBv2Zero = true;
        for(size_t i = 2; i < bv2.size(); i++){
            if(bv2[i] == '1'){
                isBv2Zero = false;
                break;
            }
        }
        if(isBv2Zero){
            return "#b" + std::string(bv1.size() - 2, '1');
        }

        std::string bv1_ = bv1.substr(2, bv1.size() - 2);
        std::string bv2_ = bv2.substr(2, bv2.size() - 2);
        if(bv1_.size() != bv2_.size()){
            // add prefix 0 to the shorter one
            if(bv1_.size() < bv2_.size()){
                bv1_ = "#b" + std::string(bv2_.size() - bv1_.size(), '0') + bv1_;
                bv2_ = "#b" + bv2_;
            }
            else{
                bv2_ = "#b" + std::string(bv1_.size() - bv2_.size(), '0') + bv2_;
                bv1_ = "#b" + bv1_;
            }
        }
        else{
            bv1_ = "#b" + bv1_;
            bv2_ = "#b" + bv2_;
        }
        // special case: divide by 0
        bool isZero = true;
        for(size_t i = 2; i < bv2_.size(); i++){
            if(bv2_[i] == '1'){
                isZero = false;
                break;
            }
        }
        if(isZero){
            // divide by 0, return all ones
            return "#b" + std::string(bv1.size() - 2, '1');
        }
        
        // extract pure binary bits (without #b prefix)
        std::string dividend_bits = bv1_.substr(2);
        std::string divisor_bits = bv2_.substr(2);
        
        std::string quotient_bits;
        std::string remainder = "";
        
        // long division
        for(char bit : dividend_bits){
            // add current bit to remainder
            remainder.push_back(bit);
            
            // try division
            if(remainder.length() < divisor_bits.length()){
                // remainder length not enough, add 0 to quotient
                quotient_bits.push_back('0');
            }
            else{
                // compare remainder with divisor (need to add #b prefix for comparison)
                std::string remainder_bv = "#b" + remainder;
                std::string divisor_bv = "#b" + divisor_bits;
                
                // binary string comparison
                bool geq = true;
                if(remainder.length() != divisor_bits.length()){
                    geq = remainder.length() > divisor_bits.length();
                }
                else{
                    for(size_t i = 0; i < remainder.length(); i++){
                        if(remainder[i] < divisor_bits[i]){
                            geq = false;
                            break;
                        }
                        else if(remainder[i] > divisor_bits[i]){
                            break;
                        }
                    }
                }
                
                if(geq){
                    // remainder greater than or equal to divisor, add 1 to quotient
                    quotient_bits.push_back('1');
                    
                    // subtract divisor from remainder
                    std::string diff = SOMTParser::BitVectorUtils::bvSub(remainder_bv, divisor_bv);
                    remainder = diff.substr(2); // remove #b prefix
                }
                else{
                    // remainder less than divisor, add 0 to quotient
                    quotient_bits.push_back('0');
                }
            }
        }
        
        // return result with prefix
        return "#b" + quotient_bits;
    }
    std::string BitVectorUtils::bvUrem(const std::string& bv1, const std::string& bv2){
        condAssert(bv1[0] == '#' && bv1[1] == 'b', "BitVectorUtils::bvUrem: invalid bitvector");
        condAssert(bv2[0] == '#' && bv2[1] == 'b', "BitVectorUtils::bvUrem: invalid bitvector");
        // div 0, return first operand
        bool isZero = true;
        for(size_t i = 2; i < bv2.size(); i++){
            if(bv2[i] == '1'){
                isZero = false;
                break;
            }
        }
        if(isZero){
            return bv1;
        }
        std::string dividend = bv1;
        std::string divisor = bv2;
        std::string quotient = SOMTParser::BitVectorUtils::bvUdiv(bv1, bv2);
        std::string res = SOMTParser::BitVectorUtils::bvSub(dividend, SOMTParser::BitVectorUtils::bvMul(quotient, bv2));
        return res;
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


    std::string BitVectorUtils::bvShl(const std::string& bv, const std::string& n){
        // left shift
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvShl: invalid bitvector");
        condAssert(n[0] == '#' && n[1] == 'b', "BitVectorUtils::bvShl: invalid bitvector");
        size_t shift = Integer(n.substr(2, n.size() - 2)).toULong();
        if(shift >= bv.size() - 2){
            return "#b0" + std::string(shift - bv.size() + 2, '0');
        }
        else{
            return "#b" + bv.substr(2, bv.size() - 2 - shift) + std::string(shift, '0');
        }
    }
    std::string BitVectorUtils::bvLshr(const std::string& bv, const std::string& n){
        // logical right shift
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvLshr: invalid bitvector");
        condAssert(n[0] == '#' && n[1] == 'b', "BitVectorUtils::bvLshr: invalid bitvector");
        size_t shift = Integer(n.substr(2, n.size() - 2)).toULong();
        if(shift >= bv.size() - 2){
            return "#b0" + std::string(shift - bv.size() + 2, '0');
        }
        else{
            return "#b" + std::string(shift, '0') + bv.substr(2, bv.size() - 2 - shift);
        }
    }
    std::string BitVectorUtils::bvAshr(const std::string& bv, const std::string& n){
        // arithmetic right shift
        condAssert(bv[0] == '#' && bv[1] == 'b', "BitVectorUtils::bvAshr: invalid bitvector");
        condAssert(n[0] == '#' && n[1] == 'b', "BitVectorUtils::bvAshr: invalid bitvector");
        size_t shift = Integer(n.substr(2, n.size() - 2)).toULong();
        if(shift >= bv.size() - 2){
            return "#b" + std::string(bv.size() - 2, bv[2]);
        }
        else{
            return "#b" + std::string(shift, bv[2]) + bv.substr(2, bv.size() - 2 - shift);
        }
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
            switch(c) {
                case '\n':   // newline
                    result += "\\n";
                    break;
                case '\t':   // tab
                    result += "\\t";
                    break;
                case '\r':   // carriage return
                    result += "\\r";
                    break;
                case '\\':   // backslash
                    result += "\\\\";
                    break;
                case '"':    // double quote
                    result += "\\\"";
                    break;
                case '\'':   // single quote
                    result += "\\'";
                    break;
                case '\0':   // null character
                    result += "\\0";
                    break;
                case '\a':   // alert
                    result += "\\a";
                    break;
                case '\b':   // backspace
                    result += "\\b";
                    break;
                case '\f':   // form feed
                    result += "\\f";
                    break;
                case '\v':   // vertical tab
                    result += "\\v";
                    break;
                default:
                    // for non-printable characters, use hexadecimal escape
                    if(c < 32 || c > 126) {
                        std::ostringstream oss;
                        oss << "\\x" << std::hex << std::setfill('0') << std::setw(2) << (unsigned char)c;
                        result += oss.str();
                    } else {
                        result += c;
                    }
                    break;
            }
        }
        return result;
    }

    std::string ConversionUtils::unescapeString(const std::string& s){
        std::string result = "";
        size_t i = 0;
        while(i < s.length()) {
            if(s[i] == '\\' && i + 1 < s.length()) {
                switch(s[i + 1]) {
                    case 'n':    // newline
                        result += '\n';
                        break;
                    case 't':    // tab
                        result += '\t';
                        break;
                    case 'r':    // carriage return
                        result += '\r';
                        break;
                    case '\\':   // backslash
                        result += '\\';
                        break;
                    case '"':    // double quote
                        result += '"';
                        break;
                    case '\'':   // single quote
                        result += '\'';
                        break;
                    case '0':    // null character
                        result += '\0';
                        break;
                    case 'a':    // alert
                        result += '\a';
                        break;
                    case 'b':    // backspace
                        result += '\b';
                        break;
                    case 'f':    // form feed
                        result += '\f';
                        break;
                    case 'v':    // vertical tab
                        result += '\v';
                        break;
                    case 'x':    // hexadecimal escape \xHH
                        if(i + 3 < s.length()) {
                            std::string hexStr = s.substr(i + 2, 2);
                            try {
                                unsigned char value = static_cast<unsigned char>(std::stoi(hexStr, nullptr, 16));
                                result += value;
                                i += 2; // skip two hexadecimal characters
                            } catch(...) {
                                // if hexadecimal parsing fails, keep the original character
                                result += s[i];
                                i--; // back one character, because i+=2 later
                            }
                        } else {
                            // incomplete hexadecimal escape, keep the original character
                            result += s[i];
                            i--; // back one character, because i+=2 later
                        }
                        break;
                    default:
                        // if not a known escape character, keep the backslash and character
                        result += s[i];
                        result += s[i + 1];
                        break;
                }
                i += 2; // skip the escape character and the next character
            } else {
                result += s[i]; // if the current character is not an escape character, add it directly
                i++;
            }
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
    // FloatingPointUtils — bit-level helpers
    // ═══════════════════════════════════════════════════════════════════════

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
