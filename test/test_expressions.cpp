#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

// Test let expressions
void test_let_expressions(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Simple let expression
        "(let ((x 10)) (+ x 5))",
        
        // Multiple bindings
        "(let ((x 10) (y 20)) (+ x y))",
        
        // Nested let expressions
        "(let ((x 10)) (let ((y (* 2 x))) (+ x y)))",
        
        // Let with shadowing - should be error
        // "(let ((x 10)) (let ((x 20)) (+ x 5)))",
        
        // Let with complex expressions
        "(let ((x (+ 1 2)) (y (* 3 4))) (- y x))",
        
        // Let with boolean expressions
        "(let ((x true) (y false)) (and x (not y)))"
    };
    
    std::cout << "=== Testing Let Expressions ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "Expression: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result);
        std::cout << "  Result: " << parser->toString(result) << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> expanded = parser->expandLet(result);
        assert(expanded);
        std::cout << "  Expanded: " << parser->toString(expanded) << std::endl;
        std::cout << std::endl;
    }
}

// Example: NNF/CNF and getCNFBoolVar / getCNFAtom roundtrip
void test_cnf_bool_var_atom_roundtrip(SOMTParser::ParserPtr& parser) {
    std::cout << "=== Testing CNF BoolVar/Atom Roundtrip ===" << std::endl;
    parser->mkVarInt("x");
    parser->mkVarInt("y");

    std::shared_ptr<SOMTParser::DAGNode> phi = parser->mkExpr("(and (> x 0) (< y 3))");
    assert(phi);

    std::shared_ptr<SOMTParser::DAGNode> nnf = parser->toNNF(phi);
    assert(nnf);
    std::vector<std::shared_ptr<SOMTParser::DAGNode>> expr_vec = {phi};
    std::shared_ptr<SOMTParser::DAGNode> cnf = parser->toCNF(expr_vec);
    assert(cnf);

    std::shared_ptr<SOMTParser::DAGNode> atom_gt = parser->mkExpr("(> x 0)");
    assert(atom_gt);
    std::shared_ptr<SOMTParser::DAGNode> b = parser->getCNFBoolVar(atom_gt);
    assert(b && "getCNFBoolVar( (> x 0) ) should return the CNF bool var");
    std::shared_ptr<SOMTParser::DAGNode> a = parser->getCNFAtom(b);
    assert(a == atom_gt);
    assert(a && "getCNFAtom(b) should recover the atom");
    assert(parser->toString(a) == parser->toString(atom_gt) && "recovered atom should be (> x 0)");

    std::cout << "  phi: " << parser->toString(phi) << std::endl;
    std::cout << "  NNF: " << parser->toString(nnf) << std::endl;
    std::cout << "  CNF: " << parser->toString(cnf) << std::endl;
    std::cout << "  getCNFBoolVar( (> x 0) ) -> bool var; getCNFAtom(bool var) -> " << parser->toString(a) << " OK" << std::endl;
    std::cout << std::endl;
}

// Test normal forms (NNF, CNF, DNF)
void test_normal_forms(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Simple formula
        "(not (and a b))",
        
        // More complex formula
        "(=> (and a b) (or c d))",
        
        // Formula with nested operations
        "(or (and a b) (and c d))",
        
        // Formula with negations
        "(not (or (not a) (and b (not c))))",
        
        // Formula with implications
        "(=> (=> a b) c)",
        
        // Formula with equivalence
        "(= (and a b) (or c d))"
    };
    
    std::cout << "=== Testing Normal Forms ===" << std::endl;
    
    // First declare boolean variables
    try {
        parser->mkVarBool("a");
        parser->mkVarBool("b");
        parser->mkVarBool("c");
        parser->mkVarBool("d");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> original = parser->mkExpr(expr);
            assert(original);
            std::cout << "  Original: " << parser->toString(original) << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> nnf = parser->toNNF(original);
            assert(nnf);
            std::cout << "  NNF: " << parser->toString(nnf) << std::endl;
            std::vector<std::shared_ptr<SOMTParser::DAGNode>> expr_vec = {original};
            std::shared_ptr<SOMTParser::DAGNode> cnf = parser->toCNF(expr_vec);
            assert(cnf);
            std::cout << "  CNF: " << parser->toString(cnf) << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> dnf = parser->toDNF(original);
            assert(dnf);
            std::cout << "  DNF: " << parser->toString(dnf) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during variable declaration: " << e.what() << std::endl;
    }
}

// Test arithmetic normalization
void test_arithmetic_normalization(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Simple comparison
        "(< (+ x 2) 5)",
        
        // Comparison with variables on both sides
        "(= (+ x 2) (+ y 3))",
        
        // Complex arithmetic expression
        "(>= (+ (* 2 x) (* 3 y)) (* 4 z))",
        
        // Mixed arithmetic and boolean operations
        "(and (> x 0) (< y 10))",
        
        // Nested comparisons
        "(or (< x y) (> x z))"
    };
    
    std::cout << "=== Testing Arithmetic Normalization ===" << std::endl;
    
    // Declare variables
    try {
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        parser->mkVarInt("z");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> original = parser->mkExpr(expr);
            assert(original);
            std::cout << "  Original: " << parser->toString(original) << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> normalized = parser->arithNormalize(original);
            assert(normalized);
            std::cout << "  Normalized: " << parser->toString(normalized) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during variable declaration: " << e.what() << std::endl;
    }
}

