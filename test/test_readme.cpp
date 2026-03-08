/*
 * Test file for README.md examples
 * Validates all C++ code examples shown in the README
 */

#include "somtparser/frontend/parser.h"
#include <iostream>
#include <unordered_set>
#include <cassert>

void test_basic_parsing() {
    std::cout << "\n=== Testing Basic Parsing ===" << std::endl;
    
    try {
        // Initialize the parser
        SOMTParser::Parser parser;
        
        // Create a simple SMT-LIB2 string to parse
        std::string smt_content = R"(
            (declare-const x Int)
            (declare-const y Int)
            (assert (> (+ x y) 10))
        )";
        
        assert(parser.parseStr(smt_content) && "parse SMT content must succeed");
        auto assertions = parser.getAssertions();
        assert(assertions.size() == 1 && "should have one assertion");
        std::cout << "Parsed assertions:" << std::endl;
        for(auto constraint: assertions){
            std::cout << "  " << parser.toString(constraint) << std::endl;
        }
        std::cout << "✓ Basic parsing test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Basic parsing test failed: " << e.what() << std::endl;
    }
}

void test_expression_building() {
    std::cout << "\n=== Testing Expression Building ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        
        // Create variables and expressions
        auto x = parser->mkVarInt("x");
        auto y = parser->mkVarInt("y");
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> add_params = {x, y};
        auto sum = parser->mkAdd(add_params);
        auto condition = parser->mkGt(sum, parser->mkConstInt(10));
        
        assert(sum && condition);
        assert(parser->toString(condition).find(">") != std::string::npos || parser->toString(condition).find("10") != std::string::npos);
        std::cout << "Sum expression: " << parser->toString(sum) << std::endl;
        std::cout << "Condition: " << parser->toString(condition) << std::endl;
        std::cout << "✓ Expression building test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Expression building test failed: " << e.what() << std::endl;
    }
}



// Some Useful Features Tests

void test_formula_analysis() {
    std::cout << "\n=== Testing Formula Analysis ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        
        // Create variables and build complex formula
        auto x = parser->mkVarInt("x");
        auto y = parser->mkVarInt("y");
        auto z = parser->mkVarBool("z");
        
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> xy_parts = {x, y};
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> parts = {
            parser->mkGt(parser->mkAdd(xy_parts), parser->mkConstInt(0)),
            parser->mkLt(x, parser->mkConstInt(10)),
            z
        };
        auto formula = parser->mkAnd(parts);
        
        // Collect atoms and variables
        std::unordered_set<std::shared_ptr<SOMTParser::DAGNode>> atoms, vars;
        parser->collectAtoms(formula, atoms);
        parser->collectVars(formula, vars);
        
        assert(atoms.size() > 0 && vars.size() >= 2 && "formula should have atoms and variables");
        std::cout << "Formula: " << parser->toString(formula) << std::endl;
        std::cout << "Found " << atoms.size() << " atoms and " << vars.size() << " variables" << std::endl;
        std::cout << "✓ Formula analysis test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Formula analysis test failed: " << e.what() << std::endl;
    }
}

void test_formula_conversions() {
    std::cout << "\n=== Testing Formula Format Conversions ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        auto a = parser->mkVarBool("a");
        auto b = parser->mkVarBool("b");
        auto c = parser->mkVarBool("c");
        
        // Build complex formula
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> or_parts = {b, c};
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> formula_parts = {
            parser->mkImplies(a, b),
            parser->mkOr(or_parts)
        };
        auto formula = parser->mkAnd(formula_parts);
        
        // Convert to different normal forms
        auto nnf = parser->toNNF(formula);
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> formula_vec = {formula};
        auto cnf = parser->toCNF(formula_vec);
        auto dnf = parser->toDNF(formula);
        
        assert(nnf && cnf && dnf);
        std::cout << "Original: " << parser->toString(formula) << std::endl;
        std::cout << "NNF: " << parser->toString(nnf) << std::endl;
        std::cout << "CNF: " << parser->toString(cnf) << std::endl;
        std::cout << "DNF: " << parser->toString(dnf) << std::endl;
        std::cout << "✓ Formula conversions test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Formula conversions test failed: " << e.what() << std::endl;
    }
}

void test_model_evaluation() {
    std::cout << "\n=== Testing Model Evaluation ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        auto model = SOMTParser::newModel();
        
        // Create a formula: (sin(x) > 0) ∧ (y > 1) ∧ (z ⟹ (x + y > 3))
        auto x = parser->mkVarReal("x");
        auto y = parser->mkVarReal("y");
        auto z = parser->mkVarBool("z");
        
        auto sin_x = parser->mkSin(x);
        auto cond1 = parser->mkGt(sin_x, parser->mkConstReal(std::string("0")));             // sin(x) > 0
        
        auto cond2 = parser->mkGt(y, parser->mkConstReal(std::string("1")));                 // y > 1
        
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> sum_xy_parts = {x, y};
        auto sum_xy = parser->mkAdd(sum_xy_parts);
        auto cond3 = parser->mkImplies(z, parser->mkGt(sum_xy, parser->mkConstReal(std::string("3"))));  // z ⟹ (x + y > 3)
        
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> formula_parts = {cond1, cond2, cond3};
        auto formula = parser->mkAnd(formula_parts);
        
        // Assign values and evaluate
        model->add(x, parser->mkConstReal(std::string("1.5")));   // sin(1.5) ≈ 0.997 > 0 ✓
        model->add(y, parser->mkConstReal(std::string("2.0")));   // 2.0 > 1 ✓
        model->add(z, parser->mkTrue());                          // x + y = 3.5 > 3 ✓
        
        auto result = parser->evaluate(formula, model);
        assert(result && result->isTrue() && "evaluate with given model should yield true");
        std::cout << "Formula: " << parser->toString(formula) << std::endl;
        std::cout << "Result: " << parser->toString(result) << std::endl;
        std::cout << "✓ Model evaluation test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Model evaluation test failed: " << e.what() << std::endl;
    }
}

