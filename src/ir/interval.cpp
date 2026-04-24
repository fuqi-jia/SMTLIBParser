/* -*- Source -*-
 *
 * The Interval Source
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

#include "somtparser/ir/interval.h"
#include "somtparser/core/asserting.h"

namespace {
    bool compareIntervalByLower(const SOMTParser::Interval& a, const SOMTParser::Interval& b) {
        return a.getLower() < b.getLower() || (a.getLower() == b.getLower() && a.isLeftClosed() > b.isLeftClosed());
    }
    bool isNaNNumber(SOMTParser::Number x) { return x.isNaN(); }
} // anonymous namespace

namespace SOMTParser{

    Interval::Interval(Number lower, Number upper, bool leftClosed, bool rightClosed)
        : lower(lower), upper(upper), leftClosed(leftClosed), rightClosed(rightClosed){
    }

    Interval::Interval(const Interval& other)
        : lower(other.lower), upper(other.upper), leftClosed(other.leftClosed), rightClosed(other.rightClosed) {
    }

    Interval::~Interval() {}
    
    Interval& Interval::operator=(const Interval& other) {
        if(this != &other) {
            lower = other.lower;
            upper = other.upper;
            leftClosed = other.leftClosed;
            rightClosed = other.rightClosed;
        }
        return *this;
    }

    bool Interval::operator==(const Interval& other) const {
        return lower == other.lower && upper == other.upper && leftClosed == other.leftClosed && rightClosed == other.rightClosed;
    }

    bool Interval::operator!=(const Interval& other) const {
        return !(*this == other);
    }

    void Interval::setLower(const Number& lower) {
        if(lower > upper) {
            throw std::invalid_argument("Lower bound is greater than upper bound");
        }
        this->lower = lower;
    }

    void Interval::setUpper(const Number& upper) {
        if(upper < lower) {
            throw std::invalid_argument("Upper bound is less than lower bound");
        }
        this->upper = upper;
    }

    void Interval::setLeftClosed(bool leftClosed) {
        this->leftClosed = leftClosed;
    }

    void Interval::setRightClosed(bool rightClosed) {
        this->rightClosed = rightClosed;
    }

    bool Interval::isLeftUnbounded() const {
        return lower.isInfinity();
    }

    bool Interval::isRightUnbounded() const {
        return upper.isInfinity();
    }

    Number Interval::midpoint() const {
        return (lower + upper) / 2;
    }

    std::string Interval::toString() const {
        std::string result = "";
        if(leftClosed) {
            result += "[";
        } else {
            result += "(";
        }
        result += lower.toString() + ", " + upper.toString();
        if(rightClosed) {
            result += "]";
        } else {
            result += ")";
        }
        return result;
    }

    void Interval::getIntegers(std::vector<Number>& integers) const {
        if(lower.isInfinity() || upper.isInfinity()) {
            throw std::invalid_argument("Interval is unbounded");
        }
        for(Number i = lower.ceil(); i <= upper.floor(); i++) {
            integers.push_back(i);
        }
    }

    bool Interval::isPoint() const {
        return lower == upper;
    }

    bool Interval::isEmpty() const {
        return lower > upper || (lower == upper && (!leftClosed || !rightClosed));
    }

    Number Interval::width() const {
        if(isEmpty()) {
            // empty interval
            return Number(-1);
        }
        return upper - lower;
    }

    bool Interval::contains(const Number& value) const {
        if(isEmpty()) return false;
        bool lowerOk = leftClosed ? (lower <= value) : (lower < value);
        bool upperOk = rightClosed ? (value <= upper) : (value < upper);
        return lowerOk && upperOk;
    }

    bool Interval::isSubsetOf(const Interval& other) const {
        if(isEmpty()) return true;
        if(other.isEmpty()) return false;
        
        // check lower bound
        bool lowerOk = (lower > other.lower) || 
                       (lower == other.lower && (!leftClosed || other.leftClosed));
        
        // check upper bound  
        bool upperOk = (upper < other.upper) || 
                       (upper == other.upper && (!rightClosed || other.rightClosed));
        
        return lowerOk && upperOk;
    }

    bool Interval::isSupersetOf(const Interval& other) const {
        if(isEmpty()) {
            return false;
        }
        // superset requirement:
        // 1. A.lower <= B.lower, if A.lower == B.lower, then A.leftClosed >= B.leftClosed
        // 2. A.upper >= B.upper, if A.upper == B.upper, then A.rightClosed >= B.rightClosed
        bool lowerOk = (lower < other.lower) || 
                       (lower == other.lower && (leftClosed || !other.leftClosed));
        bool upperOk = (upper > other.upper) || 
                       (upper == other.upper && (rightClosed || !other.rightClosed));
        return lowerOk && upperOk;
    }

    bool Interval::isDisjointFrom(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return false;
        }
        // disjoint requirement:
        // 1. A.upper < B.lower, or 
        // 2. A.upper = B.lower, but at least one is open
        // 3. B.upper < A.lower, or
        // 4. B.upper = A.lower, but at least one is open
        return upper < other.lower || 
               (upper == other.lower && (!rightClosed || !other.leftClosed)) ||
               lower > other.upper || 
               (lower == other.upper && (!leftClosed || !other.rightClosed));
    }

    bool Interval::isIntersectingWith(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return false;
        }
        // intersecting requirement: not disjoint
        return !isDisjointFrom(other);
    }

    Interval Interval::intersection(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        Number low = std::max(lower, other.lower);
        Number high = std::min(upper, other.upper);
        bool newLeftClosed = (lower < other.lower) ? other.leftClosed : 
                             (lower > other.lower) ? leftClosed : 
                             leftClosed && other.leftClosed;
        bool newRightClosed = (upper > other.upper) ? other.rightClosed : 
                              (upper < other.upper) ? rightClosed : 
                              rightClosed && other.rightClosed;
        return Interval(low, high, newLeftClosed, newRightClosed);
    }

    Interval Interval::unionWith(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        if(isDisjointFrom(other)) {
            throw std::invalid_argument("Intervals are disjoint");
        }
        
        // take the smaller lower bound, if equal, take the or of the left closedness
        Number newLower = std::min(lower, other.lower);
        bool newLeftClosed = (lower < other.lower) ? leftClosed : 
                            (other.lower < lower) ? other.leftClosed :
                            (leftClosed || other.leftClosed);
        
        // take the larger upper bound, if equal, take the or of the right closedness
        Number newUpper = std::max(upper, other.upper);
        bool newRightClosed = (upper > other.upper) ? rightClosed : 
                             (other.upper > upper) ? other.rightClosed :
                             (rightClosed || other.rightClosed);
        
        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    bool Interval::isSubsetEqOf(const Interval& other) const {
        if(isEmpty()) {
            return true;
        }
        // subset requirement: all elements in A are in B
        return lower >= other.lower && upper <= other.upper &&
               (other.leftClosed || lower > other.lower || !leftClosed) &&
               (other.rightClosed || upper < other.upper || !rightClosed);
    }

    size_t Interval::getIntervalIntCount() const {
        if(isEmpty()) return 0;
        if(isPoint() && lower.isInteger()) return 1;
        
        if(leftClosed && rightClosed) {
            return static_cast<size_t>((upper.floor() - lower.ceil() + Number(1)).toInteger().toULong());
        }
        else if(leftClosed && !rightClosed) {
            return static_cast<size_t>((upper.floor() - lower.ceil()).toInteger().toULong());
        }
        else if(!leftClosed && rightClosed) {
            return static_cast<size_t>((upper.floor() - lower.ceil()).toInteger().toULong());
        }
        else {
            // open interval
            return static_cast<size_t>((upper.floor() - lower.ceil() - Number(1)).toInteger().toULong());
        }
    }

    std::vector<Interval> Interval::difference(const Interval& other) const {
        // if the intervals are disjoint, return A
        if(isDisjointFrom(other)) {
            return {*this};
        }
        
        // if B is completely contained in A, return empty set
        if(isSupersetOf(other)) {
            return {};
        }
        
        std::vector<Interval> result;
        
        // if B is inside A, split A into two intervals
        if(isSubsetOf(other)) {
            
            // left interval: [a.low, b.low)
            bool rightClosed = !other.leftClosed;
            result.push_back(Interval(lower, other.lower, leftClosed, rightClosed));
            
            // right interval: (b.high, a.high]
            bool leftClosed = !other.rightClosed;
            result.push_back(Interval(other.upper, upper, leftClosed, rightClosed));
            
            return result;
        }
        
        // if B covers part of A from the left
        if((other.lower <= lower || (other.lower == lower && other.leftClosed >= leftClosed)) &&
           (other.upper < upper || (other.upper == upper && !other.rightClosed && rightClosed))) {
            
            // right remaining part: (b.high, a.high]
            bool leftClosed = !other.rightClosed;
            result.push_back(Interval(other.upper, upper, leftClosed, rightClosed));
            
            return result;
        }
        
        // if B covers part of A from the right
        if((other.lower > lower || (other.lower == lower && !other.leftClosed && leftClosed)) &&
           (other.upper >= upper || (other.upper == upper && other.rightClosed >= rightClosed))) {
            
            // left remaining part: [a.low, b.low)
            bool rightClosed = !other.leftClosed;
            result.push_back(Interval(lower, other.lower, leftClosed, rightClosed));
            
            return result;
        }
        
        // theoretically should not reach here, but return empty set for safety
        return {};
    }

    std::vector<Interval> Interval::unionMulti(const std::vector<Interval>& intervals) {
        if(intervals.empty()) return {};
        if(intervals.size() == 1) return {intervals[0]};
        
        // sort the intervals by the lower bound
        std::vector<Interval> sortedIntervals = intervals;
        std::sort(sortedIntervals.begin(), sortedIntervals.end(), compareIntervalByLower);
        
        std::vector<Interval> result;
        Interval current = sortedIntervals[0];
        
        for(size_t i = 1; i < sortedIntervals.size(); ++i) {
            // check if the current interval intersects with the next interval
            bool intersectOrAdjacent = !current.isDisjointFrom(sortedIntervals[i]);
            
            if(intersectOrAdjacent) {
                // if they intersect or are adjacent, merge the intervals
                current = Interval(
                    std::min(current.lower, sortedIntervals[i].lower),
                    std::max(current.upper, sortedIntervals[i].upper),
                    (current.lower < sortedIntervals[i].lower) ? current.leftClosed : 
                    (sortedIntervals[i].lower < current.lower) ? sortedIntervals[i].leftClosed :
                    (current.leftClosed || sortedIntervals[i].leftClosed),
                    (current.upper > sortedIntervals[i].upper) ? current.rightClosed : 
                    (sortedIntervals[i].upper > current.upper) ? sortedIntervals[i].rightClosed :
                    (current.rightClosed || sortedIntervals[i].rightClosed)
                );
            } else {
                // if they are disjoint, add the current interval to the result, and update the current interval to the next interval
                result.push_back(current);
                current = sortedIntervals[i];
            }
        }
        
        // add the last interval
        result.push_back(current);
        
        return result;
    }

    
    Interval Interval::operator++() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower + 1, upper + 1, leftClosed, rightClosed);
    }
    Interval Interval::operator--() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower - 1, upper - 1, leftClosed, rightClosed);
    }
    Interval Interval::operator-() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(-upper, -lower, rightClosed, leftClosed);
    }
    Interval Interval::operator+() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return *this;
    }
    Interval Interval::operator~() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(-upper, -lower, rightClosed, leftClosed);
    }
    Interval Interval::operator!() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(-upper, -lower, rightClosed, leftClosed);
    }
    Interval Interval::operator++(int) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower + 1, upper + 1, leftClosed, rightClosed);
    }
    Interval Interval::operator--(int) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower - 1, upper - 1, leftClosed, rightClosed);
    }
    Interval Interval::negate() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        return Interval(-upper, -lower, rightClosed, leftClosed);
    }
    Interval Interval::abs() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(lower >= 0) {
            // [a,b] with a ≥ 0 -> [a,b]
            return *this;
        } else if(upper <= 0) {
            // [a,b] with b ≤ 0 -> [-b,-a]
            return Interval(-upper, -lower, rightClosed, leftClosed);
        } else {
            // [a,b] with a < 0 < b -> [0,max(|a|,|b|)]
            Number maxAbs = std::max(lower.abs(), upper.abs());
            return Interval(Number(0), maxAbs, true, (maxAbs == lower.abs()) ? leftClosed : rightClosed);
        }
    }
    Interval Interval::lb() const {
        // log base 2
        if(isEmpty()) {
            return EmptyInterval;
        }

        // lb(x) is defined for x > 0
        if(upper <= 0) {
            return EmptyInterval;
        }

        Number new_lower = 0;
        Number new_upper = 0;
        bool new_leftClosed = true;
        bool new_rightClosed = true;
        if(upper <= 0){
            return EmptyInterval;
        }
        
        // for lower bound
        if(lower <= 0){
            new_lower = Number::negativeInfinity();
            new_leftClosed = false;
        }
        else if(lower.isPositiveInfinity()){
            new_lower = Number::positiveInfinity();
            new_leftClosed = false;
        }
        else{
            new_lower = lower.lb();
            new_lower = new_lower.nextBelow();
            new_leftClosed = false;
        }

        // for upper bound
        if(upper.isPositiveInfinity()){
            new_upper = Number::positiveInfinity();
            new_rightClosed = false;
        }
        else{
            new_upper = upper.lb();
            new_upper = new_upper.nextAbove();
            new_rightClosed = false;
        }
        
        return Interval(new_lower, new_upper, new_leftClosed, new_rightClosed);
    }
    Interval Interval::ln() const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        // ln(x) is defined for x > 0
        if(upper <= 0) {
            return EmptyInterval;
        }

        Number new_lower = 0;
        Number new_upper = 0;
        bool new_leftClosed = true;
        bool new_rightClosed = true;

        // for lower bound
        if(lower <= 0){
            new_lower = Number::negativeInfinity();
            new_leftClosed = false;
        }
        else if(lower.isPositiveInfinity()){
            new_lower = Number::positiveInfinity();
            new_leftClosed = false;
        }
        else{
            new_lower = lower.ln();
            new_lower = new_lower.nextBelow();
            new_leftClosed = false;
        }

        // for upper bound
        if(upper.isPositiveInfinity()){
            new_upper = Number::positiveInfinity();
            new_rightClosed = false;
        }
        else{
            new_upper = upper.ln();
            new_upper = new_upper.nextAbove();
            new_rightClosed = false;
        }

        return Interval(new_lower, new_upper, new_leftClosed, new_rightClosed);
    }
    Interval Interval::lg() const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        // lg(x) is defined for x > 0
        if(upper <= 0) {
            return EmptyInterval;
        }

        Number new_lower = 0;
        Number new_upper = 0;
        bool new_leftClosed = true;
        bool new_rightClosed = true;
        
        // for lower bound
        if(lower <= 0){
            new_lower = Number::negativeInfinity();
            new_leftClosed = false;
        }
        else if(lower.isPositiveInfinity()){
            new_lower = Number::positiveInfinity();
            new_leftClosed = false;
        }
        else{
            new_lower = lower.lg();
            new_lower = new_lower.nextBelow();
            new_leftClosed = false;
        }

        // for upper bound
        if(upper.isPositiveInfinity()){
            new_upper = Number::positiveInfinity();
            new_rightClosed = false;
        }
        else{
            new_upper = upper.lg();
            new_upper = new_upper.nextAbove();
            new_rightClosed = false;
        }

        return Interval(new_lower, new_upper, new_leftClosed, new_rightClosed);
    }
    Interval Interval::log(const Number& base) const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        // log(x, base) = ln(x) / ln(base)
        // base must be > 0 and != 1
        if(base <= 0) {
            throw std::domain_error("Logarithm base must be positive");
        }
        if(base == 1) {
            throw std::domain_error("Logarithm base cannot be 1");
        }

        // ln(x) is defined for x > 0
        if(upper <= 0) {
            return EmptyInterval;
        }

        // compute ln(x) first
        Interval lnInterval = this->ln();
        
        // compute ln(base)
        Number lnBase = base.ln();
        
        // if ln(base) == 0, which means base == 1, but we already checked this
        // divide ln(x) by ln(base)
        if(lnBase > 0) {
            return Interval(lnInterval.getLower() / lnBase, lnInterval.getUpper() / lnBase, 
                           lnInterval.isLeftClosed(), lnInterval.isRightClosed());
        } else {
            // ln(base) < 0, which means base < 1, so we need to reverse the interval
            return Interval(lnInterval.getUpper() / lnBase, lnInterval.getLower() / lnBase, 
                           lnInterval.isRightClosed(), lnInterval.isLeftClosed());
        }
    }
    Interval Interval::log(const Interval& base) const {
        if(isEmpty() || base.isEmpty()) {
            return EmptyInterval;
        }

        // log(x, base) = ln(x) / ln(base)
        // base must be > 0 and != 1
        
        // check if base contains invalid values
        if(base.getUpper() <= 0) {
            throw std::domain_error("Logarithm base must be positive");
        }
        if(base.contains(Number(1))) {
            throw std::domain_error("Logarithm base cannot be 1");
        }

        // ln(x) is defined for x > 0
        if(upper <= 0) {
            return EmptyInterval;
        }

        // compute ln(x) first
        Interval lnInterval = this->ln();
        
        // compute ln(base)
        Interval lnBaseInterval = base.ln();
        
        // if ln(base) interval contains 0, which means base contains 1, but we already checked this
        // divide ln(x) by ln(base) using interval division
        return lnInterval.divReal(lnBaseInterval);
    }
    Interval Interval::exp() const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        Number newLower = lower;
        Number newUpper = upper;
        bool newLeftClosed = false;
        bool newRightClosed = false;
        if(lower.isNegativeInfinity()) {
            // exp(-inf) = 0
            newLower = Number(0);
        }
        else if(lower.isPositiveInfinity()) {
            newLower = Number::positiveInfinity();
        }
        else{
            newLower = lower.exp();
            newLower = newLower.nextBelow();
        }

        if(upper.isNegativeInfinity()) {
            newUpper = Number(0);
        }
        else if(upper.isPositiveInfinity()) {
            newUpper = Number::positiveInfinity();
        }
        else{
            newUpper = upper.exp();
            newUpper = newUpper.nextAbove();
        }

        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::sqrt() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(upper <= 0) {
            return EmptyInterval;
        }
        Number newLower = lower;
        Number newUpper = upper;
        bool newLeftClosed = leftClosed;
        bool newRightClosed = rightClosed;
        if(lower <= 0) {
            newLower = Number(0);
            newLeftClosed = true;
        }
        else if(lower.isPositiveInfinity()) {
            newLower = Number::positiveInfinity();
            newLeftClosed = false;
        }
        else{
            newLower = lower.sqrt();
            newLower = newLower.nextBelow();
            newLeftClosed = false;
        }

        if(upper <= 0) {
            newUpper = Number(0);
            newRightClosed = true;
        }
        else if(upper.isPositiveInfinity()) {
            newUpper = Number::positiveInfinity();
            newRightClosed = false;
        }
        else{
            newUpper = upper.sqrt();
            newUpper = newUpper.nextAbove();
            newRightClosed = false;
        }

        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::safeSqrt() const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        if(upper <= 0) {
            return EmptyInterval;
        }
        Number newLower = lower;
        Number newUpper = upper;
        bool newLeftClosed = leftClosed;
        bool newRightClosed = rightClosed;
        if(lower <= 0) {
            newLower = Number(0);
            newLeftClosed = true;
        }
        else if(lower.isPositiveInfinity()) {
            newLower = Number::positiveInfinity();
            newLeftClosed = false;
        }
        else{
            newLower = lower.safeSqrt();
            newLower = newLower.nextBelow();
            newLeftClosed = false;
        }

        if(upper <= 0) {
            newUpper = Number(0);
            newRightClosed = true;
        }
        else if(upper.isPositiveInfinity()) {
            newUpper = Number::positiveInfinity();
            newRightClosed = false;
        }
        else{
            newUpper = upper.safeSqrt();
            newUpper = newUpper.nextAbove();
            newRightClosed = false;
        }

        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::sin() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // define the period
        const Number TWO_PI = Number(2) * Number::pi();

        // if the interval contains infinity, the result is undefined
        if(lower.isInfinity() || upper.isInfinity()) {
            return Interval(Number(-1), Number(1), true, true);
        }
        
        // check if the interval covers the whole period
        if((upper - lower) >= TWO_PI) {
            return Interval(Number(-1), Number(1), true, true); // full range [-1, 1]
        }
        
        // normalize to [0, 2π)
        Number factor = (lower / TWO_PI).floor();
        Number lowNorm = lower - factor * TWO_PI;
        if(lowNorm < Number(0)) lowNorm += TWO_PI;
        
        factor = (upper / TWO_PI).floor();
        Number highNorm = upper - factor * TWO_PI;
        if(highNorm < Number(0)) highNorm += TWO_PI;
        
        // if the interval crosses the period boundary
        if(highNorm < lowNorm) {
            highNorm += TWO_PI;
        }
        
        // calculate the endpoint values
        Number sinLow = lowNorm.sin();
        Number sinHigh = highNorm.sin();
        
        Number minVal = std::min(sinLow, sinHigh);
        Number maxVal = std::max(sinLow, sinHigh);
        bool newLeftClosed = sinLow < sinHigh ? leftClosed : rightClosed;
        bool newRightClosed = sinLow < sinHigh ? rightClosed : leftClosed;
        
        // check if the interval passes the extreme points π/2 or 3π/2
        Number PI_HALF = Number::pi() / Number(2);
        Number THREE_PI_HALF = Number(3) * Number::pi() / Number(2);
        
        // check if the interval contains π/2 (sine maximum is 1)
        if((lowNorm <= PI_HALF && highNorm >= PI_HALF) ||
           (lowNorm <= PI_HALF + TWO_PI && highNorm >= PI_HALF + TWO_PI)) {
            maxVal = Number(1);
            newLeftClosed = true;
        }
        
        // check if the interval contains 3π/2 (sine minimum is -1)
        if((lowNorm <= THREE_PI_HALF && highNorm >= THREE_PI_HALF) ||
           (lowNorm <= THREE_PI_HALF + TWO_PI && highNorm >= THREE_PI_HALF + TWO_PI)) {
            minVal = Number(-1);
            newRightClosed = true;
        }
        
        if(minVal > Number(-1)) {
            // extend a small value
            minVal = minVal.nextBelow();
            newLeftClosed = false;
        }
        if(maxVal < Number(1)) {
            // extend a small value
            maxVal = maxVal.nextAbove();
            newRightClosed = false;
        }
        
        return Interval(minVal, maxVal, newLeftClosed, newRightClosed);
    }

    Interval Interval::cos() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // define the period
        const Number TWO_PI = Number(2) * Number::pi();
        
        // if the interval contains infinity, the result is undefined
        if(lower.isInfinity() || upper.isInfinity()) {
            return Interval(Number(-1), Number(1), true, true);
        }
        
        // check if the interval covers the whole period
        if((upper - lower) >= TWO_PI) {
            return Interval(Number(-1), Number(1), true, true); // full range [-1, 1]
        }
        
        // normalize to [0, 2π)
        Number factor = (lower / TWO_PI).floor();
        Number lowNorm = lower - factor * TWO_PI;
        if(lowNorm < Number(0)) lowNorm += TWO_PI;
        
        factor = (upper / TWO_PI).floor();
        Number highNorm = upper - factor * TWO_PI;
        if(highNorm < Number(0)) highNorm += TWO_PI;

        // if the interval crosses the period boundary
        if(highNorm < lowNorm) {
            highNorm += TWO_PI;
        }
        
        // calculate the endpoint values
        Number cosLow = lowNorm.cos();
        Number cosHigh = highNorm.cos();
        
        Number minVal = std::min(cosLow, cosHigh);
        Number maxVal = std::max(cosLow, cosHigh);
        
        bool newLeftClosed = cosLow < cosHigh ? leftClosed : rightClosed;
        bool newRightClosed = cosLow < cosHigh ? rightClosed : leftClosed;
        
        // check if the interval passes the extreme points 0 or π
        Number ZERO = Number(0);
        Number PI = Number::pi();
        
        // check if the interval contains 0 (cosine maximum is 1)
        if((lowNorm <= ZERO && highNorm >= ZERO) ||
           (lowNorm <= ZERO + TWO_PI && highNorm >= ZERO + TWO_PI)) {
            maxVal = Number(1);
            newLeftClosed = true;
        }
        
        // check if the interval contains π (cosine minimum is -1)
        if((lowNorm <= PI && highNorm >= PI) ||
           (lowNorm <= PI + TWO_PI && highNorm >= PI + TWO_PI)) {
            minVal = Number(-1);
            newRightClosed = true;
        }
        
        if(minVal > Number(-1)) {
            // extend a small value
            minVal = minVal.nextBelow();
            newLeftClosed = false;
        }
        if(maxVal < Number(1)) {
            // extend a small value
            maxVal = maxVal.nextAbove();
            newRightClosed = false;
        }
        
        return Interval(minVal, maxVal, newLeftClosed, newRightClosed);
    }

    Interval Interval::tan() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // define the constants
        const Number PI_HALF = Number::pi() / Number(2);
        const Number PI = Number::pi();
        
        // if the interval contains infinity, the result is undefined
        if(lower.isInfinity() || upper.isInfinity()) {
            return FullInterval;
        }
        
        // normalize to [-π/2, π/2)
        Number factor = (lower / PI).floor();
        Number lowMod = lower - factor * PI;
        if(lowMod < -PI_HALF) lowMod += PI;
        if(lowMod >= PI_HALF) lowMod -= PI;
        
        factor = (upper / PI).floor();
        Number highMod = upper - factor * PI;
        if(highMod < -PI_HALF) highMod += PI;
        if(highMod >= PI_HALF) highMod -= PI;
        
        // check if the interval contains odd multiples of π/2 (tangent singularities)
        if((lowMod <= -PI_HALF && highMod >= -PI_HALF) ||
           (lowMod <= PI_HALF && highMod >= PI_HALF)) {
            // return infinite interval (tangent is unbounded)
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // calculate the tangent values at the endpoints
        Number tanLow = lower.tan();
        Number tanHigh = upper.tan();

        bool newLeftClosed = leftClosed;
        bool newRightClosed = rightClosed;
        
        if(tanLow > tanHigh) {
            std::swap(tanLow, tanHigh);
            std::swap(newLeftClosed, newRightClosed);
        }
        // extend a small value
        tanLow = tanLow.nextBelow();
        tanHigh = tanHigh.nextAbove();
        newLeftClosed = false;
        newRightClosed = false;

        return Interval(tanLow, tanHigh, newLeftClosed, newRightClosed);
    }

    Interval Interval::cot() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // cotangent function is the reciprocal of tangent function
        Interval tanInterval = this->tan();
        
        // if the tan interval contains 0, the cotangent function has singularities
        if(tanInterval.contains(Number(0))) {
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // cotangent function cot(x) = 1/tan(x)
        Interval one = Interval(Number(1), Number(1), true, true);
        return one / tanInterval;
    }

    Interval Interval::sec() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // secant function is the reciprocal of cosine function
        Interval cosInterval = this->cos();
        
        // if the cos interval contains 0, the secant function has singularities
        if(cosInterval.contains(Number(0))) {
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // secant function sec(x) = 1/cos(x)
        Interval one = Interval(Number(1), Number(1), true, true);
        return one / cosInterval;
    }

    Interval Interval::csc() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // cosecant function is the reciprocal of sine function
        Interval sinInterval = this->sin();
        
        // if the sin interval contains 0, the cosecant function has singularities
        if(sinInterval.contains(Number(0))) {
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // cosecant function csc(x) = 1/sin(x)
        Interval one = Interval(Number(1), Number(1), true, true);
        return one / sinInterval;
    }

    Interval Interval::asin() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // asin function domain is [-1, 1]
        if(lower < Number(-1) || upper > Number(1)) {
            throw std::domain_error("Argument for asin must be in range [-1, 1]");
        }
        
        // asin function is monotonically increasing
        Number asinLow = lower.asin();
        Number asinHigh = upper.asin();

        // add a small value
        asinLow = asinLow.nextBelow();
        asinHigh = asinHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(asinLow, asinHigh, newLeftClosed, newRightClosed);
    }

    Interval Interval::acos() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acos function domain is [-1, 1]
        if(lower < Number(-1) || upper > Number(1)) {
            throw std::domain_error("Argument for acos must be in range [-1, 1]");
        }
        
        // acos function is monotonically decreasing
        Number acosLow = upper.acos();
        Number acosHigh = lower.acos();
        
        return Interval(acosLow, acosHigh, rightClosed, leftClosed);
    }

    Interval Interval::atan() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // atan function is monotonically increasing, domain is the whole real number set
        Number atanLow = lower.atan();
        Number atanHigh = upper.atan();

        // add a small value
        atanLow = atanLow.nextBelow();
        atanHigh = atanHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(atanLow, atanHigh, newLeftClosed, newRightClosed);
    }

    Interval Interval::acot() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acot function is monotonically decreasing, domain is the whole real number set
        // acot(x) = π/2 - atan(x)
        Number acotLow = Number::pi() / Number(2) - upper.atan();
        Number acotHigh = Number::pi() / Number(2) - lower.atan();
        
        // added a small value in atan, so no need to add a small value here
        return Interval(acotLow, acotHigh, false, false);
    }

    Interval Interval::asec() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // asec function domain is (-∞, -1] ∪ [1, ∞)
        if(lower > Number(-1) && upper < Number(1)) {
            throw std::domain_error("Argument for asec must be outside range (-1, 1)");
        }
        
        // handle special case: interval crosses -1 or 1
        if(lower <= Number(-1) && upper >= Number(1)) {
            // return possible entire value range
            return Interval(Number(0), Number::pi(), true, true);
        }
        
        // asec function is monotonically increasing
        // asec(x) = acos(1/x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval oneOver = one / *this;
        return oneOver.acos();
    }

    Interval Interval::acsc() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acsc function domain is (-∞, -1] ∪ [1, ∞)
        if(lower > Number(-1) && upper < Number(1)) {
            throw std::domain_error("Argument for acsc must be outside range (-1, 1)");
        }
        
        // handle special case: interval crosses -1 or 1
        if(lower <= Number(-1) && upper >= Number(1)) {
            // return possible entire value range
            return Interval(-Number::pi() / Number(2), Number::pi() / Number(2), true, true);
        }
        
        // acsc function is monotonically decreasing
        // acsc(x) = asin(1/x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval oneOver = one / *this;
        return oneOver.asin();
    }
    
    Interval Interval::sinh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // sinh function is monotonically increasing
        Number sinhLow = lower.sinh();
        Number sinhHigh = upper.sinh();

        // add a small value
        sinhLow = sinhLow.nextBelow();
        sinhHigh = sinhHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(sinhLow, sinhHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::cosh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // cosh function is a even function, the minimum value is 1 at x=0
        if(lower >= Number(0)) {
            // all the parameters are positive
            Number coshLow = lower.cosh();
            Number coshHigh = upper.cosh();
            bool newLeftClosed = false;
            bool newRightClosed = false;
            
            // add a small value
            coshLow = coshLow.nextBelow();
            coshHigh = coshHigh.nextAbove();
            
            // ensure the value is not less than 1 (the minimum value of cosh)
            if(coshLow < Number(1)) {
                coshLow = Number(1);
                newLeftClosed = true;
            }
            
            return Interval(coshLow, coshHigh, newLeftClosed, newRightClosed);
        } else if(upper <= Number(0)) {
            // all the parameters are negative
            Number coshLow = upper.cosh();  // note: since cosh is an even function, we swap the order here
            Number coshHigh = lower.cosh();
            bool newLeftClosed = false;
            bool newRightClosed = false;
            
            // add a small value
            coshLow = coshLow.nextBelow();
            coshHigh = coshHigh.nextAbove();
            
            // ensure the value is not less than 1 (the minimum value of cosh)
            if(coshLow < Number(1)) {
                coshLow = Number(1);
                newLeftClosed = true;
            }
            
            return Interval(coshLow, coshHigh, newLeftClosed, newRightClosed);
        } else {
            // the interval crosses zero, the minimum value is cosh(0) = 1
            Number coshLow = Number(1);
            Number coshHigh = std::max(lower.cosh(), upper.cosh());
            bool newLeftClosed = true;
            bool newRightClosed = false;
            
            // add a small value
            coshHigh = coshHigh.nextAbove();
            
            return Interval(coshLow, coshHigh, newLeftClosed, newRightClosed);
        }
    }
    
    Interval Interval::tanh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // tanh function is monotonically increasing, value range is (-1, 1)
        Number tanhLow = lower.tanh();
        Number tanhHigh = upper.tanh();

        // add a small value
        tanhLow = tanhLow.nextBelow();
        tanhHigh = tanhHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(tanhLow, tanhHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::coth() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // coth function has singularities at 0
        if(lower <= Number(0) && upper >= Number(0)) {
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // coth function coth(x) = 1/tanh(x)
        Number cothLow = Number(1) / upper.tanh();
        Number cothHigh = Number(1) / lower.tanh();
        
        // added a small value in tanh, so no need to add a small value here
        return Interval(cothLow, cothHigh, false, false);
    }
    
    Interval Interval::sech() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // sech function sech(x) = 1/cosh(x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval coshInterval = one / this->cosh();
        
        // sech function value range is (0, 1], maximum value is 1 at 0
        Number sechLow = coshInterval.getLower();
        Number sechHigh = coshInterval.getUpper();
        bool newLeftClosed = false;
        bool newRightClosed = false;

        if(coshInterval.getUpper() >= Number(1)){
            sechHigh = Number(1);
            newRightClosed = true;
        }

        if(coshInterval.getLower() <= Number(0)){
            sechLow = Number(0);
        }
        
        return Interval(sechLow, sechHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::csch() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // csch function has singularities at 0
        if(lower <= Number(0) && upper >= Number(0)) {
            return Interval(Number::negativeInfinity(), Number::positiveInfinity(), false, false);
        }
        
        // csch function csch(x) = 1/sinh(x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval sinhInterval = one / this->sinh();
        
        // csch function value range is (-∞, 0) ∪ (0, ∞)
        Number cschLow = sinhInterval.getLower();
        Number cschHigh = sinhInterval.getUpper();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(cschLow, cschHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::asinh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // asinh function is monotonically increasing, domain is the whole real number set
        Number asinhLow = lower.asinh();
        Number asinhHigh = upper.asinh();

        // add a small value
        asinhLow = asinhLow.nextBelow();
        asinhHigh = asinhHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(asinhLow, asinhHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::acosh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acosh function domain is [1, ∞)
        if(lower < Number(1)) {
            throw std::domain_error("Argument for acosh must be >= 1");
        }
        
        // acosh function is monotonically increasing
        Number acoshLow = lower.acosh();
        Number acoshHigh = upper.acosh();

        // add a small value
        acoshLow = acoshLow.nextBelow();
        acoshHigh = acoshHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(acoshLow, acoshHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::atanh() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // atanh function domain is (-1, 1)
        if(lower <= Number(-1) || upper >= Number(1)) {
            throw std::domain_error("Argument for atanh must be in range (-1, 1)");
        }
        
        // atanh function is monotonically increasing
        Number atanhLow = lower.atanh();
        Number atanhHigh = upper.atanh();

        // add a small value
        atanhLow = atanhLow.nextBelow();
        atanhHigh = atanhHigh.nextAbove();
        bool newLeftClosed = false;
        bool newRightClosed = false;
        
        return Interval(atanhLow, atanhHigh, newLeftClosed, newRightClosed);
    }
    
    Interval Interval::acoth() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acoth function domain is (-∞, -1) ∪ (1, ∞)
        if(lower >= Number(-1) && upper <= Number(1)) {
            throw std::domain_error("Argument for acoth must be outside range [-1, 1]");
        }
        
        // acoth function acoth(x) = atanh(1/x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval oneOver = one / *this;
        return oneOver.atanh();
    }
    
    Interval Interval::asech() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // asech function domain is (0, 1]
        if(lower <= Number(0) || upper > Number(1)) {
            throw std::domain_error("Argument for asech must be in range (0, 1]");
        }
        
        // asech function is monotonically decreasing
        // asech(x) = acosh(1/x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval oneOver = one / *this;
        return oneOver.acosh();
    }
    
    Interval Interval::acsch() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        // acsch function domain is R\{0}
        if(lower <= Number(0) && upper >= Number(0)) {
            throw std::domain_error("Argument for acsch cannot be 0");
        }
        
        // acsch function is monotonically decreasing
        // acsch(x) = asinh(1/x)
        Interval one = Interval(Number(1), Number(1), true, true);
        Interval oneOver = one / *this;
        return oneOver.asinh();
    }


    Interval Interval::operator+(const Number& value) const {
        return Interval(lower + value, upper + value, leftClosed, rightClosed);
    }

    Interval Interval::operator-(const Number& value) const {
        return Interval(lower - value, upper - value, leftClosed, rightClosed);
    }

    Interval Interval::operator*(const Number& value) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(value > 0) {
            return Interval(lower * value, upper * value, leftClosed, rightClosed);
        } 
        else if(value < 0) {
            return Interval(upper * value, lower * value, rightClosed, leftClosed);
        }
        else {
            if(leftClosed && rightClosed) {
                return Interval(0, 0, true, true);
            }
            else {
                return EmptyInterval;
            }
        }
    }
    Interval Interval::operator/(const Number& value) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(value == 0) {
            throw std::domain_error("Division by zero");
        }
        if(value > 0) {
            return Interval(lower / value, upper / value, leftClosed, rightClosed);
        } else {
            return Interval(upper / value, lower / value, rightClosed, leftClosed);
        }
    }

    Interval Interval::operator+(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower + other.lower, upper + other.upper, leftClosed && other.leftClosed, rightClosed && other.rightClosed);
    }

    Interval Interval::operator-(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        return Interval(lower - other.upper, upper - other.lower, leftClosed && other.rightClosed, rightClosed && other.leftClosed);
    }

    Interval Interval::operator*(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        
        // Special case: when multiplying an interval by itself (squaring),
        // and the interval crosses zero, the result should be [0, max(abs(lower), abs(upper))²)
        // because x² is always non-negative and reaches minimum 0 at x=0
        if (this == &other || (lower == other.lower && upper == other.upper && 
            leftClosed == other.leftClosed && rightClosed == other.rightClosed)) {
            // Squaring operation: x² where x is in this interval
            if (lower <= Number(0) && upper >= Number(0)) {
                // Interval crosses zero: x² is in [0, max(abs(lower), abs(upper))²)
                Number absLower = lower.abs();
                Number absUpper = upper.abs();
                Number maxAbs = (absLower > absUpper) ? absLower : absUpper;
                Number maxSquared = maxAbs * maxAbs;
                
                // Lower bound is always 0 (closed) because x² = 0 when x = 0
                // Upper bound: if the original interval is open at the boundary where max occurs,
                // the square is also open; otherwise closed
                bool upperClosed = false;
                if (absLower > absUpper) {
                    // Maximum occurs at lower boundary
                    upperClosed = leftClosed;
                } else {
                    // Maximum occurs at upper boundary
                    upperClosed = rightClosed;
                }
                
                return Interval(Number(0), maxSquared, true, upperClosed);
            }
        }
        
        Number p1 = lower * other.lower;
        Number p2 = lower * other.upper;
        Number p3 = upper * other.lower;
        Number p4 = upper * other.upper;
        
        Number newLower = std::min({p1, p2, p3, p4});
        Number newUpper = std::max({p1, p2, p3, p4});
        bool newLeftClosed = false;
        bool newRightClosed = false;
        if(p1 == newLower) {
            newLeftClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newLower) {
            newLeftClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newLower) {
            newLeftClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newLower) {
            newLeftClosed = rightClosed && other.rightClosed;
        }

        if(p1 == newUpper) {
            newRightClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newUpper) {
            newRightClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newUpper) {
            newRightClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newUpper) {
            newRightClosed = rightClosed && other.rightClosed;
        }
        
        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::operator/(const Interval& other) const {
        return this->divReal(other);
    }

    Interval Interval::add(const Number& value) const {
        return *this + value;
    }

    Interval Interval::add(const Interval& other) const {
        return *this + other;
    }

    Interval Interval::sub(const Number& value) const {
        return *this - value;
    }

    Interval Interval::sub(const Interval& other) const {
        return *this - other;
    }

    Interval Interval::mul(const Number& value) const {
        return *this * value;
    }

    Interval Interval::mul(const Interval& other) const {
        return *this * other;
    }

    Number Interval::getLower() const {
        return lower;
    }

    Number Interval::getUpper() const {
        return upper;
    }

    bool Interval::isLeftClosed() const {
        return leftClosed;
    }

    bool Interval::isRightClosed() const {
        return rightClosed;
    }

    Interval Interval::divReal(const Number& value) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(value == 0) {
            throw std::domain_error("Division by zero");
        }
        if(value > 0) {
            return Interval(lower / value, upper / value, leftClosed, rightClosed);
        } else {
            return Interval(upper / value, lower / value, rightClosed, leftClosed);
        }
    }

    Interval Interval::divReal(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        // if the divisor interval contains 0, the result is undefined
        if((other.lower < 0 && other.upper > 0) ||
           (other.lower == 0 && other.leftClosed) ||
           (other.upper == 0 && other.rightClosed)) {
            return FullInterval;
        }

        // if the dividend interval is zero, the result is zero
        if(lower == 0 && upper == 0 && leftClosed && rightClosed) {
            return Interval(0, 0, true, true);
        }

        condAssert(!other.contains(0), "Division by interval containing zero");
        
        Number p1 = lower / other.lower;
        Number p2 = lower / other.upper;
        Number p3 = upper / other.lower;
        Number p4 = upper / other.upper;
        Number newLower = std::min({p1, p2, p3, p4});
        Number newUpper = std::max({p1, p2, p3, p4});
        bool newLeftClosed = false;
        bool newRightClosed = false;
        if(p1 == newLower) {
            newLeftClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newLower) {
            newLeftClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newLower) {
            newLeftClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newLower) {
            newLeftClosed = rightClosed && other.rightClosed;
        }

        if(p1 == newUpper) {
            newRightClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newUpper) {
            newRightClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newUpper) {
            newRightClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newUpper) {
            newRightClosed = rightClosed && other.rightClosed;
        }
        
        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::divInt(const Number& value) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(value == 0) {
            throw std::domain_error("Division by zero");
        }
        // divInt
        if(value > 0) {
            return Interval((lower / value).floor(), (upper / value).floor(), leftClosed, rightClosed);
        } else {
            return Interval((upper / value).floor(), (lower / value).floor(), rightClosed, leftClosed);
        }
    }

    Interval Interval::divInt(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        // if the divisor interval contains 0, the result is undefined
        if((other.lower < 0 && other.upper > 0) ||
           (other.lower == 0 && other.leftClosed) ||
           (other.upper == 0 && other.rightClosed)) {
            return FullInterval;
        }

        // if the dividend interval is zero, the result is zero
        if(lower == 0 && upper == 0 && leftClosed && rightClosed) {
            return Interval(0, 0, true, true);
        }
        
        Number p1 = (lower / other.lower).floor();
        Number p2 = (lower / other.upper).floor();
        Number p3 = (upper / other.lower).floor();
        Number p4 = (upper / other.upper).floor();
        Number newLower = std::min({p1, p2, p3, p4});
        Number newUpper = std::max({p1, p2, p3, p4});
        bool newLeftClosed = false;
        bool newRightClosed = false;
        if(p1 == newLower) {
            newLeftClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newLower) {
            newLeftClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newLower) {
            newLeftClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newLower) {
            newLeftClosed = rightClosed && other.rightClosed;
        }

        if(p1 == newUpper) {
            newRightClosed = leftClosed && other.leftClosed;
        }
        else if(p2 == newUpper) {
            newRightClosed = leftClosed && other.rightClosed;
        }
        else if(p3 == newUpper) {
            newRightClosed = rightClosed && other.leftClosed;
        }
        else if(p4 == newUpper) {
            newRightClosed = rightClosed && other.rightClosed;
        }
        
        
        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::mod(const Number& value) const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        if(value == Number(0)) {
            throw std::domain_error("Modulo by zero");
        }
        else if(value < Number(0)) {
            throw std::domain_error("Modulo by negative value");
        }
        
        // For modulo operation, the result is usually in the range [0, |value|)
        Number absValue = value.abs();
        Number newLower = Number(0);
        Number newUpper = absValue - Number(1);
        
        // Handle boundary conditions based on the original interval
        bool newLeftClosed = true; // Lower bound is always 0 with closed interval
        bool newRightClosed = true;
        
        // Adjust upper bound if necessary
        if(newUpper > upper) {
            newUpper = upper;
            newRightClosed = rightClosed;
        }
        else if(newUpper < lower) {
            // If modulus is smaller than lower bound of original interval,
            // take closedness from the lower bound
            newRightClosed = leftClosed;
        }
        
        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::mod(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            return EmptyInterval;
        }
        
        // Check for zero in divisor interval
        if((other.lower < Number(0) && other.upper > Number(0)) ||
           (other.lower == Number(0) && other.leftClosed) ||
           (other.upper == Number(0) && other.rightClosed)) {
            throw std::domain_error("Modulo by interval containing zero");
        }
        
        // For negative divisors, convert to positive (|a| mod |b| = a mod b for b > 0)
        Number absLower = other.lower.abs();
        Number absUpper = other.upper.abs();
        
        // The result of modulo is always in [0, max(|divisor|))
        Number maxDivisor = std::max(absLower, absUpper);
        
        // Conservatively, the result can be anywhere in [0, maxDivisor-1]
        // A more precise implementation would consider the specific values
        // in both intervals, but this is a safe approximation
        Number resultLower = Number(0);
        Number resultUpper = maxDivisor - Number(1);
        
        // Refine the upper bound if the original interval is smaller
        if(upper < resultUpper) {
            resultUpper = upper;
            return Interval(resultLower, resultUpper, true, rightClosed);
        }
        
        return Interval(resultLower, resultUpper, true, true);
    }

    Interval Interval::operator%(const Interval& other) const {
        return this->mod(other);
    }

    Interval Interval::operator%(const Number& value) const {
        return this->mod(value);
    }

    Interval Interval::operator^(const Interval& other) const {
        return this->pow(other);
    }

    Interval Interval::operator^(const Number& value) const {
        return this->pow(value);
    }

    Interval Interval::pow2() const {
        if(isEmpty()) {
            return EmptyInterval;
        }
        // power of 2: 2^x
        Number newLower = Number(0);
        Number newUpper = Number::positiveInfinity();
        bool newLeftClosed = this->isLeftClosed();
        bool newRightClosed = this->isRightClosed();
        if(lower.isPositiveInfinity()) {
            newLower = Number::positiveInfinity();
            newLeftClosed = false;
        }
        else if(lower.isNegativeInfinity()) {
            // 2^(-inf) = 0
            newLower = Number(0);
            newLeftClosed = false;
        }
        else{
            if(lower.isInteger()){
                newLower = Number(2).pow(lower.toInteger());
            }
            else{
                newLower = Number(2).pow(lower);
                newLower = newLower.nextBelow();
                newLeftClosed = false;
            }
        }

        if(upper.isPositiveInfinity()) {
            newUpper = Number::positiveInfinity();
            newRightClosed = false;
        }
        else if(upper.isNegativeInfinity()) {
            newUpper = Number(0);
            newRightClosed = false;
        }
        else{
            if(upper.isInteger()){
                newUpper = Number(2).pow(upper.toInteger());
            }
            else{
                newUpper = Number(2).pow(upper);
                newUpper = newUpper.nextAbove();
                newRightClosed = false;
            }
        }

        condAssert(newLower >= Number(0), "2^x is always positive");

        return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
    }

    Interval Interval::pow(const Number& exp) const {
        if(isEmpty()) {
            return EmptyInterval;
        }

        if(exp.isInfinity()) {
            if(exp.isPositiveInfinity()) {
                // x^inf
                if(lower > Number(1)) {
                    return Interval(Number::positiveInfinity(), Number::positiveInfinity(), false, false);
                } else if(lower >= Number(0) && upper <= Number(1)) {
                    if(lower == Number(0) && upper == Number(1)) {
                        return Interval(Number(0), Number(1), true, true);
                    } else {
                        return Interval(Number(0), Number(0).nextAbove(), false, false);
                    }
                } else if(lower < Number(0)) {
                    throw std::domain_error("Cannot compute infinite power with negative base");
                }
            } else {
                // x^-inf
                if(lower > Number(1)) {
                    return Interval(Number(0), Number(0).nextAbove(), false, false);
                } else if(lower >= Number(0) && upper <= Number(1)) {
                    if(upper == Number(1)) {
                        return Interval(Number(1), Number::positiveInfinity(), true, false);
                    } else {
                        return Interval(Number::positiveInfinity(), Number::positiveInfinity(), false, false);
                    }
                } else if(lower <= Number(0) && upper >= Number(0)) {
                    throw std::domain_error("Cannot compute infinite power of interval containing zero");
                } else {
                    throw std::domain_error("Cannot compute infinite power with complex results");
                }
            }
        }
        
        // If exp is an integer
        if(exp.isInteger()) {
            int n = exp.toInteger().toInt();
            
            if(n == 0) {
                // x^0 = 1 (assuming x != 0)
                return Interval(Number(1), Number(1), true, true);
            } else if(n > 0) {
                if(n % 2 == 0) {
                    // even power
                    if(lower >= Number(0)) {
                        // [0, inf) ^ even = [0, inf)
                        return Interval(lower.pow(n), upper.pow(n), leftClosed, rightClosed);
                    } else if(upper <= Number(0)) {
                        // (-inf, 0] ^ even = [0, inf)
                        // flip the closedness, because the even power of negative number will reverse the interval direction
                        return Interval(upper.pow(n), lower.pow(n), rightClosed, leftClosed);
                    } else {
                        // the interval crosses zero, the minimum value is at x=0
                        Number maxVal = std::max(lower.abs().pow(n), upper.abs().pow(n));
                        return Interval(Number(0), maxVal, true, 
                           ((lower.abs() > upper.abs()) ? leftClosed : rightClosed));
                    }
                } else {
                    // odd power - monotonically increasing
                    return Interval(lower.pow(n), upper.pow(n), leftClosed, rightClosed);
                }
            } else { // n < 0
                // negative power - need to handle the case of division by zero
                if(lower <= Number(0) && upper >= Number(0)) {
                    // the interval contains zero, the result is undefined
                    throw std::domain_error("Cannot compute negative power of interval containing zero");
                } else {
                    // apply x^(-n) = 1/(x^n)
                    if(n % 2 == 0) {
                        // even negative power
                        if(lower > Number(0) || upper < Number(0)) {
                            // positive or negative base
                            // for even negative power, the result is always positive
                            // when the base is positive, the function is monotonically decreasing
                            if(lower > Number(0)) {
                                return Interval(upper.pow(n), lower.pow(n), rightClosed, leftClosed);
                            } 
                            // when the base is negative, the function is monotonically decreasing
                            else {
                                // for negative number, compare the absolute value
                                return Interval(lower.abs().pow(n), upper.abs().pow(n), leftClosed, rightClosed);
                            }
                        }
                    } else {
                        // odd negative power
                        // for odd negative power, the function is monotonically decreasing
                        // and keep the sign of the original number
                        return Interval(upper.pow(n), lower.pow(n), rightClosed, leftClosed);
                    }
                }
            }
        }
        
        // non-integer power
        // only when the base is positive has meaning
        if(lower > Number(0)) {
            if(lower.isInfinity() || upper.isInfinity()) {
                if(exp > Number(0)) {
                    return Interval(Number::positiveInfinity(), Number::positiveInfinity(), false, false);
                } else {
                    return Interval(Number(0), Number(0).nextAbove(), false, false);
                }
            }
            
            // non-integer power produces a floating error, so we need to use the nextBelow and nextAbove to get the result
            return Interval(lower.pow(exp).nextBelow(), upper.pow(exp).nextAbove(), leftClosed, rightClosed);
        } else if(lower <= Number(0) && exp >= Number(0)) {
            // if the base contains 0 or negative number, and the exponent is non-negative, then special processing is needed
            if(upper <= Number(0)) {
                // if the whole interval is negative, and the exponent is not an integer, the result is undefined
                throw std::domain_error("Cannot compute non-integer power of negative interval");
            }
            // the interval crosses zero, the result starts from 0
            return Interval(Number(0), upper.pow(exp).nextAbove(), true, false);
        } else {
            // for negative number, the non-integer power is undefined
            throw std::domain_error("Cannot compute non-integer power with negative base");
        }
    }

    Interval Interval::pow(const Interval& exp) const {
        if(isEmpty() || exp.isEmpty()) {
            return EmptyInterval;
        }

        if(exp.isPoint()){
            return pow(exp.getLower());
        }

        if(isPoint()){
            auto val = getLower();
            if(val == Number(0) || val == Number(1)){
                return Interval(val, val, true, true);
            }
            else if(val == Number(2)){
                return exp.pow2();
            }
            else{
                
                // power of val: val^x
                Number newLower = Number(0);
                Number newUpper = Number::positiveInfinity();
                bool newLeftClosed = true;
                bool newRightClosed = true;
                if(exp.getLower().isPositiveInfinity()) {
                    newLower = Number::positiveInfinity();
                    newLeftClosed = false;
                }
                else if(exp.getLower().isNegativeInfinity()) {
                    // val^(-inf) = 0
                    newLower = Number(0);
                    newLeftClosed = false;
                }
                else{
                    if(exp.getLower().isInteger() && val.isInteger()){
                        newLower = val.pow(exp.getLower().toInteger());
                    }
                    else{
                        newLower = val.pow(exp.getLower());
                        newLower = newLower.nextBelow();
                        newLeftClosed = false;
                    }
                }

                if(exp.getUpper().isPositiveInfinity()) {
                    newUpper = Number::positiveInfinity();
                    newRightClosed = false;
                }
                else if(exp.getUpper().isNegativeInfinity()) {
                    newUpper = Number(0);
                    newRightClosed = false;
                }
                else{
                    if(exp.getUpper().isInteger() && val.isInteger()){
                        newUpper = val.pow(exp.getUpper().toInteger());
                    }
                    else{
                        newUpper = val.pow(exp.getUpper());
                        newUpper = newUpper.nextAbove();
                        newRightClosed = false;
                    }
                }

                return Interval(newLower, newUpper, newLeftClosed, newRightClosed);
            }
        }
        
        // if the base interval is completely positive
        if(lower > Number(0)) {
            // calculate the four corner points
            std::vector<Number> vals;
            vals.push_back(lower.pow(exp.getLower()));
            vals.push_back(lower.pow(exp.getUpper()));
            vals.push_back(upper.pow(exp.getLower()));
            vals.push_back(upper.pow(exp.getUpper()));
            vals.erase(std::remove_if(vals.begin(), vals.end(), isNaNNumber), 
                        vals.end());
            // if all the values are NaN, then the result is undefined
            if(vals.empty()){
                return FullInterval;
            }
            
            // find the minimum and maximum values
            Number minVal = vals[0];
            Number maxVal = vals[0];
            for(auto val : vals){
                if(val < minVal){
                    minVal = val;
                }
                if(val > maxVal){
                    maxVal = val;
                }
            }
            
            // the closedness depends on the relation between the corner points and the boundary values
            bool newLeftClosed = false, newRightClosed = false;
            
            if(minVal == vals[0]){
                newLeftClosed = leftClosed && exp.isLeftClosed();
            }
            else if(minVal == vals[1]){
                newLeftClosed = leftClosed && exp.isRightClosed();
            }
            else if(minVal == vals[2]){
                newLeftClosed = rightClosed && exp.isLeftClosed();
            }
            else if(minVal == vals[3]){
                newLeftClosed = rightClosed && exp.isRightClosed();
            }

            if(maxVal == vals[0]){
                newRightClosed = leftClosed && exp.isLeftClosed();
            }
            else if(maxVal == vals[1]){
                newRightClosed = leftClosed && exp.isRightClosed();
            }
            else if(maxVal == vals[2]){
                newRightClosed = rightClosed && exp.isLeftClosed();
            }
            else if(maxVal == vals[3]){
                newRightClosed = rightClosed && exp.isRightClosed();
            }
            return Interval(minVal, maxVal, newLeftClosed, newRightClosed);
        }
        
        // if the base contains 0 or negative number, and the exponent is not a fixed integer, then special processing is needed
        // this case is very complex, and may need to discuss different intervals
        throw std::domain_error("Cannot compute interval power with negative or zero base and non-integer exponent");
    }

    Interval Interval::atan2(const Number& x) const {
        if(isEmpty() || x.isNaN()) {
            return EmptyInterval;
        }
        
        // the domain of atan2(y, x) is the whole plane, and it will not cause division by zero problem
        // the range of atan2 is [-π, π]
        
        // if x is a fixed number, then the result is the atan2 of each point in the interval
        Number low = Number::atan2(lower, x);
        Number high = Number::atan2(upper, x);
        
        // special case: if the interval crosses the x-axis, and x < 0, then the result will cross the π or -π discontinuity point
        if(lower < Number(0) && upper > Number(0) && x < Number(0)) {
            // in this case, the result of atan2 is the complete range of [-π, π]
            return Interval(-Number::pi(), Number::pi(), true, true);
        }
        bool newLeftClosed = leftClosed;
        bool newRightClosed = rightClosed;
        // check if the result needs to be sorted in ascending order
        if(low > high) {
            std::swap(low, high);
            std::swap(newLeftClosed, newRightClosed);
        }

        // add a small value
        if(low < -Number::pi()){
            low = -Number::pi();
            newLeftClosed = true;
        }
        else {
            low = low.nextBelow();
            newLeftClosed = false;
        }

        if(high > Number::pi()){
            high = Number::pi();
            newRightClosed = true;
        }
        else{
            high = high.nextAbove();
            newRightClosed = false;
        }
        return Interval(low, high, newLeftClosed, newRightClosed);
    }

    Interval Interval::atan2(const Interval& x) const {
        if(isEmpty() || x.isEmpty()) {
            return EmptyInterval;
        }
        
        // the domain of atan2(y, x) is the whole plane, and the range is [-π, π]
        
        // we need to check if there are some special cases:
        
        // 1. if y and x interval both contain 0, then the result can be any angle
        if((lower <= Number(0) && upper >= Number(0)) && 
           (x.getLower() <= Number(0) && x.getUpper() >= Number(0))) {
            return Interval(-Number::pi(), Number::pi(), true, true);
        }
        
        // 2. if y interval crosses 0, and x is completely positive or negative
        if(lower < Number(0) && upper > Number(0)) {
            if(x.getUpper() < Number(0)) {
                // x is completely negative, the result is [-π, 0] U [0, π], which is the whole [-π, π]
                return Interval(-Number::pi(), Number::pi(), true, true);
            } else if(x.getLower() > Number(0)) {
                // x is completely positive, the result is [-π/2, π/2]
                return Interval(-Number::pi() / Number(2), Number::pi() / Number(2), true, true);
            }
        }
        
        // 3. if x interval crosses 0, and y is completely positive or negative
        if(x.getLower() < Number(0) && x.getUpper() > Number(0)) {
            if(lower > Number(0)) {
                // y is completely positive, the result is [0, π]
                return Interval(Number(0), Number::pi(), true, true);
            } else if(upper < Number(0)) {
                // y is completely negative, the result is [-π, 0]
                return Interval(-Number::pi(), Number(0), true, true);
            }
        }
        
        // for other cases, we need to calculate the four corner points
        Number vals[4] = {
            Number::atan2(lower, x.getLower()),
            Number::atan2(lower, x.getUpper()),
            Number::atan2(upper, x.getLower()),
            Number::atan2(upper, x.getUpper())
        };
        
        // find the minimum and maximum values
        Number minVal = std::min({vals[0], vals[1], vals[2], vals[3]});
        Number maxVal = std::max({vals[0], vals[1], vals[2], vals[3]});
        
        // check if it crosses the discontinuity point π or -π
        if(maxVal - minVal > Number::pi()) {
            // if the maximum value and minimum value differ by more than π, it means it crosses the discontinuity point
            return Interval(-Number::pi(), Number::pi(), true, true);
        }
        
        // the closedness depends on the relation between the corner points and the boundary values
        bool newLeftClosed = false, newRightClosed = false;
        
        if(minVal == vals[0])
            newLeftClosed = leftClosed && x.isLeftClosed();
        else if(minVal == vals[1])
            newLeftClosed = leftClosed && x.isRightClosed();
        else if(minVal == vals[2])
            newLeftClosed = rightClosed && x.isLeftClosed();
        else if(minVal == vals[3])
            newLeftClosed = rightClosed && x.isRightClosed();
        
        if(maxVal == vals[0])
            newRightClosed = leftClosed && x.isLeftClosed();
        else if(maxVal == vals[1])
            newRightClosed = leftClosed && x.isRightClosed();
        else if(maxVal == vals[2])
            newRightClosed = rightClosed && x.isLeftClosed();
        else if(maxVal == vals[3])
            newRightClosed = rightClosed && x.isRightClosed();

        // add a small value
        if(minVal < -Number::pi()){
            minVal = -Number::pi();
            newLeftClosed = true;
        }
        else{
            minVal = minVal.nextBelow();
            newLeftClosed = false;
        }

        if(maxVal > Number::pi()){
            maxVal = Number::pi();
            newRightClosed = true;
        }
        else{
            maxVal = maxVal.nextAbove();
            newRightClosed = false;
        }
        
        return Interval(minVal, maxVal, newLeftClosed, newRightClosed);
    }

    bool Interval::operator<(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            throw std::invalid_argument("Cannot compare empty intervals");
        }
        
        // An interval A is strictly less than interval B if:
        // The upper bound of A is less than the lower bound of B
        // Or they touch at a single point but at least one interval is open at that point
        return upper < other.lower || 
               (upper == other.lower && (!rightClosed || !other.leftClosed));
    }

    bool Interval::operator<=(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            throw std::invalid_argument("Cannot compare empty intervals");
        }
        
        // A <= B means that it's not true that A > B
        return !(*this > other);
    }

    bool Interval::operator>(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            throw std::invalid_argument("Cannot compare empty intervals");
        }
        
        // An interval A is strictly greater than interval B if:
        // The lower bound of A is greater than the upper bound of B
        // Or they touch at a single point but at least one interval is open at that point
        return lower > other.upper || 
               (lower == other.upper && (!leftClosed || !other.rightClosed));
    }

    bool Interval::operator>=(const Interval& other) const {
        if(isEmpty() || other.isEmpty()) {
            throw std::invalid_argument("Cannot compare empty intervals");
        }
        
        // A >= B means that it's not true that A < B
        return !(*this < other);
    }

    Interval& Interval::operator+=(const Number& value) {
        lower += value;
        upper += value;
        return *this;
    }

    Interval& Interval::operator-=(const Number& value) {
        lower -= value;
        upper -= value;
        return *this;
    }

    Interval& Interval::operator*=(const Number& value) {
        if(value >= 0) {
            lower *= value;
            upper *= value;
        } else {
            Number temp = lower;
            lower = upper * value;
            upper = temp * value;
            bool tempClosed = leftClosed;
            leftClosed = rightClosed;
            rightClosed = tempClosed;
        }
        return *this;
    }

    Interval& Interval::operator/=(const Number& value) {
        if(value == 0) {
            throw std::domain_error("Division by zero");
        }
        if(value > 0) {
            lower /= value;
            upper /= value;
        } else {
            Number temp = lower;
            lower = upper / value;
            upper = temp / value;
            bool tempClosed = leftClosed;
            leftClosed = rightClosed;
            rightClosed = tempClosed;
        }
        return *this;
    }

    Interval Interval::operate(const NODE_KIND& kind) const{
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        switch(kind) {
            case NODE_KIND::NT_NEG:
                return this->negate();
            case NODE_KIND::NT_ABS:
                return this->abs();
            case NODE_KIND::NT_LB:
                return this->lb();
            case NODE_KIND::NT_LN:
                return this->ln();
            case NODE_KIND::NT_LG:
                return this->lg();
            case NODE_KIND::NT_EXP:
                return this->exp();
            case NODE_KIND::NT_SQRT:
                return this->sqrt();
            case NODE_KIND::NT_SAFESQRT:
                return this->safeSqrt();
            case NODE_KIND::NT_SIN:
                return this->sin();
            case NODE_KIND::NT_COS:
                return this->cos();
            case NODE_KIND::NT_TAN:
                return this->tan();
            case NODE_KIND::NT_COT:
                return this->cot();
            case NODE_KIND::NT_SEC:
                return this->sec();
            case NODE_KIND::NT_CSC:
                return this->csc();
            case NODE_KIND::NT_ASIN:
                return this->asin();
            case NODE_KIND::NT_ACOS:
                return this->acos();
            case NODE_KIND::NT_ATAN:
                return this->atan();
            case NODE_KIND::NT_ACOT:
                return this->acot();
            case NODE_KIND::NT_ASEC:
                return this->asec();
            case NODE_KIND::NT_ACSC:
                return this->acsc();
            case NODE_KIND::NT_SINH:
                return this->sinh();
            case NODE_KIND::NT_COSH:
                return this->cosh();
            case NODE_KIND::NT_TANH:
                return this->tanh();
            case NODE_KIND::NT_COTH:
                return this->coth();
            case NODE_KIND::NT_SECH:
                return this->sech();
            case NODE_KIND::NT_CSCH:
                return this->csch();
            case NODE_KIND::NT_ASINH:
                return this->asinh();
            case NODE_KIND::NT_ACOSH:
                return this->acosh();
            case NODE_KIND::NT_ATANH:
                return this->atanh();
            case NODE_KIND::NT_ACOTH:
                return this->acoth();
            case NODE_KIND::NT_ASECH:
                return this->asech();
            case NODE_KIND::NT_ACSCH:
                return this->acsch();
            default:
                throw std::invalid_argument("Unsupported unary operation");
        }
    }

    Interval Interval::operate(const NODE_KIND& kind, const Number& value) const{
        if(isEmpty()) {
            return EmptyInterval;
        }
        
        switch(kind) {
            case NODE_KIND::NT_ADD:
                return (*this + value);
            case NODE_KIND::NT_SUB:
                return (*this - value);
            case NODE_KIND::NT_MUL:
                return (*this * value);
            case NODE_KIND::NT_DIV_REAL:
                return this->divReal(value);
            case NODE_KIND::NT_DIV_INT:
                return this->divInt(value);
            case NODE_KIND::NT_MOD:
                return this->mod(value);
            case NODE_KIND::NT_POW:
                return this->pow(value);
            case NODE_KIND::NT_ATAN2:
                return this->atan2(value);
            case NODE_KIND::NT_LT:
            case NODE_KIND::NT_LE:
            case NODE_KIND::NT_GT:
            case NODE_KIND::NT_GE:
            case NODE_KIND::NT_EQ:
            case NODE_KIND::NT_DISTINCT:
                // comparison operations return boolean values, not supported for interval arithmetic
                throw std::invalid_argument("Comparison operations not supported for interval arithmetic");
            default:
                throw std::invalid_argument("Unsupported binary operation with Number");
        }
    }

    Interval Interval::operate(const NODE_KIND& kind, const Interval& other) const{
        if(isEmpty() || other.isEmpty()) {
            throw std::invalid_argument("One or both intervals are empty");
        }
        
        switch(kind) {
            case NODE_KIND::NT_ADD:
                return (*this + other);
            case NODE_KIND::NT_SUB:
                return (*this - other);
            case NODE_KIND::NT_MUL:
                return (*this * other);
            case NODE_KIND::NT_DIV_REAL:
                return this->divReal(other);
            case NODE_KIND::NT_DIV_INT:
                return this->divInt(other);
            case NODE_KIND::NT_MOD:
                return this->mod(other);
            case NODE_KIND::NT_POW:
                return this->pow(other);
            case NODE_KIND::NT_ATAN2:
                return this->atan2(other);
            case NODE_KIND::NT_AND:
                // and operation
                if(this->isIntersectingWith(other)) {
                    return this->intersection(other);
                } else {
                    // return empty interval
                    return Interval(Number(1), Number(0), false, false);
                }
            case NODE_KIND::NT_OR:
                // or operation
                if(this->isIntersectingWith(other) || !this->isDisjointFrom(other)) {
                    return this->unionWith(other);
                } else {
                    // non-intersecting intervals cannot be represented by a single interval, return the smallest interval containing both
                    Number newLower = std::min(lower, other.lower);
                    Number newUpper = std::max(upper, other.upper);
                    return Interval(newLower, newUpper, false, false);
                }
            case NODE_KIND::NT_LT:
            case NODE_KIND::NT_LE:
            case NODE_KIND::NT_GT:
            case NODE_KIND::NT_GE:
            case NODE_KIND::NT_EQ:
            case NODE_KIND::NT_DISTINCT:
                // comparison operations return boolean values, not supported for interval arithmetic
                throw std::invalid_argument("Comparison operations not supported for interval arithmetic");
            default:
                throw std::invalid_argument("Unsupported binary operation with Interval");
        }
    }
}
