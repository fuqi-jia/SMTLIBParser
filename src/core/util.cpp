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
#include <vector>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <string>
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
}