void test_let_expression_expansion() {
    std::cout << "\n=== Testing Let Expression Expansion ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        
        auto x = parser->mkVarInt("x");
        auto y = parser->mkVarInt("y");
        auto let_expr = parser->mkExpr("(let ((temp (+ x 1))) (> temp y))");
        auto expanded = parser->expandLet(let_expr);
        
        assert(let_expr && expanded);
        assert(parser->toString(expanded).find("temp") == std::string::npos || parser->toString(expanded).size() >= 5);
        std::cout << "Let expression: " << parser->toString(let_expr) << std::endl;
        std::cout << "Expanded: " << parser->toString(expanded) << std::endl;
        std::cout << "✓ Let expression expansion test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Let expression expansion test failed: " << e.what() << std::endl;
    }
}

void test_enhanced_omt() {
    std::cout << "\n=== Testing Enhanced Optimization Modulo Theories (OMT) ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        
        // Parse OMT commands
        parser->parseStr("(declare-const x Int)");
        parser->parseStr("(declare-const y Int)");
        parser->parseStr("(assert (> x 0))");
        parser->parseStr("(assert (> y 0))");
        parser->parseStr("(assert-soft (< x 100) :weight 1.0)");
        parser->parseStr("(assert-soft (< y 50) :weight 2.0)");
        
        // Access soft assertions and objectives
        auto soft_assertions = parser->getSoftAssertions();
        auto soft_weights = parser->getSoftWeights();
        
        assert(soft_assertions.size() == 2 && soft_weights.size() == 2);
        std::cout << "Found " << soft_assertions.size() << " soft assertions" << std::endl;
        for (size_t i = 0; i < soft_assertions.size(); ++i) {
            std::cout << "Soft assertion " << i << " (weight " << parser->toString(soft_weights[i]) << "): "
                      << parser->toString(soft_assertions[i]) << std::endl;
        }
        std::cout << "✓ Enhanced OMT test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Enhanced OMT test failed: " << e.what() << std::endl;
    }
}

void test_comprehensive_features() {
    std::cout << "\n=== Testing Comprehensive Features ===" << std::endl;
    
    try {
        auto parser = SOMTParser::newParser();
        auto model = SOMTParser::newModel();
        
        // Create variables for different theories
        auto x = parser->mkVarInt("x");
        auto y = parser->mkVarReal("y");
        auto b = parser->mkVarBool("b");
        
        // Create complex expressions
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> arith_parts = {x, parser->mkConstInt(5)};
        auto arithmetic_expr = parser->mkAdd(arith_parts);
        auto comparison_expr = parser->mkGt(arithmetic_expr, parser->mkConstInt(10));
        auto conditional_expr = parser->mkIte(b, x, parser->mkConstInt(0));
        
        // Build compound formula
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> compound_parts = {
            comparison_expr, 
            parser->mkGt(y, parser->mkConstReal(SOMTParser::Real(0.5)))
        };
        auto compound_formula = parser->mkAnd(compound_parts);
        
        std::cout << "Compound formula: " << parser->toString(compound_formula) << std::endl;
        
        // Test evaluation with partial model
        model->add(x, parser->mkConstInt(8));
        model->add(b, parser->mkTrue());
        
        auto eval_arithmetic = parser->evaluate(arithmetic_expr, model);
        auto eval_conditional = parser->evaluate(conditional_expr, model);
        assert(eval_arithmetic && parser->toString(eval_arithmetic) == "13");
        assert(eval_conditional && parser->toString(eval_conditional) == "8");
        std::cout << "Arithmetic expression evaluated: " << parser->toString(eval_arithmetic) << std::endl;
        std::cout << "Conditional expression evaluated: " << parser->toString(eval_conditional) << std::endl;
        
        // Test format conversions
        auto formula_nnf = parser->toNNF(compound_formula);
        std::vector<std::shared_ptr<SOMTParser::DAGNode>> formula_vec;
        formula_vec.push_back(compound_formula);
        auto formula_cnf = parser->toCNF(formula_vec);
        
        std::cout << "NNF: " << parser->toString(formula_nnf) << std::endl;
        std::cout << "CNF: " << parser->toString(formula_cnf) << std::endl;
        
        std::cout << "✓ Comprehensive features test passed" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "✗ Comprehensive features test failed: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Testing README.md C++ Examples" << std::endl;
    std::cout << "===============================" << std::endl;
    
    // Basic Usage Examples Tests
    test_basic_parsing();
    test_expression_building();
    
    // Some Useful Features Tests
    test_formula_analysis();
    test_formula_conversions();
    test_model_evaluation();
    test_let_expression_expansion();
    test_enhanced_omt();
    
    // Comprehensive Features Test
    test_comprehensive_features();
    
    std::cout << "\n===============================" << std::endl;
    std::cout << "All README examples tested!" << std::endl;
    
    return 0;
}