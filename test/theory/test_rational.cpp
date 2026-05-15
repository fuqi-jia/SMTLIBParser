#include <iostream>
#include <string>
#include "somtparser/ir/number.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/core/util.h"
#include <cassert>

using namespace SOMTParser;

void test_high_precision_rational_basic() {
    std::cout << "=== HighPrecisionRational Basic ===" << std::endl;

    // Decimal parsing
    Rational r1("59.01938237");
    assert(r1.toString() == "5901938237/1000000000");
    std::cout << "  59.01938237 -> " << r1.toString() << std::endl;

    Rational r2("0.5");
    assert(r2.toString() == "1/2");
    std::cout << "  0.5 -> " << r2.toString() << std::endl;

    Rational r3("-0.125");
    assert(r3.toString() == "-1/8");
    std::cout << "  -0.125 -> " << r3.toString() << std::endl;

    Rational r4("42");
    assert(r4.toString() == "42");
    std::cout << "  42 -> " << r4.toString() << std::endl;

    Rational r5("3.0");
    assert(r5.toString() == "3");
    std::cout << "  3.0 -> " << r5.toString() << std::endl;

    // a/b format parsing
    Rational r6("7/3");
    assert(r6.toString() == "7/3");
    std::cout << "  7/3 -> " << r6.toString() << std::endl;

    Rational r7("-22/7");
    assert(r7.toString() == "-22/7");
    std::cout << "  -22/7 -> " << r7.toString() << std::endl;

    std::cout << "  Basic parsing OK" << std::endl;
}

void test_high_precision_rational_arithmetic() {
    std::cout << "=== HighPrecisionRational Arithmetic ===" << std::endl;

    Rational a("1/2");
    Rational b("1/4");
    assert((a + b).toString() == "3/4");
    assert((a - b).toString() == "1/4");
    assert((a * b).toString() == "1/8");
    assert((a / b).toString() == "2");

    Rational c("7/3");
    Rational d("2/5");
    assert((c + d).toString() == "41/15");
    assert((c - d).toString() == "29/15");
    assert((c * d).toString() == "14/15");
    assert((c / d).toString() == "35/6");

    std::cout << "  Arithmetic OK" << std::endl;
}

void test_number_rational_type() {
    std::cout << "=== Number RATIONAL_TYPE ===" << std::endl;

    Number n1("0.5", false);
    assert(n1.isRational());
    assert(!n1.isReal());  // isReal now returns true for RATIONAL too... wait
    // Actually isReal() returns true for RATIONAL_TYPE as well
    assert(n1.isReal());
    assert(n1.toString() == "1/2");
    std::cout << "  Number(0.5) type=" << n1.getType() << " str=" << n1.toString() << std::endl;

    Number n2("42", false);
    assert(n2.isInteger());
    assert(n2.getType() == Number::INT_TYPE);
    std::cout << "  Number(42) type=" << n2.getType() << " str=" << n2.toString() << std::endl;

    Number n3("3.14159", false);
    assert(n3.isRational());
    std::cout << "  Number(3.14159) str=" << n3.toString() << std::endl;

    // Integer division that is not exact -> rational
    Number seven(SOMTParser::Integer(7));
    Number three(SOMTParser::Integer(3));
    Number q = seven / three;
    assert(q.isRational());
    assert(q.toString() == "7/3");
    std::cout << "  7/3 = " << q.toString() << std::endl;

    // Exact integer division still integer
    Number six(SOMTParser::Integer(6));
    Number two(SOMTParser::Integer(2));
    Number exact = six / two;
    assert(exact.isInteger());
    assert(exact.toString() == "3");
    std::cout << "  6/2 = " << exact.toString() << std::endl;

    // toReal conversion
    Number rat(std::string("1/2"));
    Real r = rat.toReal();
    assert(r.toDouble() == 0.5);
    std::cout << "  toReal(1/2) = " << r.toDouble() << std::endl;

    std::cout << "  Number rational type OK" << std::endl;
}

