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

#include "somtparser/ir/value.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cstring>
#include <cmath>

namespace {
    bool isNotSpaceInt(int ch) { return !std::isspace(ch); }
    unsigned char toLowerChar(unsigned char c) { return static_cast<unsigned char>(std::tolower(c)); }
    unsigned char toUpperChar(unsigned char c) { return static_cast<unsigned char>(std::toupper(c)); }
} // anonymous namespace

namespace SOMTParser {
    // Default constructor
    Value::Value() : value_type(UNKNOWN) {}

    // Copy constructor
    Value::Value(const Value& other) : 
        string_value(other.string_value),
        number_value(other.number_value),
        interval_value(other.interval_value),
        boolean_value(other.boolean_value),
        value_type(other.value_type),
        bv_width_(other.bv_width_),
        fp_smtlib_(other.fp_smtlib_),
        fp_eb_(other.fp_eb_),
        fp_sb_(other.fp_sb_),
        array_data_(other.array_data_),
        array_default_(other.array_default_) {}

    // Assignment operator
    Value& Value::operator=(const Value& other) {
        if (this != &other) {
            string_value = other.string_value;
            number_value = other.number_value;
            interval_value = other.interval_value;
            boolean_value = other.boolean_value;
            value_type = other.value_type;
            bv_width_ = other.bv_width_;
            fp_smtlib_ = other.fp_smtlib_;
            fp_eb_ = other.fp_eb_;
            fp_sb_ = other.fp_sb_;
            array_data_ = other.array_data_;
            array_default_ = other.array_default_;
        }
        return *this;
    }

    // Destructor
    Value::~Value() {}

    // Constructors for different types
    Value::Value(const std::string& string_value) : 
        string_value(string_value), value_type(STRING) {}
    
    Value::Value(const Number& number_value) : 
        number_value(number_value), value_type(NUMBER) {}
    
    Value::Value(const Interval& interval_value) : 
        interval_value(interval_value), value_type(INTERVAL) {}
    
    Value::Value(const bool& boolean_value) : 
        boolean_value(boolean_value), value_type(BOOLEAN) {}
    
    Value::Value(const ValueType& value_type) : value_type(value_type) {}

    // Setter methods
    void Value::setValue(const std::string& string_value) {
        this->string_value = string_value;
        this->value_type = STRING;
    }
    
    void Value::setValue(const Number& number_value) {
        this->number_value = number_value;
        this->value_type = NUMBER;
    }
    
    void Value::setValue(const Interval& interval_value) {
        this->interval_value = interval_value;
        this->value_type = INTERVAL;
    }
    
    void Value::setValue(const bool& boolean_value) {
        this->boolean_value = boolean_value;
        this->value_type = BOOLEAN;
    }

    // Getter methods
    ValueType Value::getType() const {
        return value_type;
    }
    
    std::string Value::getStringValue() const {
        if (value_type != STRING) {
            throw std::runtime_error("Value is not a string");
        }
        return string_value;
    }
    
    Number Value::getNumberValue() const {
        if (value_type != NUMBER) {
            throw std::runtime_error("Value is not a number (getNumberValue is only for NUMBER; use BitVectorUtils on BV literals for unsigned/signed interpretation)");
        }
        return number_value;
    }
    
    Interval Value::getIntervalValue() const {
        if (value_type != INTERVAL) {
            throw std::runtime_error("Value is not an interval");
        }
        return interval_value;
    }
    
    bool Value::getBooleanValue() const {
        if (value_type != BOOLEAN) {
            throw std::runtime_error("Value is not a boolean");
        }
        return boolean_value;
    }

    // Assignment operators for different types
    Value& Value::operator=(const std::string& string_value) {
        this->string_value = string_value;
        this->value_type = STRING;
        return *this;
    }
    
    Value& Value::operator=(const Number& number_value) {
        this->number_value = number_value;
        this->value_type = NUMBER;
        return *this;
    }
    
    Value& Value::operator=(const Interval& interval_value) {
        this->interval_value = interval_value;
        this->value_type = INTERVAL;
        return *this;
    }
    
    Value& Value::operator=(const bool& boolean_value) {
        this->boolean_value = boolean_value;
        this->value_type = BOOLEAN;
        return *this;
    }

    // toString method
    std::string Value::toString() const {
        switch (value_type) {
            case STRING:
                return string_value;
            case NUMBER:
                return number_value.toString();
            case INTERVAL:
                return interval_value.toString();
            case BOOLEAN:
                return boolean_value ? "true" : "false";
            case BV:
                return number_value.toString();
            case FP:
                return fp_smtlib_;
            case ARRAY:
                return "(array)";
            default:
                return "unknown";
        }
    }

    // Factory functions to create shared_ptr<Value>
    std::shared_ptr<Value> newValue(const std::string& string_value) {
        return std::make_shared<Value>(string_value);
    }
    
