#include "somtparser/frontend/parser.h"
#include <iostream>
#include <string>
#include <cassert>

int main() {
    // Create parser
    auto parser = SOMTParser::newParser();
    
    // SMT-LIB model strings for testing
    std::string model_str1 = R"(
(model
  (define-fun v17 () Real
    0.0)
  (define-fun v15 () Real
    (- 1.0))
  (define-fun v7 () Real
    1.0)
  (define-fun v12 () Real
    (- 1.0))
  (define-fun v1 () Real
    1.0)
  (define-fun v13 () Real
    (- 1.0))
  (define-fun v6 () Real
    (- (/ 1.0 4.0)))
  (define-fun v2 () Real
    1.0)
  (define-fun v16 () Real
    1.0)
  (define-fun v18 () Real
    (- 1.0))
  (define-fun v10 () Real
    (/ 1.0 2.0))
  (define-fun v3 () Real
    0.0)
  (define-fun v14 () Real
    (- 8.0))
  (define-fun v9 () Real
    1.0)
  (define-fun v11 () Real
    1.0)
  (define-fun v4 () Real
    (- (/ 1.0 4.0)))
  (define-fun v5 () Real
    0.0)
  (define-fun v8 () Real
    0.0)
  (define-fun hypothesis () Bool
    (let ((a!1 (+ (* v10 (* v2 v2) v5) (* (- 1.0) v1 v10 v2 v7) (* (* v2 v2) v7 v9))))
  (and (> v4 (- 1.0))
       (> v7 0.0)
       (> a!1 0.0)
       (< v4 0.0)
       (< v6 0.0)
       (< a!1 (* v2 v2)))))
  (define-fun assumptions () Bool
    (let ((a!1 (= (+ (* v14 v18 (* v2 v2 v2) v4) (* v14 v17 (* v2 v2 v2) v7))
              (+ (* v16 v17 (* v2 v2 v2) v4)
                 (* (- 1.0) v1 v12 (* v16 v16) v2 v5)
                 (* (* v1 v1) v12 (* v16 v16) v7)
                 (* v15 v16 (* v2 v2 v2) v7))))
      (a!2 (= (+ v2
                 (* v2 v4)
                 (* (- 1.0) v2 v5)
                 (* (- 1.0) v10 v2 v5)
                 (* v2 v6)
                 (* v1 v10 v7)
                 (* (- 1.0) v2 v7 v9))
              0.0))
      (a!3 (= (+ (* v18 v4)
                 (* 2.0 v18 v3 v4)
                 (* v18 (* v3 v3) v4)
                 (* v17 v7)
                 (* 2.0 v17 v3 v7)
                 (* v17 (* v3 v3) v7)
                 (* v11 v8))
              (+ (* v13 v6) (* v13 v3 v6)))))
  (and (> (* v14 v18) (* v16 v17))
       (> (* v14 v17) (* v15 v16))
       (> (* v2 v9) 0.0)
       (> (* v2 v2 v9) (* v1 v10 v2))
       (< (* (* v1 v1) v12 v2) 0.0)
       (> v10 0.0)
       (< (* v12 v2) 0.0)
       (> v1 0.0)
       (> v2 0.0)
       (> v11 0.0)
       (< v13 0.0)
       (> v3 (- 1.0))
       (> v16 0.0)
       (< v14 0.0)
       (< v18 0.0)
       (< v15 0.0)
       (> (* v15 v18) (* v17 v17))
       a!1
       a!2
       a!3
       (= v8 0.0)
       (= v5 0.0))))
  (define-fun /0 ((x!0 Real) (x!1 Real)) Real
    (ite (and (= x!0 (- (/ 1.0 4.0))) (= x!1 1.0)) (- (/ 1.0 4.0))
      0.0))
)
)";

    std::string model_str2 = R"(
    (
  (define-fun | | () (_ BitVec 4)
    #xc)
  (define-fun v1 () (_ BitVec 4)
    #x0)
  (define-fun ?v0 () (_ BitVec 4)
    #x1)
  (define-fun notes () (_ BitVec 4)
    #x7)
  (define-fun V0 () (_ BitVec 4)
    #x5)
  (define-fun ~!@$%^&*_-+=><.?/ () (_ BitVec 4)
    #xa)
  (define-fun |~!@$%^&*_-+=<>.?/()| () (_ BitVec 4)
    #xe)
  (define-fun v0 () (_ BitVec 4)
    #xf)
  (define-fun  () (_ BitVec 4)
    #x3)
  (define-fun ~!@$%^&*_-+=<>.?/ () (_ BitVec 4)
    #xf)
)
    )";

  // Simplified version to reproduce the as-array issue
  std::string model_str3_simple = R"(
  (
  (define-fun testVar () (Array Int Int)
    (_ as-array k!52))
  (define-fun k!52 ((x!0 Int)) Int
    (ite (= x!0 1) 10 20))
  )
  )";

  std::string model_str3_original = R"(
  (
  (define-fun |c_#memory_$Pointer$.offset_primed| () (Array Int (Array Int Int))
    (let ((a!1 (store (store ((as const (Array Int (Array Int Int)))
                           ((as const (Array Int Int)) 171))
                         42
                         (_ as-array k!52))
                  46
                  (_ as-array k!54))))
  (store (store a!1 54 (_ as-array k!50)) 52 (_ as-array k!49))))
  (define-fun |c_~#__CS_thread_allocated~0.offset| () Int
    4094277)
  (define-fun |c_~#full~0.base| () Int
    46)
  (define-fun c_~__CS_ret~0 () Int
    0)
  (define-fun c_~__CS_thread~0.offset () (Array Int Int)
    ((as const (Array Int Int)) 0))
  (define-fun |#funAddr~thread1.base| () Int
    (- 1))
  (define-fun k!52 ((x!0 Int)) Int
    (ite (= x!0 82) 312
    (ite (= x!0 4094299) 48
    (ite (= x!0 4094277) 249
      172))))
  (define-fun k!54 ((x!0 Int)) Int
    (ite (= x!0 272) 273
    (ite (= x!0 4094311) 302
      170)))
  (define-fun k!50 ((x!0 Int)) Int
    (ite (= x!0 100) 254
    (ite (= x!0 4094275) 222
      173)))
  (define-fun k!49 ((x!0 Int)) Int
    (ite (= x!0 4091003) 279
    (ite (= x!0 82) 313
      175)))
)
  )";

    // Test case from smtrat: symbols starting with invalid characters like '('
    // These symbols need to be wrapped with |...| in define-fun/declare-fun declarations
    std::string model_str4_smtrat = R"(
(model 
        (define-fun (38,19)!131 () Real
                1
        )
        (define-fun (72,19)!72 () Real
                1
        )
        (define-fun (70,19)!73 () Real
                1
        )
        (define-fun (63,19)!117 () Real
                0
        )
)
)";

    // Basic interface: parse model from string then evaluate(phi, M)
    {
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        auto phi = parser->mkExpr("(and (> x 0) (> y 0))");
        assert(phi);
        std::string modelStr = R"(
(model
  (define-fun x () Int 1)
  (define-fun y () Int 2)
))";
        auto M = parser->parseModel(modelStr);
        assert(M && M->size() >= 2);
        auto psi = parser->evaluate(phi, M);
        assert(psi && psi->isTrue() && "evaluate((and (> x 0) (> y 0)), M) with x=1,y=2 should be true");
        std::cout << "Basic parseModel + evaluate: phi=(and (> x 0) (> y 0)), M={x->1,y->2} => " << parser->toString(psi) << " OK" << std::endl;
    }

    try {
        auto model = parser->parseModel(model_str1);
        assert(model && "model1 should parse successfully");
        std::cout << "Model 1 parsed successfully!" << std::endl;
        std::cout << "Model 1 size: " << model->size() << std::endl;
        assert(model->size() > 0);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
        assert(pairs.size() == model->size());
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        assert(false && "model1 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str2);
        assert(model && "model2 should parse successfully");
        std::cout << "Model 2 parsed successfully!" << std::endl;
        std::cout << "Model 2 size: " << model->size() << std::endl;
        assert(model->size() > 0);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        assert(false && "model2 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str3_simple);
        assert(model && "model3 (simple as-array) should parse successfully");
        std::cout << "Model 3 (simplified) parsed successfully!" << std::endl;
        std::cout << "Model 3 size: " << model->size() << std::endl;
        assert(model->size() >= 1);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        assert(false && "model3 should not throw");
    }

    try {
        auto model = parser->parseModel(model_str4_smtrat);
        assert(model && "model4 (smtrat) should parse successfully");
        std::cout << "Model 4 (smtrat) parsed successfully!" << std::endl;
        std::cout << "Model 4 size: " << model->size() << std::endl;
        assert(model->size() >= 3);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        assert(false && "model4 should not throw");
    }

    try {
        // Parse model 5 - CVC5 real_algebraic_number format test
        std::string model_str5_cvc5 = R"(
(model
  (define-fun x () Real
    (_ real_algebraic_number <(+ (* 1 (^ x 2)) (- 3)), ((/ 3 2), (/ 7 4))>))
)";
        
        auto model = parser->parseModel(model_str5_cvc5);
        
        assert(model && "model5 (CVC5 real_algebraic_number) should parse successfully");
        std::cout << "Model 5 (CVC5 real_algebraic_number) parsed successfully!" << std::endl;
        std::cout << "Model 5 size: " << model->size() << std::endl;
        assert(model->size() == 1);
        auto pairs = model->getPairs();
        for (const auto& pair : pairs) {
            std::cout << "Variable: " << pair.first << " = " << parser->toString(pair.second) << std::endl;
            if (pair.second->isCRealAlgebraicNumber()) {
                std::cout << "  -> This is a real_algebraic_number node" << std::endl;
                std::cout << "  -> Polynomial: " << parser->toString(pair.second->getChild(0)) << std::endl;
                std::cout << "  -> Lower bound: " << parser->toString(pair.second->getChild(1)) << std::endl;
                std::cout << "  -> Upper bound: " << parser->toString(pair.second->getChild(2)) << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during parsing: " << e.what() << std::endl;
        assert(false && "model5 should not throw");
    }
    return 0;
}
