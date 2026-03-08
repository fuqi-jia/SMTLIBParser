/* -*- Header -*-
 *
 * The Interval Header
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

#ifndef INTERVAL_HEADER
#define INTERVAL_HEADER

#include "somtparser/ir/number.h"
#include "somtparser/core/kind.h"
#include <vector>

namespace SOMTParser {

class Interval {
    private:
        Number lower;
        Number upper;
        bool leftClosed;
        bool rightClosed;
    public:
        Interval(Number lower = Number::zero(), Number upper = Number::zero(), bool leftClosed = true, bool rightClosed = true);
        
        Interval(const Interval& other);
        Interval& operator=(const Interval& other);
        ~Interval();

        // setters: 
        // warning: these functions will change the interval
        void setLower(const Number& lower);
        void setUpper(const Number& upper);
        void setLeftClosed(bool leftClosed);
        void setRightClosed(bool rightClosed);

        // getters
        Number getLower() const;
        Number getUpper() const;
        bool isLeftClosed() const;
        bool isRightClosed() const;
        bool isLeftUnbounded() const;
        bool isRightUnbounded() const;
        Number midpoint() const;
        std::string toString() const;
        void getIntegers(std::vector<Number>& integers) const;
        bool isPoint() const;
        bool isEmpty() const;
        Number width() const;
        bool contains(const Number& value) const;
        bool isSubsetOf(const Interval& other) const;
        bool isSubsetEqOf(const Interval& other) const;
        bool isSupersetOf(const Interval& other) const;
        bool isDisjointFrom(const Interval& other) const;
        bool isIntersectingWith(const Interval& other) const;
        Interval intersection(const Interval& other) const;
        Interval unionWith(const Interval& other) const;
        std::vector<Interval> difference(const Interval& other) const;
        size_t getIntervalIntCount() const;

        // unary operators
        Interval operator++() const;
        Interval operator--() const;
        Interval operator-() const;
        Interval operator+() const;
        Interval operator~() const;
        Interval operator!() const;
        Interval operator++(int) const;
        Interval operator--(int) const;
        Interval negate() const;
        Interval abs() const;
        Interval lb() const;
        Interval ln() const;
        Interval lg() const;
        Interval pow2() const;
        Interval exp() const;
        Interval sqrt() const;
        Interval safeSqrt() const;
        Interval sin() const;
        Interval cos() const;
        Interval tan() const;
        Interval cot() const;
        Interval sec() const;
        Interval csc() const;
        Interval asin() const;
        Interval acos() const;
        Interval atan() const;
        Interval acot() const;
        Interval asec() const;
        Interval acsc() const;
        Interval sinh() const;
        Interval cosh() const;
        Interval tanh() const;
        Interval coth() const;
        Interval sech() const;
        Interval csch() const;
        Interval asinh() const;
        Interval acosh() const;
        Interval atanh() const;
        Interval acoth() const;
        Interval asech() const;
        Interval acsch() const;
        
        // binary operators
        Interval operator+(const Number& value) const;
        Interval operator-(const Number& value) const;
        Interval operator*(const Number& value) const;
        Interval operator/(const Number& value) const;
        Interval operator%(const Number& value) const;
        Interval operator^(const Number& value) const;
        Interval operator+(const Interval& other) const;
        Interval operator-(const Interval& other) const;
        Interval operator*(const Interval& other) const;
        Interval operator/(const Interval& other) const;
        Interval operator%(const Interval& other) const;
        Interval operator^(const Interval& other) const;
        Interval add(const Number& value) const;
        Interval add(const Interval& other) const;
        Interval sub(const Number& value) const;
        Interval sub(const Interval& other) const;
        Interval mul(const Number& value) const;
        Interval mul(const Interval& other) const;
        Interval divReal(const Number& value) const;
        Interval divReal(const Interval& other) const;
        Interval divInt(const Number& value) const;
        Interval divInt(const Interval& other) const;
        Interval mod(const Number& value) const;
        Interval mod(const Interval& other) const;
        Interval pow(const Number& exp) const;
        Interval pow(const Interval& exp) const;
        Interval atan2(const Number& x) const;
        Interval atan2(const Interval& x) const;
        Interval log(const Number& base) const;
        Interval log(const Interval& base) const;

        // advanced functions
        Interval operate(const NODE_KIND& kind) const;
        Interval operate(const NODE_KIND& kind, const Number& value) const;
        Interval operate(const NODE_KIND& kind, const Interval& other) const;

        // comparison operators
        bool operator==(const Interval& other) const;
        bool operator!=(const Interval& other) const;
        bool operator<(const Interval& other) const;
        bool operator<=(const Interval& other) const;
        bool operator>(const Interval& other) const;
        bool operator>=(const Interval& other) const;

        // assignment operators
        Interval& operator+=(const Number& value);
        Interval& operator-=(const Number& value);
        Interval& operator*=(const Number& value);
        Interval& operator/=(const Number& value);

        // static method, for handling multiple intervals
        static std::vector<Interval> unionMulti(const std::vector<Interval>& intervals);
};

inline Interval EmptyInterval = Interval(1, -1, false, false);
inline Interval FullInterval = Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);

} // namespace SOMTParser

#endif