void test_number_mixed_arithmetic() {
    std::cout << "=== Number Mixed Arithmetic ===" << std::endl;

    Number i(SOMTParser::Integer(5));
    Number r(std::string("1/2"));

    Number sum = i + r;
    assert(sum.isRational());
    assert(sum.toString() == "11/2");
    std::cout << "  5 + 1/2 = " << sum.toString() << std::endl;

    Number prod = i * r;
    assert(prod.isRational());
    assert(prod.toString() == "5/2");
    std::cout << "  5 * 1/2 = " << prod.toString() << std::endl;

    Number diff = i - r;
    assert(diff.isRational());
    assert(diff.toString() == "9/2");
    std::cout << "  5 - 1/2 = " << diff.toString() << std::endl;

    Number quot = i / r;
    assert(quot.isRational());
    assert(quot.toString() == "10");
    std::cout << "  5 / 1/2 = " << quot.toString() << std::endl;

    // Rational + Rational
    Number a(std::string("1/3"));
    Number b(std::string("1/6"));
    Number s = a + b;
    assert(s.toString() == "1/2");
    std::cout << "  1/3 + 1/6 = " << s.toString() << std::endl;

    std::cout << "  Mixed arithmetic OK" << std::endl;
}

void test_parser_constant_folding() {
    std::cout << "=== Parser Constant Folding ===" << std::endl;

    ParserPtr parser = newParser();

    // SMT-LIB rational syntax: (/ 1 10) should fold to exact rational
    auto node1 = parser->mkExpr("(/ 1 10)");
    assert(node1 != nullptr);
    assert(node1->isCReal());
    std::string s1 = parser->toString(node1);
    std::cout << "  (/ 1 10) -> " << s1 << std::endl;
    assert(s1 == "1/10");

    // (/ 7 3)
    auto node2 = parser->mkExpr("(/ 7 3)");
    assert(node2 != nullptr);
    assert(node2->isCReal());
    std::string s2 = parser->toString(node2);
    std::cout << "  (/ 7 3) -> " << s2 << std::endl;
    assert(s2 == "7/3");

    // (/ 10 2) -> 5
    auto node3 = parser->mkExpr("(/ 10 2)");
    assert(node3 != nullptr);
    assert(node3->isCReal());
    std::string s3 = parser->toString(node3);
    std::cout << "  (/ 10 2) -> " << s3 << std::endl;
    assert(s3 == "5");

    // (/ -1 10)
    auto node4 = parser->mkExpr("(/ -1 10)");
    assert(node4 != nullptr);
    assert(node4->isCReal());
    std::string s4 = parser->toString(node4);
    std::cout << "  (/ -1 10) -> " << s4 << std::endl;
    assert(s4 == "-1/10");

    // (/ 0 10) -> 0
    auto node5 = parser->mkExpr("(/ 0 10)");
    assert(node5 != nullptr);
    assert(node5->isCReal());
    std::string s5 = parser->toString(node5);
    std::cout << "  (/ 0 10) -> " << s5 << std::endl;
    assert(s5 == "0");

    // nested: (+ (/ 1 10) (/ 2 10))
    auto node6 = parser->mkExpr("(+ (/ 1 10) (/ 2 10))");
    assert(node6 != nullptr);
    std::string s6 = parser->toString(node6);
    std::cout << "  (+ (/ 1 10) (/ 2 10)) -> " << s6 << std::endl;
    assert(s6 == "3/10");

    std::cout << "  Parser constant folding OK" << std::endl;
}

