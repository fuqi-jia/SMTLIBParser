#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

// Test basic quantifier operations (forall and exists)
void test_basic_quantifiers(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(forall ((x Int)) (> x 0))",                            // Simple forall
        "(exists ((x Int)) (= x 0))",                            // Simple exists
        "(forall ((x Int) (y Int)) (=> (> x 0) (>= (+ x y) y)))", // Multiple variables in forall
        "(exists ((x Int) (y Int)) (and (> x 0) (< y 0)))",      // Multiple variables in exists
        "(exists ((x Bool)) (and x (not x)))",                   // Unsatisfiable existential
        "(forall ((x Bool)) (or x (not x)))"                     // Valid universal
    };
    
    std::cout << "=== Testing Basic Quantifiers ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        VERIFY(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test nested quantifiers
void test_nested_quantifiers(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(forall ((x Int)) (exists ((y Int)) (= y (* 2 x))))",   // forall-exists
        "(exists ((x Int)) (forall ((y Int)) (>= x y)))",        // exists-forall
        "(forall ((x Int)) (forall ((y Int)) (>= (+ x y) 0)))",  // forall-forall
        "(exists ((x Int)) (exists ((y Int)) (= (+ x y) 0)))",   // exists-exists
        "(forall ((x Int)) (exists ((y Int)) (forall ((z Int)) (= (+ x y) z))))" // Three levels
    };
    
    std::cout << "=== Testing Nested Quantifiers ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        VERIFY(result && !result->isErr());
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// Test quantifiers with arrays
void test_quantifiers_with_arrays(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(forall ((i Int)) (=> (and (>= i 0) (< i 10)) (= (select a i) 0)))",  // All elements in range are 0
        "(exists ((i Int)) (= (select a i) 1))",                               // Some element is 1
        "(forall ((i Int) (j Int)) (=> (= i j) (= (select a i) (select a j))))", // Array congruence
        "(forall ((i Int)) (= (select (store a i 5) i) 5))",                   // Select-after-store axiom
        "(forall ((i Int) (j Int)) (=> (not (= i j)) (= (select (store a i 5) j) (select a j))))" // Select-after-store axiom
    };
    
    std::cout << "=== Testing Quantifiers with Arrays ===" << std::endl;
    
    try {
        // First declare the array
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        std::shared_ptr<SOMTParser::DAGNode> array = parser->mkArray("a", int_sort, int_sort);
        std::cout << "Declared array: " << parser->toString(array) << std::endl;
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            VERIFY(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during array declaration: " << e.what() << std::endl;
    }
}

// Test manual creation of quantifier variables and formulas
void test_manual_quantifier_creation(SOMTParser::ParserPtr& parser) {
    std::cout << "=== Testing Manual Quantifier Creation ===" << std::endl;
    
    try {
        // Create sorts
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        std::shared_ptr<SOMTParser::Sort> bool_sort = SOMTParser::SortManager::BOOL_SORT;
        
        // Create quantifier variables
        std::shared_ptr<SOMTParser::DAGNode> x_var = parser->mkQuantVar("x", int_sort);
        std::shared_ptr<SOMTParser::DAGNode> y_var = parser->mkQuantVar("y", int_sort);
        
        // Create a formula: x > y
        std::shared_ptr<SOMTParser::DAGNode> x_gt_y = parser->mkGt(x_var, y_var);
        
        // Create a forall formula: forall x,y. x > y
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> forall_params = {x_var, y_var, x_gt_y};
        std::shared_ptr<SOMTParser::DAGNode> forall_formula = parser->mkForall(forall_params);
        
        VERIFY(forall_formula && parser->toString(forall_formula).find("forall") != std::string::npos);
        std::cout << "Manually created forall formula: " << parser->toString(forall_formula) << std::endl;
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> exists_params = {x_var, y_var, x_gt_y};
        std::shared_ptr<SOMTParser::DAGNode> exists_formula = parser->mkExists(exists_params);
        VERIFY(exists_formula && parser->toString(exists_formula).find("exists") != std::string::npos);
        std::cout << "Manually created exists formula: " << parser->toString(exists_formula) << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

// Test real-world quantifier examples
void test_practical_quantifier_examples(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Injectivity property
        "(forall ((x Int) (y Int)) (=> (= (f x) (f y)) (= x y)))",
        
        // Surjectivity property
        "(forall ((y Int)) (exists ((x Int)) (= (f x) y)))",
        
        // Transitivity of a relation
        "(forall ((x Int) (y Int) (z Int)) (=> (and (R x y) (R y z)) (R x z)))",
        
        // All elements in array are non-negative
        "(forall ((i Int)) (=> (and (>= i 0) (< i n)) (>= (select arr i) 0)))",
        
        // Sortedness of an array
        "(forall ((i Int) (j Int)) (=> (and (>= i 0) (< i n) (>= j 0) (< j n) (< i j)) (<= (select arr i) (select arr j))))"
    };
    
    std::cout << "=== Testing Practical Quantifier Examples ===" << std::endl;
    
    try {
        // Declare some necessary symbols for the examples
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        
        // Function f: Int -> Int
        std::vector<std::shared_ptr<SOMTParser::Sort>> f_params = {int_sort};
        parser->mkFuncDec("f", f_params, int_sort);
        
        // Relation R: Int, Int -> Bool
        std::shared_ptr<SOMTParser::Sort> bool_sort = SOMTParser::SortManager::BOOL_SORT;
        std::vector<std::shared_ptr<SOMTParser::Sort>> r_params = {int_sort, int_sort};
        parser->mkFuncDec("R", r_params, bool_sort);
        
        // Variable n: Int
        parser->mkVarInt("n");
        
        // Array arr: Int -> Int
        parser->mkArray("arr", int_sort, int_sort);
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            VERIFY(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "======= Quantifiers Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_basic_quantifiers(parser);
    test_nested_quantifiers(parser);
    test_quantifiers_with_arrays(parser);
    test_manual_quantifier_creation(parser);
    test_practical_quantifier_examples(parser);
    
    return 0;
} 