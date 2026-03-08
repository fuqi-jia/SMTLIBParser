#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

// Test combination of arithmetic and boolean logic
void test_arithmetic_boolean_combination(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Arithmetic in boolean context
        "(and (> ab_x 0) (< ab_y 10))",
        
        // Boolean operators with arithmetic comparisons
        "(or (= ab_x ab_y) (and (> ab_x 0) (< ab_y 0)))",
        
        // Implication with arithmetic
        "(=> (>= ab_x 0) (>= (+ ab_x ab_y) ab_y))",
        
        // Ite mixing boolean and arithmetic
        "(ite (> ab_x 0) (+ ab_x ab_y) (- ab_x ab_y))",
        
        // Complex expression mixing theories
        "(and (= (+ ab_x ab_y) ab_z) (or (> ab_x 0) (< ab_y 0)))"
    };
    
    std::cout << "=== Testing Arithmetic and Boolean Logic Combination ===" << std::endl;
    
    try {
        // Declare variables
        parser->mkVarInt("ab_x");
        parser->mkVarInt("ab_y");
        parser->mkVarInt("ab_z");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during variable declaration: " << e.what() << std::endl;
    }
}

// Test combination of arrays and arithmetic
void test_array_arithmetic_combination(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Array access with arithmetic indices
        "(select aa_a (+ aa_i 1))",
        
        // Store with arithmetic expressions
        "(store aa_a (* 2 aa_i) (+ aa_j 5))",
        
        // Array equality with stores and arithmetic
        "(= (store aa_a aa_i (+ aa_j 1)) (store aa_a aa_i (+ 1 aa_j)))",
        
        // Arrays in arithmetic context
        "(> (select aa_a aa_i) (select aa_a aa_j))",
        
        // Arithmetic constraints on array elements
        "(forall ((aa_k Int)) (=> (and (>= aa_k 0) (< aa_k 10)) (>= (select aa_a aa_k) 0)))"
    };
    
    std::cout << "=== Testing Array and Arithmetic Combination ===" << std::endl;
    
    try {
        // Declare variables and array
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        parser->mkArray("aa_a", int_sort, int_sort);
        parser->mkVarInt("aa_i");
        parser->mkVarInt("aa_j");
        parser->mkVarInt("aa_k");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

// Test combination of bitvectors with other theories
void test_bitvector_combination(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Bitvector in boolean context
        "(= #b1010 #b1010)",
        
        // Bitvector operations with boolean results
        "(bvult #b1010 #b1100)",
        
        // Bitvector with arithmetic
        "(= (bv2nat #b1010) 10)",
        
        // Bitvector in arrays
        "(select bv_a #b1010)",
        
        // Complex bitvector with other theories
        "(and (bvult bv_x bv_y) (= (select bv_a bv_x) #b0101))"
    };
    
    std::cout << "=== Testing Bitvector Combination ===" << std::endl;
    
    try {
        // Declare variables
        std::shared_ptr<SOMTParser::Sort> bv4_sort = parser->mkBVSort(4);
        parser->mkVarBv("bv_x", 4);
        parser->mkVarBv("bv_y", 4);
        
        // Declare array from BV to BV
        parser->mkArray("bv_a", bv4_sort, bv4_sort);
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

// Test combination of strings with other theories
void test_string_combination(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // String in boolean context
        "(= \"hello\" \"world\")",
        
        // String with arithmetic
        "(= (str.len str_s) 5)",
        
        // String indexing with arithmetic
        "(= (str.at str_s str_i) \"a\")",
        
        // String in arrays
        "(select str_a \"key\")",
        
        // Complex string operations with other theories
        "(and (str.prefixof \"pre\" str_s) (> (str.indexof str_s \"x\" 0) (+ str_i str_j)))"
    };
    
    std::cout << "=== Testing String Combination ===" << std::endl;
    
    try {
        // Declare variables
        std::shared_ptr<SOMTParser::Sort> str_sort = SOMTParser::SortManager::STR_SORT;
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        
        parser->mkVarStr("str_s");
        parser->mkVarInt("str_i");
        parser->mkVarInt("str_j");
        
        // Declare array from string to int
        parser->mkArray("str_a", str_sort, int_sort);
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

// Test quantifiers with different theories
void test_quantifier_combinations(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Quantifiers with arrays
        "(forall ((qt_i Int)) (=> (and (>= qt_i 0) (< qt_i 10)) (= (select qt_a qt_i) 0)))",
        
        // Quantifiers with bitvectors
        "(exists ((qt_x (_ BitVec 4))) (= (bvadd qt_x #b0001) qt_y))",
        
        // Quantifiers with strings
        "(forall ((qt_c String)) (=> (= (str.len qt_c) 1) (str.contains qt_s qt_c)))",
        
        // Nested quantifiers with mixed theories
        "(forall ((qt_i Int)) (exists ((qt_j Int)) (and (< qt_i qt_j) (= (select qt_a qt_i) (select qt_a qt_j)))))",
        
        // Complex quantifier with multiple theories
        "(exists ((qt_x Int) (qt_y Int)) (and (> qt_x 0) (< qt_y 10) (= (select qt_a qt_x) (select qt_a qt_y))))"
    };
    
    std::cout << "=== Testing Quantifiers with Different Theories ===" << std::endl;
    
    try {
        // Declare variables
        std::shared_ptr<SOMTParser::Sort> int_sort = parser->mkIntSort();
        std::shared_ptr<SOMTParser::Sort> bv4_sort = parser->mkBVSort(4);
        std::shared_ptr<SOMTParser::Sort> str_sort = parser->mkStrSort();
        
        parser->mkArray("qt_a", int_sort, int_sort);
        parser->mkVarBv("qt_y", 4);
        parser->mkVarStr("qt_s");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

// Test real-world combined theory examples
void test_real_world_examples(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Simple symbolic execution constraint
        "(and (= rw_x (+ rw_y 5)) (= rw_z (* 2 rw_x)) (> rw_z 20))",
        
        // Array bounds checking
        "(=> (and (>= rw_i 0) (< rw_i (select rw_a rw_array_length))) (>= (select rw_a rw_i) 0))",
        
        // String password validation
        "(and (>= (str.len rw_password) 8) (str.contains rw_password \"A\") (not (= rw_password \"password\")))",
        
        // Bitvector cryptographic constraint
        "(= (bvxor (bvand rw_bv_x rw_bv_y) (bvor rw_bv_x rw_bv_y)) rw_expected)"
    };
    
    std::cout << "=== Testing Real-World Combined Theory Examples ===" << std::endl;
    
    try {
        // Declare variables
        std::shared_ptr<SOMTParser::Sort> int_sort = parser->mkIntSort();
        std::shared_ptr<SOMTParser::Sort> bv32_sort = parser->mkBVSort(32);
        std::shared_ptr<SOMTParser::Sort> str_sort = parser->mkStrSort();
        
        parser->mkVarInt("rw_x");
        parser->mkVarInt("rw_y");
        parser->mkVarInt("rw_z");
        parser->mkVarInt("rw_i");
        parser->mkVarInt("rw_n");
        parser->mkVarInt("rw_max_val");
        parser->mkVarInt("rw_array_length");
        parser->mkArray("rw_a", int_sort, int_sort);
        parser->mkVarStr("rw_password");
        parser->mkVarBv("rw_bv_x", 32);
        parser->mkVarBv("rw_bv_y", 32);
        parser->mkVarBv("rw_expected", 32);
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
            assert(result && !result->isErr());
            std::cout << "  Result: " << parser->toString(result) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "======= Theory Combination Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_arithmetic_boolean_combination(parser);
    test_array_arithmetic_combination(parser);
    test_bitvector_combination(parser);
    test_string_combination(parser);
    test_quantifier_combinations(parser);
    test_real_world_examples(parser);
    
    return 0;
} 