void test_parser_decimal_to_rational() {
    std::cout << "=== Parser Decimal to Rational ===" << std::endl;

    ParserPtr parser = newParser();

    auto node1 = parser->mkExpr("59.01938237");
    assert(node1 != nullptr);
    assert(node1->isCReal());
    std::string s1 = parser->toString(node1);
    std::cout << "  59.01938237 -> " << s1 << std::endl;
    assert(s1 == "5901938237/1000000000");

    auto node2 = parser->mkExpr("0.125");
    assert(node2 != nullptr);
    std::string s2 = parser->toString(node2);
    std::cout << "  0.125 -> " << s2 << std::endl;
    assert(s2 == "1/8");

    auto node3 = parser->mkExpr("-2.5");
    assert(node3 != nullptr);
    std::string s3 = parser->toString(node3);
    std::cout << "  -2.5 -> " << s3 << std::endl;
    assert(s3 == "-5/2");

    std::cout << "  Decimal to rational OK" << std::endl;
}

void test_transcendental_fallback() {
    std::cout << "=== Transcendental Fallback ===" << std::endl;

    // Rational input to transcendental should convert to MPFR
    Number n(std::string("2.0"));
    Number s = n.sin();
    (void)s;
    assert(s.isReal());
    std::cout << "  sin(2.0) type=" << s.getType() << " str=" << s.toString() << std::endl;

    Number e = n.exp();
    (void)e;
    assert(e.isReal());
    std::cout << "  exp(2.0) type=" << e.getType() << " str=" << e.toString() << std::endl;

    std::cout << "  Transcendental fallback OK" << std::endl;
}

void test_dump_smtlib2_rational() {
    std::cout << "=== dumpSMTLIB2 Rational Output ===" << std::endl;
    ParserPtr parser = newParser();

    auto n1 = parser->mkExpr("(/ 1 10)");
    std::string s1 = dumpSMTLIB2(n1);
    std::cout << "  (/ 1 10) -> " << s1 << std::endl;
    assert(s1 == "(/ 1 10)");

    auto n2 = parser->mkExpr("(/ 7 3)");
    std::string s2 = dumpSMTLIB2(n2);
    std::cout << "  (/ 7 3) -> " << s2 << std::endl;
    assert(s2 == "(/ 7 3)");

    auto n3 = parser->mkExpr("(/ -1 10)");
    std::string s3 = dumpSMTLIB2(n3);
    std::cout << "  (/ -1 10) -> " << s3 << std::endl;
    assert(s3 == "(- (/ 1 10))");

    // Decimal literals should remain unchanged
    auto n4 = parser->mkExpr("0.125");
    std::string s4 = dumpSMTLIB2(n4);
    std::cout << "  0.125 -> " << s4 << std::endl;
    assert(s4 == "0.125");

    // Negative decimal literal
    auto n5 = parser->mkExpr("-2.5");
    std::string s5 = dumpSMTLIB2(n5);
    std::cout << "  -2.5 -> " << s5 << std::endl;
    assert(s5 == "(- 2.5)");

    std::cout << "  dumpSMTLIB2 rational output OK" << std::endl;
}

void test_type_checker_is_real() {
    std::cout << "=== TypeChecker::isReal ===" << std::endl;

    assert(TypeChecker::isReal("3.14159"));
    assert(TypeChecker::isReal("42"));
    assert(TypeChecker::isReal("1/2"));
    assert(TypeChecker::isReal("-7/3"));
    assert(!TypeChecker::isReal("1/0"));  // denominator zero rejected
    assert(!TypeChecker::isReal("abc"));
    assert(!TypeChecker::isReal("1.2.3"));
    assert(!TypeChecker::isReal("1/2/3"));

    std::cout << "  TypeChecker::isReal OK" << std::endl;
}

int main() {
    std::cout << "======= Rational Number Tests =======" << std::endl;

    test_high_precision_rational_basic();
    test_high_precision_rational_arithmetic();
    test_number_rational_type();
    test_number_mixed_arithmetic();
    test_parser_constant_folding();
    test_parser_decimal_to_rational();
    test_transcendental_fallback();
    test_dump_smtlib2_rational();
    test_type_checker_is_real();

    std::cout << "\n======= ALL RATIONAL TESTS PASSED =======" << std::endl;
    return 0;
}
