/* -*- Header -*-
 *
 * The Value Class
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
#ifndef VALUE_HEADER
#define VALUE_HEADER
#include "somtparser/ir/number.h"
#include "somtparser/ir/interval.h"
#include <memory>

namespace SOMTParser{
    // This class is used to store the value of a variable
    // It can be any kind of value, including a number, an interval, a string, a boolean, etc.
    enum ValueType{
        UNKNOWN,
        STRING,
        NUMBER,
        INTERVAL,
        BOOLEAN,
        BV,
        FP,
        ARRAY
    };

    class Value{
        public:
            Value();
            Value(const Value& other);
            Value& operator=(const Value& other);
            ~Value();

            // constructor
            Value(const std::string& string_value);
            Value(const Number& number_value);
            Value(const Interval& interval_value);
            Value(const bool& boolean_value);
            Value(const ValueType& value_type); // empty value

            void setValue(const std::string& string_value);
            void setValue(const Number& number_value);
            void setValue(const Interval& interval_value);
            void setValue(const bool& boolean_value);

            ValueType getType() const;
            std::string getStringValue() const;
            Number getNumberValue() const;
            Interval getIntervalValue() const;
            bool getBooleanValue() const;

            // Assignment operators for different types
            Value& operator=(const std::string& string_value);
            Value& operator=(const Number& number_value);
            Value& operator=(const Interval& interval_value);
            Value& operator=(const bool& boolean_value);

            // all the operators are defined in the number, interval, string, bool, bv, fp, array
           
            // all the comparison operators
            bool operator==(const Value& other) const;
            bool operator!=(const Value& other) const;
            bool operator<(const Value& other) const;
            bool operator<=(const Value& other) const;
            bool operator>(const Value& other) const;
            bool operator>=(const Value& other) const;

            // all the assignment operators
            Value& operator+=(const Value& other);
            Value& operator-=(const Value& other);
            Value& operator*=(const Value& other);
            Value& operator/=(const Value& other);
            Value& operator%=(const Value& other);
            Value& operator^=(const Value& other);
            Value& operator&=(const Value& other);
            Value& operator|=(const Value& other);
            Value& operator<<=(const Value& other);
            Value& operator>>=(const Value& other);
            Value& operator++();
            Value& operator--();
            Value operator++(int);
            Value operator--(int);

            // all the logical operators are defined in the bool
            bool operator&&(const Value& other) const;
            bool operator||(const Value& other) const;
            bool operator!() const;

            // all arithmetic operators are defined in the number
            Value operator+(const Value& other) const;
            Value operator-(const Value& other) const;
            Value operator*(const Value& other) const;
            Value operator/(const Value& other) const;
            Value operator%(const Value& other) const;
            Value operator^(const Value& other) const;
            Value operator&(const Value& other) const;
            Value operator|(const Value& other) const;
            Value operator~() const;
            Value operator<<(const Value& other) const;
            Value operator>>(const Value& other) const;

            // all arithmetic operators are defined in the number
            Value neg() const;
            Value abs() const;
            Value sqrt() const;
            Value safeSqrt() const;
            Value pow(const Value& other) const;
            Value exp() const;
            Value ln() const;
            Value lg() const;
            Value lb() const;
            Value log(const Value& other) const;
            Value ceil() const;
            Value floor() const;
            Value round() const;
            Value sin() const;
            Value cos() const;
            Value tan() const;
            Value cot() const;
            Value sec() const;
            Value csc() const;
            Value asin() const;
            Value acos() const;
            Value atan() const;
            Value acot() const;
            Value asec() const;
            Value acsc() const;
            Value atan2(const Value& other) const;
            Value sinh() const;
            Value cosh() const;
            Value tanh() const;
            Value coth() const;
            Value sech() const;
            Value csch() const;
            Value asinh() const;
            Value acosh() const;
            Value atanh() const;
            Value acoth() const;
            Value asech() const;
            Value acsch() const;

            // all the bitwise operators are defined in the bv
            // Value notOp() const;  // renamed from 'not' as it's a C++ keyword
            // Value andOp(const Value& other) const;  // renamed from 'and'
            // Value orOp(const Value& other) const;   // renamed from 'or'
            // Value xorOp(const Value& other) const;  // renamed from 'xor'
            // Value impliesOp(const Value& other) const; // renamed from 'implies'
            // Value concatBv(const Value& other) const;  // renamed from 'concat' (to avoid conflict with string concat)
            // Value extract(const Value& other) const;
            // Value repeatBv(const Value& other) const;  // renamed from 'repeat' (to avoid conflict with string repeat)
            // Value rotate_left(const Value& other) const;
            // Value rotate_right(const Value& other) const;
            // Value shift_left(const Value& other) const;
            // Value shift_right(const Value& other) const;

            // // all the fp operators are defined in the fp
            // Value fadd(const Value& other) const;
            // Value fsub(const Value& other) const;
            // Value fmul(const Value& other) const;
            // Value fdiv(const Value& other) const;
            // Value frem(const Value& other) const;

            // // all the array operators are defined in the array
            // Value select(const Value& index) const;
            // Value store(const Value& index, const Value& value) const;

            // all the string operators are defined in the string
            Value concatStr(const Value& other) const;  // renamed from 'concat'
            Value substr(const Value& start, const Value& end) const;
            Value repeatStr(const Value& other) const;  // renamed from 'repeat'
            Value replace(const Value& old, const Value& newVal) const;  // renamed parameter 'new' as it's a C++ keyword
            Value split(const Value& delimiter) const; // array is also a value
            Value join(const Value& delimiter) const;
            Value reverse() const;
            Value sortStr() const;  // renamed from 'sort' to avoid conflict with STL
            Value unique() const;
            Value trim() const;
            Value ltrim() const;
            Value rtrim() const;
            Value toLower() const;
            Value toUpper() const;
            Value toNumber() const;
            Value toBoolean() const;
            Value toBV() const;
            Value toFP() const;
            Value toArray() const;

            // print
            std::string toString() const;
        private:
            std::string string_value;
            Number number_value;
            Interval interval_value;
            bool boolean_value;

            ValueType value_type;
    };

    std::shared_ptr<Value> newValue(const std::string& string_value);
    std::shared_ptr<Value> newValue(const Number& number_value);
    std::shared_ptr<Value> newValue(const Interval& interval_value);
    std::shared_ptr<Value> newValue(const bool& boolean_value);
    std::shared_ptr<Value> newValue(const ValueType& value_type);
    std::shared_ptr<Value> newValue(const int& integer_value);
    std::shared_ptr<Value> newValue(const double& double_value);
    std::shared_ptr<Value> newValue(const float& float_value);
    std::shared_ptr<Value> newValue(const long& long_value);
    std::shared_ptr<Value> newValue(const short& short_value);
    std::shared_ptr<Value> newValue(const char& char_value);
    
}
#endif