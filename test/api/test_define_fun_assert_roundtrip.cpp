// -> issue #36
#include "somtparser/frontend/parser.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include "test_helpers.h"

int main() {
    const char* tmp_file = "define_fun_assert_roundtrip.smt2";
    const char* smt2_content = R"(
(set-logic QF_S)
(define-fun eq ((x String) (y String)) Bool (= x y))
(declare-fun x () String)
(declare-fun u () String)
(assert (eq x u))
(check-sat)
(exit)
)";

    std::ofstream out(tmp_file);
    VERIFY(out && "cannot create temp smt2 file");
    out << smt2_content;
    out.close();

    auto parser = SOMTParser::newParser();
    bool ok = parser->parse(tmp_file);
    std::remove(tmp_file);

    VERIFY(ok && "parse should succeed");
    std::string dumped = parser->dumpSMT2();
    std::cout << dumped;

    VERIFY(dumped.find("(assert (eq x u))") != std::string::npos &&
           "dumpSMT2 must output (assert (eq x u)) with both arguments");
    VERIFY(dumped.find("(define-fun eq ") != std::string::npos &&
           "dump must contain define-fun eq");
    VERIFY((dumped.find("(= x y)") != std::string::npos ||
            dumped.find("(= y x)") != std::string::npos) &&
           "define-fun eq body must preserve both parameters");

    std::cout << "define_fun_assert_roundtrip: parse + dump preserves (eq x u) and (= x y) OK" << std::endl;
    return 0;
}