    std::shared_ptr<Value> newValue(const Number& number_value) {
        return std::make_shared<Value>(number_value);
    }
    
    std::shared_ptr<Value> newValue(const Interval& interval_value) {
        return std::make_shared<Value>(interval_value);
    }
    
    std::shared_ptr<Value> newValue(const bool& boolean_value) {
        return std::make_shared<Value>(boolean_value);
    }
    
    std::shared_ptr<Value> newValue(const ValueType& value_type) {
        return std::make_shared<Value>(value_type);
    }
    
    std::shared_ptr<Value> newValue(const int& integer_value) {
        return std::make_shared<Value>(Number(integer_value));
    }
    
    std::shared_ptr<Value> newValue(const double& double_value) {
        return std::make_shared<Value>(Number::fromApproxDouble(double_value));
    }
    
    std::shared_ptr<Value> newValue(const float& float_value) {
        return std::make_shared<Value>(Number::fromApproxDouble(static_cast<double>(float_value)));
    }
    
    std::shared_ptr<Value> newValue(const long& long_value) {
        return std::make_shared<Value>(Number(static_cast<int>(long_value)));
    }
    
    std::shared_ptr<Value> newValue(const short& short_value) {
        return std::make_shared<Value>(Number(static_cast<int>(short_value)));
    }
    
    std::shared_ptr<Value> newValue(const char& char_value) {
        return std::make_shared<Value>(Number(static_cast<int>(char_value)));
    }

    // Comparison operators
    bool Value::operator==(const Value& other) const {
        if (value_type != other.value_type) {
            return false; // Different types are not equal
        }
        
        switch (value_type) {
            case STRING:
                return string_value == other.string_value;
            case NUMBER:
                return number_value == other.number_value;
            case INTERVAL:
                return interval_value == other.interval_value;
            case BOOLEAN:
                return boolean_value == other.boolean_value;
            default:
                return false;
        }
    }
    
    bool Value::operator!=(const Value& other) const {
        return !(*this == other);
    }
    
    bool Value::operator<(const Value& other) const {
        if (value_type != other.value_type) {
            throw std::runtime_error("Cannot compare values of different types");
        }
        
        switch (value_type) {
            case STRING:
                return string_value < other.string_value;
            case NUMBER:
                return number_value < other.number_value;
            case INTERVAL:
                return interval_value < other.interval_value;
            case BOOLEAN:
                return boolean_value < other.boolean_value;
            default:
                throw std::runtime_error("Cannot compare values of this type");
        }
    }
    
    bool Value::operator<=(const Value& other) const {
        return (*this < other) || (*this == other);
    }
    
    bool Value::operator>(const Value& other) const {
        return !(*this <= other);
    }
    
    bool Value::operator>=(const Value& other) const {
        return !(*this < other);
    }

