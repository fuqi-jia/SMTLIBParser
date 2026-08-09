#include <iostream>
#include <string>
#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

int main() {
    std::cout << "======= Parser Error Test =======" << std::endl;

    // Test 1: error_kind_mismatch — fixed, should parse successfully
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        parser->setOption("keep_let", false);
        parser->setOption("preserve_let", false);
        bool ok = parser->parseStr(R"(
(set-logic QF_S)
(set-option :produce-models true)


(declare-fun s  () String)
(declare-fun filename_0  () String)
(declare-fun filename_1  () String)
(declare-fun filename_2  () String)
(declare-fun i1 () Int)
(declare-fun i2 () Int)
(declare-fun i3 () Int)
(declare-fun tmpStr0 () String)
(declare-fun tmpStr1 () String)
(declare-fun tmpStr2 () String)



(assert (= filename_0 s) )

; i1 = LastIndexof(filename_0, "/")
; --------------------------------------------------------------------
(assert (ite (str.contains filename_0 "/")
             (and (= filename_0 (str.++ tmpStr0 (str.++ "/" tmpStr1) ) )
                  (not (str.contains tmpStr1 "/") )
                  (= i1 (str.len tmpStr0) )
             )
             (= i1 (- 0 1) )
        )
)


(assert (ite (not (= i1 (- 0 1) ) )
             (and (= i2 (- (str.len filename_0) i1) )
                  (= filename_1 (str.substr filename_0 i1 i2) )
             )
             (= filename_1 filename_0)
        )
)

(assert (= i3 (str.indexof filename_1 "." 0) ) )


(assert (ite (not (= i3 (- 0 1) ) )
             (= filename_2 (str.substr filename_1 0 i3) )
             (= filename_2 filename_1)
        )
)



(check-sat)
(get-model)
)");
        VERIFY(ok && "error_kind_mismatch should parse");
        std::cout << "Test 1 passed: error_kind_mismatch\n";
    }

    // Test 2: error_unknown_symbol — contains :named; parser supports :named so expect success
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        parser->setOption("keep_let", false);
        parser->setOption("preserve_let", false);
        bool ok = parser->parseStr(R"(
(set-option :produce-unsat-cores true)
(declare-fun A () Bool)
(declare-fun B () Bool)
(assert (! A :named AA))
(assert (! (not B) :named BB))
(assert (= A B))
(assert A :id A)
(check-sat)
(get-unsat-core)
(exit)
)");
        VERIFY(ok && "error_unknown_symbol should parse");
        std::cout << "Test 2 passed: error_unknown_symbol\n";
    }

    // Test 3: error_unknown_symbol_2 — contains (declare-sort S 1) etc.
    {
        SOMTParser::ParserPtr parser = SOMTParser::newParser();
        parser->setOption("keep_let", false);
        parser->setOption("preserve_let", false);
        bool ok = parser->parseStr(R"(
(declare-sort S 1)
(define-sort SB () (S Bool))
(declare-fun A () (S Bool))
(declare-fun B () SB)
(assert (= A B))
(check-sat)
(exit)
)");
        VERIFY(ok && "error_unknown_symbol_2 should parse");
        std::cout << "Test 3 passed: error_unknown_symbol_2\n";
    }

    std::cout << "\n======= Test Complete =======" << std::endl;
    return 0;
}