// Test binarization of operations
void test_binarization(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // N-ary and
        "(and a b c d)",
        
        // N-ary or
        "(or a b c d)",
        
        // N-ary equality
        "(= x y z)",
        
        // N-ary addition
        "(+ x y z)",
        
        // N-ary multiplication
        "(* x y z)",
        
        // Mixed operations
        "(and (or a b c) (= x y z))",
        
        // Nested operations
        "(and (or a b) (or c d) (or e f))"
    };
    
    std::cout << "=== Testing Binarization ===" << std::endl;
    
    // Declare variables
    try {
        parser->mkVarBool("a");
        parser->mkVarBool("b");
        parser->mkVarBool("c");
        parser->mkVarBool("d");
        parser->mkVarBool("e");
        parser->mkVarBool("f");
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        parser->mkVarInt("z");
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> original = parser->mkExpr(expr);
            assert(original);
            std::cout << "  Original: " << parser->toString(original) << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> binarized = parser->binarizeOp(original);
            assert(binarized);
            std::cout << "  Binarized: " << parser->toString(binarized) << std::endl;
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during variable declaration: " << e.what() << std::endl;
    }
}

// Test collecting atoms and variables
void test_collect_atoms_variables(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // Mixed boolean and arithmetic
        "(and (> x 0) (< y 10) (= z 5))",
        
        // Complex nested expression
        "(or (and (> x 0) (< y 10)) (not (= z 5)))",
        
        // Expression with functions
        "(and (p x y) (q z))"
    };
    
    std::cout << "=== Testing Collecting Atoms and Variables ===" << std::endl;
    
    // Declare variables and functions
    try {
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        parser->mkVarInt("z");
        
        std::shared_ptr<SOMTParser::Sort> int_sort = SOMTParser::SortManager::INT_SORT;
        std::shared_ptr<SOMTParser::Sort> bool_sort = SOMTParser::SortManager::BOOL_SORT;
        
        std::vector<std::shared_ptr<SOMTParser::Sort>> p_params = {int_sort, int_sort};
        parser->mkFuncDec("p", p_params, bool_sort);
        
        std::vector<std::shared_ptr<SOMTParser::Sort>> q_params = {int_sort};
        parser->mkFuncDec("q", q_params, bool_sort);
        
        for (const auto& expr : expressions) {
            std::cout << "Expression: " << expr << std::endl;
            std::shared_ptr<SOMTParser::DAGNode> formula = parser->mkExpr(expr);
            assert(formula);
            std::cout << "  Formula: " << parser->toString(formula) << std::endl;
            std::unordered_set<std::shared_ptr<SOMTParser::DAGNode>> atoms;
            parser->collectAtoms(formula, atoms);
            std::cout << "  Atoms:" << std::endl;
            for (const auto& atom : atoms) {
                std::cout << "    " << parser->toString(atom) << std::endl;
            }
            std::unordered_set<std::shared_ptr<SOMTParser::DAGNode>> vars;
            parser->collectVars(formula, vars);
            std::cout << "  Variables:" << std::endl;
            for (const auto& var : vars) {
                std::cout << "    " << parser->toString(var) << std::endl;
            }
            assert(atoms.size() > 0 || vars.size() > 0);
            std::cout << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Exception during declaration: " << e.what() << std::endl;
    }
}

// Test expression substitution
void test_substitution(SOMTParser::ParserPtr& parser) {
    std::cout << "=== Testing Expression Substitution ===" << std::endl;
    
    try {
        // Declare variables
        parser->mkVarInt("x");
        parser->mkVarInt("y");
        parser->mkVarInt("z");
        
        // Create a formula: x + y > z
        std::string expr = "(> (+ x y) z)";
        std::shared_ptr<SOMTParser::DAGNode> formula = parser->mkExpr(expr);
        std::cout << "Original formula: " << parser->toString(formula) << std::endl;
        
        // Create substitution map
        std::unordered_map<std::string, std::shared_ptr<SOMTParser::DAGNode>> subst_map;
        subst_map["x"] = parser->mkConstInt(5);
        subst_map["y"] = parser->mkConstInt(10);
        
        // Apply substitution
        std::shared_ptr<SOMTParser::DAGNode> substituted = parser->substitute(formula, subst_map);
        assert(substituted);
        std::cout << "After substitution {x->5, y->10}: " << parser->toString(substituted) << std::endl;
        assert(parser->toString(substituted).find("15") != std::string::npos || parser->toString(substituted).find("5") != std::string::npos);
        std::unordered_map<std::string, std::shared_ptr<SOMTParser::DAGNode>> subst_map2;
        subst_map2["z"] = parser->mkConstInt(0);
        std::shared_ptr<SOMTParser::DAGNode> substituted2 = parser->substitute(substituted, subst_map2);
        assert(substituted2);
        std::cout << "After substitution {z->0}: " << parser->toString(substituted2) << std::endl;
        assert(parser->toString(substituted2).find("0") != std::string::npos || parser->toString(substituted2).find("15") != std::string::npos);
        
        // Create a more complex formula with let
        std::string expr2 = "(let ((a (+ x y))) (> a z))";
        std::shared_ptr<SOMTParser::DAGNode> formula2 = parser->mkExpr(expr2);
        assert(formula2);
        std::cout << "Complex formula: " << parser->toString(formula2) << std::endl;
        
        // Apply substitution to complex formula
        std::shared_ptr<SOMTParser::DAGNode> substituted3 = parser->substitute(formula2, subst_map);
        assert(substituted3);
        std::cout << "After substitution {x->5, y->10}: " << parser->toString(substituted3) << std::endl;

    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    
    std::cout << std::endl;
}

int main() {
    std::cout << "======= Complex Expressions Test =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_let_expressions(parser);
    test_cnf_bool_var_atom_roundtrip(parser);
    test_normal_forms(parser);
    test_arithmetic_normalization(parser);
    test_binarization(parser);
    test_collect_atoms_variables(parser);
    test_substitution(parser);
    
    return 0;
} 