    // Arithmetic operators
    Value Value::operator+(const Value& other) const {
        // Type checking and conversion
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value + other.number_value);
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value + other.interval_value);
        } else if (value_type == STRING && other.value_type == STRING) {
            return Value(string_value + other.string_value); // String concatenation
        } else {
            throw std::runtime_error("Cannot add values of these types");
        }
    }
    
    Value Value::operator-(const Value& other) const {
        // Type checking
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value - other.number_value);
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value - other.interval_value);
        } else {
            throw std::runtime_error("Cannot subtract values of these types");
        }
    }
    
    Value Value::operator*(const Value& other) const {
        // Type checking
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value * other.number_value);
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value * other.interval_value);
        } else {
            throw std::runtime_error("Cannot multiply values of these types");
        }
    }
    
    Value Value::operator/(const Value& other) const {
        // Type checking and division by zero
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value / other.number_value);
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value / other.interval_value);
        } else {
            throw std::runtime_error("Cannot divide values of these types");
        }
    }
    
    Value Value::operator%(const Value& other) const {
        // Type checking
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value % other.number_value);
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value.mod(other.interval_value));
        } else {
            throw std::runtime_error("Cannot perform modulo on values of these types");
        }
    }
    
    // Mathematical functions
    Value Value::sin() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.sin());
            case INTERVAL:
                return Value(interval_value.sin());
            default:
                throw std::runtime_error("Cannot compute sine of this type");
        }
    }
    
    Value Value::cos() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.cos());
            case INTERVAL:
                return Value(interval_value.cos());
            default:
                throw std::runtime_error("Cannot compute cosine of this type");
        }
    }
    
    Value Value::tan() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.tan());
            case INTERVAL:
                return Value(interval_value.tan());
            default:
                throw std::runtime_error("Cannot compute tangent of this type");
        }
    }
    
    Value Value::sqrt() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.sqrt());
            case INTERVAL:
                return Value(interval_value.sqrt());
            default:
                throw std::runtime_error("Cannot compute square root of this type");
        }
    }
    
    Value Value::exp() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.exp());
            case INTERVAL:
                return Value(interval_value.exp());
            default:
                throw std::runtime_error("Cannot compute exponential of this type");
        }
    }
    
    Value Value::ln() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.ln());
            case INTERVAL:
                return Value(interval_value.ln());
            default:
                throw std::runtime_error("Cannot compute natural logarithm of this type");
        }
    }
    
    Value Value::abs() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.abs());
            case INTERVAL:
                return Value(interval_value.abs());
            default:
                throw std::runtime_error("Cannot compute absolute value of this type");
        }
    }
    
    Value Value::pow(const Value& other) const {
        // Type checking
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value.pow(other.number_value));
        } else if (value_type == INTERVAL && other.value_type == INTERVAL) {
            return Value(interval_value.pow(other.interval_value));
        } else if (value_type == INTERVAL && other.value_type == NUMBER) {
            return Value(interval_value.pow(other.number_value));
        } else {
            throw std::runtime_error("Cannot compute power with these types");
        }
    }

    // Logical operators
    bool Value::operator&&(const Value& other) const {
        if (value_type != BOOLEAN || other.value_type != BOOLEAN) {
            throw std::runtime_error("Logical AND requires boolean operands");
        }
        return boolean_value && other.boolean_value;
    }
    
    bool Value::operator||(const Value& other) const {
        if (value_type != BOOLEAN || other.value_type != BOOLEAN) {
            throw std::runtime_error("Logical OR requires boolean operands");
        }
        return boolean_value || other.boolean_value;
    }
    
    bool Value::operator!() const {
        if (value_type != BOOLEAN) {
            throw std::runtime_error("Logical NOT requires a boolean operand");
        }
        return !boolean_value;
    }
    
    // Assignment arithmetic operators
    Value& Value::operator+=(const Value& other) {
        *this = *this + other;
        return *this;
    }
    
    Value& Value::operator-=(const Value& other) {
        *this = *this - other;
        return *this;
    }
    
    Value& Value::operator*=(const Value& other) {
        *this = *this * other;
        return *this;
    }
    
    Value& Value::operator/=(const Value& other) {
        *this = *this / other;
        return *this;
    }
    
    Value& Value::operator%=(const Value& other) {
        *this = *this % other;
        return *this;
    }
    
    // Bitwise operators
    Value Value::operator^(const Value& other) const {
        if ((value_type == NUMBER || value_type == BV) && (other.value_type == NUMBER || other.value_type == BV)) {
            Number result = number_value ^ other.number_value;
            Value v(result);
            if (value_type == BV || other.value_type == BV) {
                v.value_type = BV;
                v.bv_width_ = (bv_width_ > 0) ? bv_width_ : other.bv_width_;
            }
            return v;
        }
        throw std::runtime_error("Bitwise XOR requires number or BV operands");
    }
    
    Value Value::operator&(const Value& other) const {
        if ((value_type == NUMBER || value_type == BV) && (other.value_type == NUMBER || other.value_type == BV)) {
            Number result = number_value & other.number_value;
            Value v(result);
            if (value_type == BV || other.value_type == BV) {
                v.value_type = BV;
                v.bv_width_ = (bv_width_ > 0) ? bv_width_ : other.bv_width_;
            }
            return v;
        }
        throw std::runtime_error("Bitwise AND requires number or BV operands");
    }
    
    Value Value::operator|(const Value& other) const {
        if ((value_type == NUMBER || value_type == BV) && (other.value_type == NUMBER || other.value_type == BV)) {
            Number result = number_value | other.number_value;
            Value v(result);
            if (value_type == BV || other.value_type == BV) {
                v.value_type = BV;
                v.bv_width_ = (bv_width_ > 0) ? bv_width_ : other.bv_width_;
            }
            return v;
        }
        throw std::runtime_error("Bitwise OR requires number or BV operands");
    }
    
    Value Value::operator<<(const Value& other) const {
        if ((value_type == NUMBER || value_type == BV) && (other.value_type == NUMBER || other.value_type == BV)) {
            unsigned long shift = other.number_value.floorToInteger().toULong();
            Number result = number_value << shift;
            Value v(result);
            v.value_type = value_type;
            v.bv_width_ = bv_width_;
            return v;
        }
        throw std::runtime_error("Left shift requires number or BV operands");
    }
    
    Value Value::operator>>(const Value& other) const {
        if ((value_type == NUMBER || value_type == BV) && (other.value_type == NUMBER || other.value_type == BV)) {
            unsigned long shift = other.number_value.floorToInteger().toULong();
            Number result = number_value >> shift;
            Value v(result);
            v.value_type = value_type;
            v.bv_width_ = bv_width_;
            return v;
        }
        throw std::runtime_error("Right shift requires number or BV operands");
    }
    
    // Increment and decrement
    Value& Value::operator++() {
        if (value_type != NUMBER && value_type != INTERVAL) {
            throw std::runtime_error("Increment requires a number or interval");
        }
        if (value_type == NUMBER) {
            number_value = number_value + Number(1);
        } else {
            interval_value = interval_value.operator++();
        }
        return *this;
    }
    
    Value& Value::operator--() {
        if (value_type != NUMBER && value_type != INTERVAL) {
            throw std::runtime_error("Decrement requires a number or interval");
        }
        if (value_type == NUMBER) {
            number_value = number_value - Number(1);
        } else {
            interval_value = interval_value.operator--();
        }
        return *this;
    }
    
    Value Value::operator++(int) {
        Value old = *this;
        ++(*this);
        return old;
    }
    
    Value Value::operator--(int) {
        Value old = *this;
        --(*this);
        return old;
    }
    
    // Unary operators
    Value Value::operator~() const {
        if (value_type == NUMBER || value_type == BV) {
            Number result = ~number_value;
            Value v(result);
            v.value_type = value_type;
            v.bv_width_ = bv_width_;
            return v;
        } else if (value_type == INTERVAL) {
            return Value(interval_value.operator~());
        } else {
            throw std::runtime_error("Bitwise NOT requires a number, BV, or interval");
        }
    }
    
    Value Value::neg() const {
        if (value_type == NUMBER) {
            return Value(-number_value);
        } else if (value_type == INTERVAL) {
            return Value(interval_value.negate());
        } else {
            throw std::runtime_error("Negation requires a number or interval");
        }
    }
    
    // Additional mathematical functions
    Value Value::safeSqrt() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.safeSqrt());
            case INTERVAL:
                return Value(interval_value.safeSqrt());
            default:
                throw std::runtime_error("Cannot compute safe square root of this type");
        }
    }
    
    Value Value::lg() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.lg());
            case INTERVAL:
                return Value(interval_value.lg());
            default:
                throw std::runtime_error("Cannot compute base-10 logarithm of this type");
        }
    }
    
    Value Value::lb() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.lb());
            case INTERVAL:
                return Value(interval_value.lb());
            default:
                throw std::runtime_error("Cannot compute base-2 logarithm of this type");
        }
    }
    
    Value Value::log(const Value& other) const {
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(number_value.log(other.number_value));
        } else {
            throw std::runtime_error("Logarithm with custom base not implemented for intervals");
        }
    }
    
    Value Value::ceil() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.ceil());
            case INTERVAL:
                // Interval ceil would need specific implementation if needed
                throw std::runtime_error("Ceiling function not implemented for intervals");
            default:
                throw std::runtime_error("Cannot compute ceiling of this type");
        }
    }
    
    Value Value::floor() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.floor());
            case INTERVAL:
                // Interval floor would need specific implementation if needed
                throw std::runtime_error("Floor function not implemented for intervals");
            default:
                throw std::runtime_error("Cannot compute floor of this type");
        }
    }
    
    Value Value::round() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.round());
            case INTERVAL:
                // Interval round would need specific implementation if needed
                throw std::runtime_error("Round function not implemented for intervals");
            default:
                throw std::runtime_error("Cannot round this type");
        }
    }

    // Trigonometric functions
    Value Value::cot() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.cot());
            case INTERVAL:
                return Value(interval_value.cot());
            default:
                throw std::runtime_error("Cannot compute cotangent of this type");
        }
    }
    
    Value Value::sec() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.sec());
            case INTERVAL:
                return Value(interval_value.sec());
            default:
                throw std::runtime_error("Cannot compute secant of this type");
        }
    }
    
    Value Value::csc() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.csc());
            case INTERVAL:
                return Value(interval_value.csc());
            default:
                throw std::runtime_error("Cannot compute cosecant of this type");
        }
    }
    
    // Inverse trigonometric functions
    Value Value::asin() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.asin());
            case INTERVAL:
                return Value(interval_value.asin());
            default:
                throw std::runtime_error("Cannot compute arcsine of this type");
        }
    }
    
    Value Value::acos() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acos());
            case INTERVAL:
                return Value(interval_value.acos());
            default:
                throw std::runtime_error("Cannot compute arccosine of this type");
        }
    }
    
    Value Value::atan() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.atan());
            case INTERVAL:
                return Value(interval_value.atan());
            default:
                throw std::runtime_error("Cannot compute arctangent of this type");
        }
    }
    
    Value Value::acot() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acot());
            case INTERVAL:
                return Value(interval_value.acot());
            default:
                throw std::runtime_error("Cannot compute arccotangent of this type");
        }
    }
    
    Value Value::asec() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.asec());
            case INTERVAL:
                return Value(interval_value.asec());
            default:
                throw std::runtime_error("Cannot compute arcsecant of this type");
        }
    }
    
    Value Value::acsc() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acsc());
            case INTERVAL:
                return Value(interval_value.acsc());
            default:
                throw std::runtime_error("Cannot compute arccosecant of this type");
        }
    }
    
    Value Value::atan2(const Value& other) const {
        if (value_type == NUMBER && other.value_type == NUMBER) {
            return Value(Number::atan2(number_value, other.number_value));
        } else {
            throw std::runtime_error("atan2 not implemented for intervals");
        }
    }
    
    // Hyperbolic functions
    Value Value::sinh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.sinh());
            case INTERVAL:
                return Value(interval_value.sinh());
            default:
                throw std::runtime_error("Cannot compute hyperbolic sine of this type");
        }
    }
    
    Value Value::cosh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.cosh());
            case INTERVAL:
                return Value(interval_value.cosh());
            default:
                throw std::runtime_error("Cannot compute hyperbolic cosine of this type");
        }
    }
    
    Value Value::tanh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.tanh());
            case INTERVAL:
                return Value(interval_value.tanh());
            default:
                throw std::runtime_error("Cannot compute hyperbolic tangent of this type");
        }
    }
    
    Value Value::coth() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.coth());
            case INTERVAL:
                return Value(interval_value.coth());
            default:
                throw std::runtime_error("Cannot compute hyperbolic cotangent of this type");
        }
    }
    
    Value Value::sech() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.sech());
            case INTERVAL:
                return Value(interval_value.sech());
            default:
                throw std::runtime_error("Cannot compute hyperbolic secant of this type");
        }
    }
    
    Value Value::csch() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.csch());
            case INTERVAL:
                return Value(interval_value.csch());
            default:
                throw std::runtime_error("Cannot compute hyperbolic cosecant of this type");
        }
    }
    
    // Inverse hyperbolic functions
    Value Value::asinh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.asinh());
            case INTERVAL:
                return Value(interval_value.asinh());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic sine of this type");
        }
    }
    
    Value Value::acosh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acosh());
            case INTERVAL:
                return Value(interval_value.acosh());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic cosine of this type");
        }
    }
    
    Value Value::atanh() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.atanh());
            case INTERVAL:
                return Value(interval_value.atanh());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic tangent of this type");
        }
    }
    
    Value Value::acoth() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acoth());
            case INTERVAL:
                return Value(interval_value.acoth());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic cotangent of this type");
        }
    }
    
    Value Value::asech() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.asech());
            case INTERVAL:
                return Value(interval_value.asech());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic secant of this type");
        }
    }
    
    Value Value::acsch() const {
        switch (value_type) {
            case NUMBER:
                return Value(number_value.acsch());
            case INTERVAL:
                return Value(interval_value.acsch());
            default:
                throw std::runtime_error("Cannot compute inverse hyperbolic cosecant of this type");
        }
    }

    // String operations
    Value Value::concatStr(const Value& other) const {
        if (value_type == STRING && other.value_type == STRING) {
            return Value(string_value + other.string_value);
        } else {
            throw std::runtime_error("String concatenation requires string operands");
        }
    }

    Value Value::repeatStr(const Value& other) const {
        if (value_type == STRING && other.value_type == NUMBER) {
            std::string result;
            int count = other.number_value.floorToInteger().toInt();
            for (int i = 0; i < count; i++) {
                result += string_value;
            }
            return Value(result);
        } else {
            throw std::runtime_error("String repeat requires a string and a number");
        }
    }

    Value Value::sortStr() const {
        if (value_type == STRING) {
            std::string result = string_value;
            std::sort(result.begin(), result.end());
            return Value(result);
        } else {
            throw std::runtime_error("Sort operation requires a string");
        }
    }

    // String operations (continued)
    Value Value::substr(const Value& start, const Value& end) const {
        if (value_type == STRING && start.value_type == NUMBER && end.value_type == NUMBER) {
            int startPos = start.number_value.floorToInteger().toInt();
            int length = end.number_value.floorToInteger().toInt();
            
            if (startPos < 0 || startPos >= static_cast<int>(string_value.length())) {
                throw std::out_of_range("String substring start position out of range");
            }
            
            return Value(string_value.substr(startPos, length));
        } else {
            throw std::runtime_error("Substring operation requires a string and two numbers");
        }
    }
    
    Value Value::replace(const Value& old, const Value& newVal) const {
        if (value_type == STRING && old.value_type == STRING && newVal.value_type == STRING) {
            std::string result = string_value;
            size_t pos = 0;
            while ((pos = result.find(old.string_value, pos)) != std::string::npos) {
                result.replace(pos, old.string_value.length(), newVal.string_value);
                pos += newVal.string_value.length();
            }
            return Value(result);
        } else {
            throw std::runtime_error("Replace operation requires three strings");
        }
    }
    
    Value Value::split(const Value& delimiter) const {
        (void)delimiter;
        throw std::runtime_error("String split not implemented");
    }
    
    Value Value::join(const Value& delimiter) const {
        (void)delimiter;
        throw std::runtime_error("String join not implemented");
    }
    
    Value Value::reverse() const {
        if (value_type == STRING) {
            std::string result = string_value;
            std::reverse(result.begin(), result.end());
            return Value(result);
        } else {
            throw std::runtime_error("Reverse operation requires a string");
        }
    }
    
    Value Value::unique() const {
        if (value_type == STRING) {
            std::string result;
            for (char c : string_value) {
                if (result.find(c) == std::string::npos) {
                    result += c;
                }
            }
            return Value(result);
        } else {
            throw std::runtime_error("Unique operation requires a string");
        }
    }
    
    Value Value::trim() const {
        if (value_type == STRING) {
            std::string result = string_value;
            // Trim left
            result.erase(result.begin(), std::find_if(result.begin(), result.end(), isNotSpaceInt));
            // Trim right
            result.erase(std::find_if(result.rbegin(), result.rend(), isNotSpaceInt).base(), result.end());
            return Value(result);
        } else {
            throw std::runtime_error("Trim operation requires a string");
        }
    }
    
    Value Value::ltrim() const {
        if (value_type == STRING) {
            std::string result = string_value;
            result.erase(result.begin(), std::find_if(result.begin(), result.end(), isNotSpaceInt));
            return Value(result);
        } else {
            throw std::runtime_error("Left trim operation requires a string");
        }
    }
    
    Value Value::rtrim() const {
        if (value_type == STRING) {
            std::string result = string_value;
            result.erase(std::find_if(result.rbegin(), result.rend(), isNotSpaceInt).base(), result.end());
            return Value(result);
        } else {
            throw std::runtime_error("Right trim operation requires a string");
        }
    }
    
    Value Value::toLower() const {
        if (value_type == STRING) {
            std::string result = string_value;
            std::transform(result.begin(), result.end(), result.begin(), toLowerChar);
            return Value(result);
        } else {
            throw std::runtime_error("toLower operation requires a string");
        }
    }
    
    Value Value::toUpper() const {
        if (value_type == STRING) {
            std::string result = string_value;
            std::transform(result.begin(), result.end(), result.begin(), toUpperChar);
            return Value(result);
        } else {
            throw std::runtime_error("toUpper operation requires a string");
        }
    }
    
    // Type conversion operations
    Value Value::toNumber() const {
        if (value_type == STRING) {
            try {
                return Value(Number(string_value));
            } catch (const std::exception& e) {
                throw std::runtime_error("Cannot convert string to number: " + std::string(e.what()));
            }
        } else if (value_type == BOOLEAN) {
            return Value(Number(boolean_value ? 1 : 0));
        } else if (value_type == NUMBER) {
            return *this;
        } else {
            throw std::runtime_error("Cannot convert this type to number");
        }
    }
    
    Value Value::toBoolean() const {
        if (value_type == STRING) {
            return Value(string_value == "true" || string_value == "1");
        } else if (value_type == NUMBER) {
            return Value(number_value != Number(0));
        } else if (value_type == BOOLEAN) {
            return *this;
        } else {
            throw std::runtime_error("Cannot convert this type to boolean");
        }
    }
    
    Value Value::toBV() const {
        if (value_type == BV) {
            return *this;
        } else if (value_type == NUMBER) {
            Value result(number_value);
            result.value_type = BV;
            result.bv_width_ = bv_width_;
            return result;
        } else {
            throw std::runtime_error("Cannot convert this type to bit-vector");
        }
    }
    
    Value Value::toFP() const {
        if (value_type == FP) {
            return *this;
        } else if (value_type == NUMBER) {
            // Convert number to FP string representation using default double precision
            Value result(ValueType::FP);
            double d = number_value.toReal(64).toDouble();
            // Store as IEEE 754 double-precision bit pattern
            uint64_t bits;
            std::memcpy(&bits, &d, sizeof(bits));
            uint64_t sign = (bits >> 63) & 1;
            uint64_t exp  = (bits >> 52) & 0x7FF;
            uint64_t sig  = bits & 0x000FFFFFFFFFFFFF;
            // Build SMT-LIB FP string
            std::ostringstream oss;
            oss << "(fp #b" << sign << " #b";
            for (int i = 10; i >= 0; --i)
                oss << ((exp >> i) & 1);
            oss << " #b";
            for (int i = 51; i >= 0; --i)
                oss << ((sig >> i) & 1);
            oss << ")";
            result.fp_smtlib_ = oss.str();
            result.fp_eb_ = 11;
            result.fp_sb_ = 53;
            return result;
        } else if (value_type == STRING) {
            // Try parsing string as a number first, then convert
            Value num = toNumber();
            return num.toFP();
        } else {
            throw std::runtime_error("Cannot convert this type to floating-point");
        }
    }
    
    Value Value::toArray() const {
        if (value_type == ARRAY) {
            return *this;
        } else {
            // Create single-element array with key "0"
            Value result(ValueType::ARRAY);
            result.array_data_ = std::make_shared<std::unordered_map<std::string, Value>>();
            (*result.array_data_)["0"] = *this;
            return result;
        }
    }
    
    // Other method implementations can be added as needed
    // For array operations, etc.

    // Bitwise assignment operators
    Value& Value::operator^=(const Value& other) {
        *this = *this ^ other;
        return *this;
    }
    
    Value& Value::operator&=(const Value& other) {
        *this = *this & other;
        return *this;
    }
    
    Value& Value::operator|=(const Value& other) {
        *this = *this | other;
        return *this;
    }
    
    Value& Value::operator<<=(const Value& other) {
        *this = *this << other;
        return *this;
    }
    
    Value& Value::operator>>=(const Value& other) {
        *this = *this >> other;
        return *this;
    }

    // ─── FP accessors ───────────────────────────────────────────────────
    void Value::setFpValue(const std::string& fp_str, uint32_t eb, uint32_t sb) {
        fp_smtlib_ = fp_str;
        fp_eb_ = eb;
        fp_sb_ = sb;
        value_type = FP;
    }

    std::string Value::getFpString() const {
        if (value_type != FP) {
            throw std::runtime_error("Value is not a floating-point");
        }
        return fp_smtlib_;
    }

    // ─── Array accessors ────────────────────────────────────────────────
    void Value::setArrayElement(const std::string& key, const Value& val) {
        if (!array_data_) {
            array_data_ = std::make_shared<std::unordered_map<std::string, Value>>();
        }
        (*array_data_)[key] = val;
        value_type = ARRAY;
    }

    void Value::setArrayDefault(const Value& val) {
        array_default_ = std::make_shared<Value>(val);
        value_type = ARRAY;
    }

    // ─── BV operators ───────────────────────────────────────────────────
    Value Value::notOp() const {
        return ~(*this);
    }

    Value Value::andOp(const Value& other) const {
        return *this & other;
    }

    Value Value::orOp(const Value& other) const {
        return *this | other;
    }

    Value Value::xorOp(const Value& other) const {
        return *this ^ other;
    }

    Value Value::impliesOp(const Value& other) const {
        // bitwise implies: ~a | b
        return notOp().orOp(other);
    }

    Value Value::concatBv(const Value& other) const {
        if ((value_type != BV && value_type != NUMBER) ||
            (other.value_type != BV && other.value_type != NUMBER)) {
            throw std::runtime_error("concatBv requires BV or NUMBER operands");
        }
        // Shift this value left by other's width, then OR
        uint32_t other_w = other.bv_width_;
        Number result = (number_value << other_w) | other.number_value;
        Value v(result);
        v.value_type = BV;
        v.bv_width_ = bv_width_ + other_w;
        return v;
    }

    Value Value::extract(uint32_t high, uint32_t low) const {
        if (value_type != BV && value_type != NUMBER) {
            throw std::runtime_error("extract requires BV or NUMBER operand");
        }
        uint32_t width = high - low + 1;
        // Shift right by low, then mask to width bits
        Number shifted = number_value >> low;
        Number mask = (Number(Integer(1)) << width) - Number(1);
        Number result = shifted & mask;
        Value v(result);
        v.value_type = BV;
        v.bv_width_ = width;
        return v;
    }

    Value Value::repeatBv(uint32_t n) const {
        if (value_type != BV && value_type != NUMBER) {
            throw std::runtime_error("repeatBv requires BV or NUMBER operand");
        }
        if (n == 0) {
            throw std::runtime_error("repeatBv count must be > 0");
        }
        Value result = *this;
        for (uint32_t i = 1; i < n; ++i) {
            result = result.concatBv(*this);
        }
        return result;
    }

    Value Value::rotate_left(const Value& other) const {
        if ((value_type != BV && value_type != NUMBER) ||
            (other.value_type != BV && other.value_type != NUMBER)) {
            throw std::runtime_error("rotate_left requires BV or NUMBER operands");
        }
        uint32_t w = bv_width_;
        if (w == 0) throw std::runtime_error("rotate_left: BV width is 0");
        unsigned long amount = other.number_value.floorToInteger().toULong() % w;
        if (amount == 0) return *this;
        // rotate left: (x << amount) | (x >> (w - amount)), masked to w bits
        Number mask = (Number(Integer(1)) << w) - Number(1);
        Number result = ((number_value << amount) | (number_value >> (w - amount))) & mask;
        Value v(result);
        v.value_type = BV;
        v.bv_width_ = w;
        return v;
    }

    Value Value::rotate_right(const Value& other) const {
        if ((value_type != BV && value_type != NUMBER) ||
            (other.value_type != BV && other.value_type != NUMBER)) {
            throw std::runtime_error("rotate_right requires BV or NUMBER operands");
        }
        uint32_t w = bv_width_;
        if (w == 0) throw std::runtime_error("rotate_right: BV width is 0");
        unsigned long amount = other.number_value.floorToInteger().toULong() % w;
        if (amount == 0) return *this;
        Number mask = (Number(Integer(1)) << w) - Number(1);
        Number result = ((number_value >> amount) | (number_value << (w - amount))) & mask;
        Value v(result);
        v.value_type = BV;
        v.bv_width_ = w;
        return v;
    }

    Value Value::shift_left(const Value& other) const {
        return *this << other;
    }

    Value Value::shift_right(const Value& other) const {
        return *this >> other;
    }

    // ─── FP operators ───────────────────────────────────────────────────
    Value Value::fadd(const Value& other) const {
        if (value_type != FP || other.value_type != FP) {
            throw std::runtime_error("fadd requires FP operands");
        }
        // Use native double arithmetic with default rounding
        double a = 0.0, b = 0.0;
        // Simple: parse from string representation if available
        // Use a rough numeric approach via toString
        try {
            a = std::stod(fp_smtlib_);
        } catch(...) { a = 0.0; }
        try {
            b = std::stod(other.fp_smtlib_);
        } catch(...) { b = 0.0; }
        double r = a + b;
        Value result{Number::fromApproxDouble(r)};
        return result.toFP();
    }

    Value Value::fsub(const Value& other) const {
        if (value_type != FP || other.value_type != FP) {
            throw std::runtime_error("fsub requires FP operands");
        }
        double a = 0.0, b = 0.0;
        try { a = std::stod(fp_smtlib_); } catch(...) { a = 0.0; }
        try { b = std::stod(other.fp_smtlib_); } catch(...) { b = 0.0; }
        double r = a - b;
        Value result{Number::fromApproxDouble(r)};
        return result.toFP();
    }

    Value Value::fmul(const Value& other) const {
        if (value_type != FP || other.value_type != FP) {
            throw std::runtime_error("fmul requires FP operands");
        }
        double a = 0.0, b = 0.0;
        try { a = std::stod(fp_smtlib_); } catch(...) { a = 0.0; }
        try { b = std::stod(other.fp_smtlib_); } catch(...) { b = 0.0; }
        double r = a * b;
        Value result{Number::fromApproxDouble(r)};
        return result.toFP();
    }

    Value Value::fdiv(const Value& other) const {
        if (value_type != FP || other.value_type != FP) {
            throw std::runtime_error("fdiv requires FP operands");
        }
        double a = 0.0, b = 0.0;
        try { a = std::stod(fp_smtlib_); } catch(...) { a = 0.0; }
        try { b = std::stod(other.fp_smtlib_); } catch(...) { b = 0.0; }
        double r = a / b;
        Value result{Number::fromApproxDouble(r)};
        return result.toFP();
    }

    Value Value::frem(const Value& other) const {
        if (value_type != FP || other.value_type != FP) {
            throw std::runtime_error("frem requires FP operands");
        }
        double a = 0.0, b = 0.0;
        try { a = std::stod(fp_smtlib_); } catch(...) { a = 0.0; }
        try { b = std::stod(other.fp_smtlib_); } catch(...) { b = 0.0; }
        double r = std::fmod(a, b);
        Value result{Number::fromApproxDouble(r)};
        return result.toFP();
    }

    // ─── Array operators ────────────────────────────────────────────────
    Value Value::select(const Value& index) const {
        if (value_type != ARRAY) {
            throw std::runtime_error("select requires an ARRAY operand");
        }
        std::string key = index.toString();
        if (array_data_) {
            auto it = array_data_->find(key);
            if (it != array_data_->end()) {
                return it->second;
            }
        }
        if (array_default_) {
            return *array_default_;
        }
        throw std::runtime_error("Array select: index not found and no default value");
    }

    Value Value::store(const Value& index, const Value& val) const {
        if (value_type != ARRAY) {
            throw std::runtime_error("store requires an ARRAY operand");
        }
        Value result(*this);
        // Copy-on-write: clone the map
        if (result.array_data_) {
            result.array_data_ = std::make_shared<std::unordered_map<std::string, Value>>(
                *result.array_data_);
        } else {
            result.array_data_ = std::make_shared<std::unordered_map<std::string, Value>>();
        }
        (*result.array_data_)[index.toString()] = val;
        return result;
    }
}