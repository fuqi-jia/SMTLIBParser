/* -*- Source -*-
 *
 * An SMT/OMT Parser (Conversion part)
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

#include "parser.h"

namespace SMTLIBParser {

    void Parser::collectAtoms(std::shared_ptr<DAGNode> expr, boost::unordered_set<std::shared_ptr<DAGNode>>& atoms) {
        boost::unordered_set<std::shared_ptr<DAGNode>> visited;
        collectAtoms(expr, atoms, visited);
    }

    void Parser::collectAtoms(std::vector<std::shared_ptr<DAGNode>> exprs, boost::unordered_set<std::shared_ptr<DAGNode>>& atoms) {
        boost::unordered_set<std::shared_ptr<DAGNode>> visited;
        for(auto& expr : exprs) {
            collectAtoms(expr, atoms, visited);
        }
    }

    void Parser::collectAtoms(std::shared_ptr<DAGNode> expr, boost::unordered_set<std::shared_ptr<DAGNode>>& atoms, boost::unordered_set<std::shared_ptr<DAGNode>>& visited) {
        if (visited.find(expr) != visited.end()) {
            return;
        }
        visited.insert(expr);
        if (expr->isAtom()) {
            // directly insert the atom and return
            atoms.insert(expr);
            return;
        }
        else{
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                collectAtoms(expr->getChild(i), atoms, visited);
            }
        }
    }

    std::shared_ptr<DAGNode> Parser::replaceAtoms(std::shared_ptr<DAGNode> expr, boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& atom_map) {
        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> visited;
        bool is_changed = false;
        std::shared_ptr<DAGNode> new_expr = replaceAtoms(expr, atom_map, visited, is_changed);
        if (is_changed) {
            return new_expr;
        }
        return expr;
    }

    std::shared_ptr<DAGNode> Parser::replaceAtoms(std::shared_ptr<DAGNode> expr, boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& atom_map, boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& visited, bool& is_changed) {
        if (visited.find(expr) != visited.end()) {
            return visited[expr];
        }
        if (expr->isAtom() && atom_map.find(expr) != atom_map.end()) {
            is_changed = true;
            visited[expr] = atom_map[expr];
            return atom_map[expr];
        }
        std::vector<std::shared_ptr<DAGNode>> children;
        for (size_t i = 0; i < expr->getChildrenSize(); i++) {
            bool child_is_changed = false;
            std::shared_ptr<DAGNode> child_expr = replaceAtoms(expr->getChild(i), atom_map, visited, child_is_changed);
            if (child_is_changed) {
                is_changed = true;
            }
            children.emplace_back(child_expr);
        }
        if (is_changed) {
            std::shared_ptr<DAGNode> new_expr = mkOper(expr->getSort(), expr->getKind(), children);
            visited[expr] = new_expr; // no need to visit again
            return new_expr;
        }
        cassert(!is_changed, "replaceAtoms: is_changed is true");
        visited[expr] = expr;
        return expr;
    }

    std::shared_ptr<DAGNode> Parser::toTseitinCNF(std::shared_ptr<DAGNode> expr, std::vector<std::shared_ptr<DAGNode>>& clauses) {
        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> visited;
        std::shared_ptr<DAGNode> result = toTseitinCNF(expr, visited, clauses);
        return result;
    }
    std::shared_ptr<DAGNode> Parser::toTseitinXor(std::shared_ptr<DAGNode> a, std::shared_ptr<DAGNode> b, std::vector<std::shared_ptr<DAGNode>>& clauses) {
        // c <-> a xor b <=> (c -> a xor b) and (a xor b -> c)
        // => c -> a xor b => ¬c or a xor b => ¬c or ((¬a or ¬b) and (a or b))
        //    => (¬c or ¬a or ¬b) and (¬c or a or b)
        // => a xor b -> c => ¬((¬a and b) or (a and ¬b)) or c => (a or ¬b) and (¬a or b) -> c
        //    => (a or ¬b or c) and (¬a or b or c)
        std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
        // -a -b -c
        std::vector<std::shared_ptr<DAGNode>> or_children1;
        or_children1.emplace_back(mkNot(c));
        or_children1.emplace_back(mkNot(a));
        or_children1.emplace_back(mkNot(b));
        clauses.emplace_back(mkOr(or_children1));
        // a b -c
        std::vector<std::shared_ptr<DAGNode>> or_children2;
        or_children2.emplace_back(mkNot(c));
        or_children2.emplace_back(a);
        or_children2.emplace_back(b);
        clauses.emplace_back(mkOr(or_children2));
        // -a b c
        std::vector<std::shared_ptr<DAGNode>> or_children3;
        or_children3.emplace_back(c);
        or_children3.emplace_back(mkNot(a));
        or_children3.emplace_back(b);
        clauses.emplace_back(mkOr(or_children3));
        // a -b c
        std::vector<std::shared_ptr<DAGNode>> or_children4;
        or_children4.emplace_back(c);
        or_children4.emplace_back(a);
        or_children4.emplace_back(mkNot(b));
        clauses.emplace_back(mkOr(or_children4));
        return c;
    }

    // auxiliary function: handle the equivalence relation between two boolean variables
    std::shared_ptr<DAGNode> Parser::toTseitinEq(std::shared_ptr<DAGNode> a, std::shared_ptr<DAGNode> b, std::vector<std::shared_ptr<DAGNode>>& clauses) {
        std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
        
        // c <-> (a <-> b) -> c -> (a <-> b) and (a <-> b) -> c
        // => (¬c or a or ¬b) and (¬c or ¬a or b) and (c or a or b) and (c or ¬a or ¬b)
        
        // add clause: (¬c ∨ a ∨ ¬b) - when a is false or b is true, c can be false
        clauses.emplace_back(mkOr({mkNot(c), a, mkNot(b)}));
        
        // add clause: (¬c ∨ ¬a ∨ b) - when a is true or b is false, c can be false
        clauses.emplace_back(mkOr({mkNot(c), mkNot(a), b}));
        
        // add clause: (c ∨ a ∨ b) - when a and b are both false, c must be true
        clauses.emplace_back(mkOr({c, a, b}));
        
        // add clause: (c ∨ ¬a ∨ ¬b) - when a and b are both true, c must be true
        clauses.emplace_back(mkOr({c, mkNot(a), mkNot(b)}));
        
        return c;
    }

    // auxiliary function: handle the inequality relation between two boolean variables
    std::shared_ptr<DAGNode> Parser::toTseitinDistinct(std::shared_ptr<DAGNode> a, std::shared_ptr<DAGNode> b, std::vector<std::shared_ptr<DAGNode>>& clauses) {
        std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
        
        // c <-> (a != b) -> c -> (a != b)
        // => (¬c or a or b) and (¬c or ¬a or ¬b) and (c or ¬a or b) and (c or a or ¬b)
        
        // add clause: (¬c ∨ a ∨ b) - when a and b are both false, c can be false
        clauses.emplace_back(mkOr({mkNot(c), a, b}));
        
        // add clause: (¬c ∨ ¬a ∨ ¬b) - when a and b are both true, c can be false
        clauses.emplace_back(mkOr({mkNot(c), mkNot(a), mkNot(b)}));
        
        // add clause: (c ∨ ¬a ∨ b) - when a is true and b is false, c must be true
        clauses.emplace_back(mkOr({c, mkNot(a), b}));
        
        // add clause: (c ∨ a ∨ ¬b) - when a is false and b is true, c must be true
        clauses.emplace_back(mkOr({c, a, mkNot(b)}));
        
        return c;
    }
    std::shared_ptr<DAGNode> Parser::toTseitinCNF(std::shared_ptr<DAGNode> expr, boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& visited, std::vector<std::shared_ptr<DAGNode>>& clauses) {
        // Tseitin CNF is ¬applied to atoms: all atoms are already in CNF form
        if(expr->isAtom()){
            // directly return the original expression
            cassert(cnf_map.find(expr) != cnf_map.end(), "toTseitinCNF: expr is an atom but not in cnf_map");
            return cnf_map[expr];
        }
        if(expr->isLiteral() || expr->isTempVar()) {
            // always return the original expression
            // and no need to add to visited
            return expr;
        }
        if (visited.find(expr) != visited.end()) {
            return visited[expr];
        }
        
        if(expr->isLet()){
            std::shared_ptr<DAGNode> result = toTseitinCNF(expandLet(expr), visited, clauses);
            visited[expr] = result;
            return result;
        }
        // c <-> ¬a <=> c -> ¬a and ¬a -> c
        // => ¬c or ¬a
        // => a or c
        if(expr->isNot()){
            // TODO: after NNF
            std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
            clauses.emplace_back(mkOr({mkNot(c), mkNot(expr->getChild(0))}));
            clauses.emplace_back(mkOr({expr->getChild(0), c}));
            visited[expr] = c;
            return c;
        }
        // a and b and ... <=> c <-> a and b and ...
        // => c -> a and b and ... => ¬c or a and b and ... => (¬c or a) and (¬c or b) and ...
        // => a and b and ... -> c => ¬a or ¬b or ... or c
        else if (expr->isAnd()) {
            std::vector<std::shared_ptr<DAGNode>> children;
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child_cnf = toTseitinCNF(expr->getChild(i), visited, clauses);
                children.emplace_back(child_cnf);
            }
            if(children.size() == 1){
                // return the only child, which is a temp var or boolean variable
                cassert(children[0]->isLiteral(), "toTseitinCNF: children[0] is not a literal");
                return children[0];
            }
            std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
            std::vector<std::shared_ptr<DAGNode>> or_children;
            for(auto& child : children) {
                clauses.emplace_back(mkOr({mkNot(c), child}));
                or_children.emplace_back(mkNot(child));
            }
            or_children.emplace_back(c);
            clauses.emplace_back(mkOr(or_children));
            visited[expr] = c; // no need to visit again
            cassert(c->isLiteral() || c->isTempVar(), "toTseitinCNF: c is not a literal or temp var");
            return c;
        }
        // a or b or ... <=> c <-> a or b or ...
        // => c -> a or b or ... => ¬c or a or b or ... => ¬c or a or b or ...
        // => a or b or ... -> c => (¬a and ¬b and ...) or c => ¬a or c and ¬b or c and ...
        else if (expr->isOr()) {
            std::vector<std::shared_ptr<DAGNode>> children;
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child_cnf = toTseitinCNF(expr->getChild(i), visited, clauses);
                children.emplace_back(child_cnf);
            }
            if(children.size() == 1){
                // return the only child, which is a temp var or boolean variable
                cassert(children[0]->isLiteral(), "toTseitinCNF: children[0] is not a literal");
                return children[0];
            }
            std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
            std::vector<std::shared_ptr<DAGNode>> or_children;
            for(auto& child : children) {
                clauses.emplace_back(mkOr({mkNot(child), c}));
                or_children.emplace_back(child);
            }
            or_children.emplace_back(mkNot(c));
            clauses.emplace_back(mkOr(or_children));
            visited[expr] = c; // no need to visit again
            cassert(c->isLiteral() || c->isTempVar(), "toTseitinCNF: c is not a literal or temp var");
            return c;
        }
        // c <-> a -> b <=> c -> a -> b and a -> b -> c
        // => c -> a -> b => ¬c or ¬a or b
        // => a -> b -> c => ¬a or ¬b or c
        else if (expr->isImplies()) {
            std::vector<std::shared_ptr<DAGNode>> children;
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child_cnf = toTseitinCNF(expr->getChild(i), visited, clauses);
                children.emplace_back(child_cnf);
            }
            if(children.size() == 1){
                // return the only child, which is a temp var or boolean variable
                cassert(children[0]->isLiteral(), "toTseitinCNF: children[0] is not a literal");
                return children[0];
            }
            std::shared_ptr<DAGNode> c = mkTempVar(BOOL_SORT);
            std::vector<std::shared_ptr<DAGNode>> or_children1;
            std::vector<std::shared_ptr<DAGNode>> or_children2;
            or_children1.emplace_back(mkNot(c));
            for(size_t i = 0; i < children.size(); i++) {
                if(i == children.size() - 1){
                    or_children1.emplace_back(children[i]);
                } else {
                    or_children1.emplace_back(mkNot(children[i]));
                }
                or_children2.emplace_back(mkNot(children[i]));
            }
            or_children2.emplace_back(c);
            clauses.emplace_back(mkOr(or_children1));
            clauses.emplace_back(mkOr(or_children2));
            visited[expr] = c; // no need to visit again
            cassert(c->isLiteral() || c->isTempVar(), "toTseitinCNF: c is not a literal or temp var");
            return c;
        }
        // c <-> a xor b <=> (c -> a xor b) and (a xor b -> c)
        // => c -> a xor b => ¬c or a xor b => ¬c or ((¬a or ¬b) and (a or b))
        //    => (¬c or ¬a or ¬b) and (¬c or a or b)
        // => a xor b -> c => ¬((¬a and b) or (a and ¬b)) or c => (a or ¬b) and (¬a or b) -> c
        //    => (a or ¬b or c) and (¬a or b or c)
        else if (expr->isXor()) {
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                std::shared_ptr<DAGNode> child = toTseitinCNF(expr->getChild(i), visited, clauses);
                children.emplace_back(child);
            }
            std::shared_ptr<DAGNode> result = children[0];
            for(size_t i = 1; i < children.size(); i++){
                result = toTseitinXor(result, children[i], clauses);
            }
            visited[expr] = result;
            return result;
        }
        else if(expr->isEq()){
            // all children are boolean variables
            bool all_bool = true;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                if(!expr->getChild(i)->getSort()->isBool()){
                    all_bool = false;
                    err_all(ERROR_TYPE::ERR_TYPE_MIS, "Need boolean variables for eq, but got " + expr->getChild(i)->getSort()->toString());
                    break;
                }
            }

            cassert(all_bool, "toTseitinCNF: eq has non-boolean variables");
            
            if(all_bool){
                if(expr->getChildrenSize() == 1){
                    // directly return the only child
                    std::shared_ptr<DAGNode> child_cnf = toTseitinCNF(expr->getChild(0), visited, clauses);
                    visited[expr] = child_cnf;
                    return child_cnf;
                }
                else if(expr->getChildrenSize() == 2){
                    // two children
                    std::shared_ptr<DAGNode> a = toTseitinCNF(expr->getChild(0), visited, clauses);
                    std::shared_ptr<DAGNode> b = toTseitinCNF(expr->getChild(1), visited, clauses);
                    std::shared_ptr<DAGNode> c = toTseitinEq(a, b, clauses);
                    visited[expr] = c;
                    cassert(c->isLiteral() || c->isTempVar(), "toTseitinCNF: c is not a literal or temp var");
                    return c;
                }
                else{
                    // multiple children
                    std::vector<std::shared_ptr<DAGNode>> eq_results;
                    std::shared_ptr<DAGNode> first = toTseitinCNF(expr->getChild(0), visited, clauses);
                    
                    for(size_t i = 1; i < expr->getChildrenSize(); i++){
                        std::shared_ptr<DAGNode> next = toTseitinCNF(expr->getChild(i), visited, clauses);
                        eq_results.emplace_back(toTseitinEq(first, next, clauses));
                    }
                    
                    // combine all equalities using AND
                    std::shared_ptr<DAGNode> result = mkTempVar(BOOL_SORT);
                    
                    // add the equivalence relation clauses
                    // result -> (eq1 ∧ eq2 ∧ ... ∧ eqn)
                    // => ¬result ∨ (eq1 ∧ eq2 ∧ ... ∧ eqn)
                    // => (¬result ∨ eq1) ∧ (¬result ∨ eq2) ∧ ... ∧ (¬result ∨ eqn)
                    for(auto& eq : eq_results){
                        clauses.emplace_back(mkOr({mkNot(result), eq}));
                    }
                    
                    // (eq1 ∧ eq2 ∧ ... ∧ eqn) -> result
                    // => ¬(eq1 ∧ eq2 ∧ ... ∧ eqn) ∨ result
                    // => (¬eq1 ∨ ¬eq2 ∨ ... ∨ ¬eqn) ∨ result
                    std::vector<std::shared_ptr<DAGNode>> or_children;
                    for(auto& eq : eq_results){
                        or_children.emplace_back(mkNot(eq));
                    }
                    or_children.emplace_back(result);
                    clauses.emplace_back(mkOr(or_children));
                    
                    visited[expr] = result;
                    cassert(result->isLiteral() || result->isTempVar(), "toTseitinCNF: result is not a literal or temp var");
                    return result;
                }
            }
        }
        else if(expr->isDistinct() && expr->getChild(0)->getSort()->isBool() && expr->getChild(1)->getSort()->isBool()){
            // ensure all children are boolean
            bool all_bool = true;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                if(!expr->getChild(i)->getSort()->isBool()){
                    all_bool = false;
                    break;
                }
            }
            
            if(all_bool){
                // handle boolean inequality
                if(expr->getChildrenSize() == 1){
                    // single child, meaningless, return true
                    err_all(ERROR_TYPE::ERR_TYPE_MIS, "distinct with 1 variable is meaningless");
                    return mkTrue();
                }
                else if(expr->getChildrenSize() == 2){
                    // two children
                    std::shared_ptr<DAGNode> a = toTseitinCNF(expr->getChild(0), visited, clauses);
                    std::shared_ptr<DAGNode> b = toTseitinCNF(expr->getChild(1), visited, clauses);
                    std::shared_ptr<DAGNode> c = toTseitinDistinct(a, b, clauses);
                    visited[expr] = c;
                    cassert(c->isLiteral() || c->isTempVar(), "toTseitinCNF: c is not a literal or temp var");
                    return c;
                }
                else{
                    // for boolean variables, distinct with more than 2 variables is always unsatisfiable
                    // because boolean values can only be true or false
                    std::cerr << "toTseitinCNF: distinct with more than 2 variables is always unsatisfiable" << std::endl;
                    return mkFalse();
                }
            }
        }
        else if(expr->isIte() && expr->getChild(1)->getSort()->isBool() && expr->getChild(2)->getSort()->isBool()){
            // handle condition expression: if a then b else c
            std::shared_ptr<DAGNode> a = toTseitinCNF(expr->getChild(0), visited, clauses);
            std::shared_ptr<DAGNode> b = toTseitinCNF(expr->getChild(1), visited, clauses);
            std::shared_ptr<DAGNode> c = toTseitinCNF(expr->getChild(2), visited, clauses);
            std::shared_ptr<DAGNode> d = mkTempVar(BOOL_SORT);
            
            // add clause: (¬d ∨ ¬a ∨ b) - when a is true, d must be the same as b
            clauses.emplace_back(mkOr({mkNot(d), mkNot(a), b}));
            
            // add clause: (¬d ∨ a ∨ c) - when a is false, d must be the same as c
            clauses.emplace_back(mkOr({mkNot(d), a, c}));
            
            // add clause: (d ∨ ¬a ∨ ¬b) - when a is true and b is true, d must be true
            clauses.emplace_back(mkOr({d, mkNot(a), mkNot(b)}));
            
            // add clause: (d ∨ a ∨ ¬c) - when a is false and c is false, d must be true
            clauses.emplace_back(mkOr({d, a, mkNot(c)}));
            
            visited[expr] = d;
            cassert(d->isLiteral() || d->isTempVar(), "toTseitinCNF: d is not a literal or temp var");
            return d;
        }
        else{
            err_all(ERROR_TYPE::ERR_TYPE_MIS, "unsupported node type: " + kindToString(expr->getKind()));
            return mkFalse();
        }

        return expr;
    }
    
    

    // convert a list of expressions to CNF (a large AND node, whose children are all OR clauses)
    std::shared_ptr<DAGNode> Parser::toCNF(std::vector<std::shared_ptr<DAGNode>> exprs) {
        std::shared_ptr<DAGNode> result = mkAnd(exprs);
        std::shared_ptr<DAGNode> cnf = toCNF(result);
        cnf_map[result] = cnf;
        return cnf;
    }

    // convert a single expression to CNF
    std::shared_ptr<DAGNode> Parser::toCNF(std::shared_ptr<DAGNode> expr) {
        if(cnf_map.find(expr) != cnf_map.end()){
            return cnf_map[expr];
        }
        // convert to NNF first
        expr = toNNF(expr);
        // simple check: if all children are atom, just return it
        bool all_atoms = true;
        for(size_t i=0;i<expr->getChildrenSize();i++){
            if(!expr->getChild(i)->isAtom()){
                all_atoms = false;
                break;
            }
        }
        if(all_atoms){
            return expr;
        }
        cassert(!all_atoms, "toCNF: expr has non-atom children");
        // expand let
        if(expr->isLet()){
            expr = expandLet(expr);
        }
        std::vector<std::shared_ptr<DAGNode>> clauses;
        // collect all atoms
        boost::unordered_set<std::shared_ptr<DAGNode>> atoms;
        collectAtoms(expr, atoms);

        // create a new variable for each atom
        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> atom_map;
        for (auto& atom : atoms) {
            std::shared_ptr<DAGNode> new_var = mkTempVar(BOOL_SORT);
            atom_map[atom] = new_var;
            // add to cnf_map
            cnf_map[atom] = new_var;
            cnf_map[mkNot(atom)] = mkNot(new_var);
        }

        // use Tseitin transformation for each atom
        for(auto& atom : atoms){
            // not(atom) -> (new_var)
            clauses.emplace_back(mkOr({mkNot(atom), atom_map[atom]}));
            // (new_var) -> (atom)
            clauses.emplace_back(mkOr({mkNot(atom_map[atom]), atom}));
        }

        // create a new formula with Tseitin variables
        std::shared_ptr<DAGNode> new_expr = replaceAtoms(expr, atom_map);

        // currently, the new formula has only boolean variables
        std::shared_ptr<DAGNode> tseitin_cnf = toTseitinCNF(new_expr, clauses);
        clauses.emplace_back(tseitin_cnf);

        // if there is only one clause, return it directly
        if (clauses.size() == 1) {
            cnf_map[expr] = clauses[0];
            return clauses[0];
        }
        // otherwise, create an AND node, containing all OR clauses
        std::shared_ptr<DAGNode> cnf = mkAnd(clauses);
        cnf_map[expr] = cnf;
        return cnf;
    }

    // convert a list of expressions to DNF (a large OR node, whose children are all AND terms)
    std::shared_ptr<DAGNode> Parser::toDNF(std::vector<std::shared_ptr<DAGNode>> exprs) {
        std::shared_ptr<DAGNode> result = mkAnd(exprs);
        std::shared_ptr<DAGNode> dnf = toDNF(result);
        dnf_map[result] = dnf;
        return dnf;
    }

    // convert a single expression to DNF
    std::shared_ptr<DAGNode> Parser::toDNF(std::shared_ptr<DAGNode> expr) {
        // eliminate xor, implies, ite, eq, distinct
        expr = toDNFEliminateAll(expr);
        expr = toNNF(expr);
        std::shared_ptr<DAGNode> dnf = applyDNFDistributiveLaw(expr);
        dnf_map[expr] = dnf;
        return dnf;
    }

    // eliminate xor, implies, ite, eq, distinct
    std::shared_ptr<DAGNode> Parser::toDNFEliminateAll(std::shared_ptr<DAGNode> expr){
        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> visited;
        bool is_changed = true;
        while(is_changed){
            is_changed = false;
            expr = toDNFEliminateAll(expr, visited, is_changed);
        }
        return expr;
    }

    std::shared_ptr<DAGNode> Parser::toDNFEliminateAll(std::shared_ptr<DAGNode> expr,
                                                        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& visited,
                                                        bool& is_changed){
        if(expr->isLiteral() || expr->isAtom() || expr->isTempVar()){ return expr; }
        if(visited.find(expr) != visited.end()){
            return visited[expr];
        }

        if(expr->isLet()){
            std::shared_ptr<DAGNode> result = toDNFEliminateAll(expandLet(expr), visited, is_changed);
            visited[expr] = result;
            return result;
        }

        if(expr->isNot()){
            bool child_changed = false;
            std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(0), visited, child_changed);
            if(child_changed){
                is_changed = true;
            }
            std::shared_ptr<DAGNode> result = mkNot(child);
            visited[expr] = result;
            return result;
        }
        else if(expr->isAnd()){
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                bool child_changed = false;
                std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(i), visited, child_changed);
                if(child_changed){
                    is_changed = true;
                }
                children.emplace_back(child);
            }
            if(is_changed){
                std::shared_ptr<DAGNode> result = mkAnd(children);
                visited[expr] = result;
                return result;
            }
            else{
                // no change, return the original expression
                visited[expr] = expr;
                return expr;
            }
        }
        else if(expr->isOr()){
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                bool child_changed = false;
                std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(i), visited, child_changed);
                if(child_changed){
                    is_changed = true;
                }
                children.emplace_back(child);
            }
            if(is_changed){
                std::shared_ptr<DAGNode> result = mkOr(children);
                visited[expr] = result;
                return result;
            }
            else{
                // no change, return the original expression
                visited[expr] = expr;
                return expr;
            }
        }
        else if(expr->isImplies()){
            // A -> B <=> ¬A ∨ B
            std::shared_ptr<DAGNode> A = toDNFEliminateAll(expr->getChild(0), visited, is_changed);
            std::shared_ptr<DAGNode> B = toDNFEliminateAll(expr->getChild(1), visited, is_changed);
            std::shared_ptr<DAGNode> result = mkOr({mkNot(A), B});
            is_changed = true;
            visited[expr] = result;
            return result;
        }
        else if(expr->isIte()){
            // if a then b else c <=> (¬a ∨ b) ∧ (a ∨ c)
            std::shared_ptr<DAGNode> A = toDNFEliminateAll(expr->getChild(0), visited, is_changed);
            std::shared_ptr<DAGNode> B = toDNFEliminateAll(expr->getChild(1), visited, is_changed);
            std::shared_ptr<DAGNode> C = toDNFEliminateAll(expr->getChild(2), visited, is_changed);
            std::shared_ptr<DAGNode> result = mkAnd({mkOr({mkNot(A), B}), mkOr({A, C})});
            is_changed = true;
            visited[expr] = result;
            return result;
        }
        else if(expr->isXor()){
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(i), visited, is_changed);
                children.emplace_back(child);
            }
            std::shared_ptr<DAGNode> result = toDNFEliminateXOR(children);
            is_changed = true;
            visited[expr] = result;
            return result;
        }
        else if(expr->isEq()){
            // check if all children are boolean types
            bool all_bool = true;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                if(!expr->getChild(i)->getSort()->isBool()){
                    all_bool = false;
                    break;
                }
            }

            if(!all_bool){
                err_all(ERROR_TYPE::ERR_TYPE_MIS, "eq with non-boolean variables");
                return mkFalse();
            }
            
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                bool child_changed = false;
                std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(i), visited, child_changed);
                if(child_changed){
                    is_changed = true;
                }
                children.emplace_back(child);
            }
            
            std::shared_ptr<DAGNode> result = toDNFEliminateEq(children);
            is_changed = true;
            visited[expr] = result;
            return result;
        }
        else if(expr->isDistinct()){
            // check if all children are boolean types
            bool all_bool = true;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                if(!expr->getChild(i)->getSort()->isBool()){
                    all_bool = false;
                    break;
                }
            }

            if(!all_bool){
                err_all(ERROR_TYPE::ERR_TYPE_MIS, "distinct with non-boolean variables");
                return mkFalse();
            }
            
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                bool child_changed = false;
                std::shared_ptr<DAGNode> child = toDNFEliminateAll(expr->getChild(i), visited, child_changed);
                if(child_changed){
                    is_changed = true;
                }
                children.emplace_back(child);
            }
            
            // if boolean type and more than 2 variables, distinct is unsatisfiable
            if(all_bool && children.size() > 2){
                std::shared_ptr<DAGNode> result = mkFalse();
                is_changed = true;
                visited[expr] = result;
                return result;
            }
            
            std::shared_ptr<DAGNode> result = toDNFEliminateDistinct(children);
            is_changed = true;
            visited[expr] = result;
            return result;
        }
        else{
            err_all(ERROR_TYPE::ERR_TYPE_MIS, "unsupported node type: " + kindToString(expr->getKind()));
            return mkFalse();
        }
    }

    // apply DNF distributive law
    std::shared_ptr<DAGNode> Parser::applyDNFDistributiveLaw(std::shared_ptr<DAGNode> expr) {
        boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> visited;
        std::shared_ptr<DAGNode> result = applyDNFDistributiveLawRec(expr, visited);
        return flattenDNF(result);
    }

    // flatten DNF expression, eliminate nested OR/AND
    std::shared_ptr<DAGNode> Parser::flattenDNF(std::shared_ptr<DAGNode> expr) {
        if (!expr->isOr() && !expr->isAnd()) {
            return expr;
        }
        
        std::vector<std::shared_ptr<DAGNode>> flattened;
        
        if (expr->isOr()) {
            // collect all OR node children, eliminate nested OR
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child = expr->getChild(i);
                if (child->isOr()) {
                    // if child is also OR, expand it
                    for (size_t j = 0; j < child->getChildrenSize(); j++) {
                        flattened.push_back(flattenDNF(child->getChild(j)));
                    }
                } else {
                    flattened.push_back(flattenDNF(child));
                }
            }
            
            // remove duplicates
            boost::unordered_set<std::shared_ptr<DAGNode>> unique_terms;
            std::vector<std::shared_ptr<DAGNode>> result;
            for (auto& term : flattened) {
                if (unique_terms.find(term) == unique_terms.end()) {
                    unique_terms.insert(term);
                    result.push_back(term);
                }
            }
            
            if (result.size() == 1) {
                return result[0];
            }
            return mkOr(result);
        }
        else { // expr->isAnd()
            // collect all AND node children, eliminate nested AND
            for (size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child = expr->getChild(i);
                if (child->isAnd()) {
                    // if child is also AND, expand it
                    for (size_t j = 0; j < child->getChildrenSize(); j++) {
                        flattened.push_back(flattenDNF(child->getChild(j)));
                    }
                } else {
                    flattened.push_back(flattenDNF(child));
                }
            }
            
            // remove duplicates
            boost::unordered_set<std::shared_ptr<DAGNode>> unique_literals;
            std::vector<std::shared_ptr<DAGNode>> result;
            for (auto& literal : flattened) {
                if (unique_literals.find(literal) == unique_literals.end()) {
                    unique_literals.insert(literal);
                    result.push_back(literal);
                }
            }
            
            if (result.size() == 1) {
                return result[0];
            }
            return mkAnd(result);
        }
    }

    // recursive implementation of DNF distributive law
    std::shared_ptr<DAGNode> Parser::applyDNFDistributiveLawRec(
            std::shared_ptr<DAGNode> expr,
            boost::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>>& visited) {
        
        // base case: literal or visited node
        if(expr->isLiteral() || expr->isAtom() || expr->isTempVar()) { 
            return expr; 
        }
        
        if(visited.find(expr) != visited.end()) {
            return visited[expr];
        }

        if(expr->isLet()){
            std::shared_ptr<DAGNode> result = applyDNFDistributiveLawRec(expandLet(expr), visited);
            visited[expr] = result;
            return result;
        }
        
        if(expr->isNot()) {
            // negation only appears in front of literals in NNF
            return expr;
        }
        else if(expr->isOr()) {
            // recursively process each child
            std::vector<std::shared_ptr<DAGNode>> children;
            bool has_or_child = false;
            
            for(size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child = applyDNFDistributiveLawRec(expr->getChild(i), visited);
                if(child->isOr()) {
                    has_or_child = true;
                }
                children.push_back(child);
            }
            
            // if there are no nested OR children, return the OR node directly
            if(!has_or_child) {
                std::shared_ptr<DAGNode> result = mkOr(children);
                visited[expr] = result;
                return result;
            }
            
            // otherwise, flatten the nested OR node
            std::vector<std::shared_ptr<DAGNode>> flattened;
            for(auto& child : children) {
                if(child->isOr()) {
                    for(size_t j = 0; j < child->getChildrenSize(); j++) {
                        flattened.push_back(child->getChild(j));
                    }
                } else {
                    flattened.push_back(child);
                }
            }
            
            std::shared_ptr<DAGNode> result = mkOr(flattened);
            visited[expr] = result;
            return result;
        }
        else if(expr->isAnd()) {
            // recursively process each child, and divide into OR children and non-OR children
            std::vector<std::shared_ptr<DAGNode>> andChildren;
            std::vector<std::shared_ptr<DAGNode>> orChildren;
            
            for(size_t i = 0; i < expr->getChildrenSize(); i++) {
                std::shared_ptr<DAGNode> child = applyDNFDistributiveLawRec(expr->getChild(i), visited);
                if(child->isOr()) {
                    orChildren.push_back(child);
                } else {
                    andChildren.push_back(child);
                }
            }
            
            // if there are no OR children, return the AND node directly
            if(orChildren.empty()) {
                std::shared_ptr<DAGNode> result = mkAnd(andChildren);
                visited[expr] = result;
                return result;
            }
            
            // apply distributive law
            // initialize distributedOr as the first OR child
            std::shared_ptr<DAGNode> distributedOr = orChildren[0];
            
            // apply distributive law to each subsequent OR child
            for(size_t i = 1; i < orChildren.size(); i++) {
                std::shared_ptr<DAGNode> currentOr = orChildren[i];
                std::vector<std::shared_ptr<DAGNode>> newOrTerms;
                
                // distributive law: (a∨b∨c)∧(d∨e) → (a∧d)∨(a∧e)∨(b∧d)∨(b∧e)∨(c∧d)∨(c∧e)
                for(size_t j = 0; j < distributedOr->getChildrenSize(); j++) {
                    for(size_t k = 0; k < currentOr->getChildrenSize(); k++) {
                        std::vector<std::shared_ptr<DAGNode>> andTerm;
                        
                        // copy previous AND terms
                        if(distributedOr->getChild(j)->isAnd()) {
                            for(size_t l = 0; l < distributedOr->getChild(j)->getChildrenSize(); l++) {
                                andTerm.push_back(distributedOr->getChild(j)->getChild(l));
                            }
                        } else {
                            andTerm.push_back(distributedOr->getChild(j));
                        }
                        
                        // add new AND terms
                        if(currentOr->getChild(k)->isAnd()) {
                            for(size_t l = 0; l < currentOr->getChild(k)->getChildrenSize(); l++) {
                                andTerm.push_back(currentOr->getChild(k)->getChild(l));
                            }
                        } else {
                            andTerm.push_back(currentOr->getChild(k));
                        }
                        
                        newOrTerms.push_back(mkAnd(andTerm));
                    }
                }
                
                distributedOr = mkOr(newOrTerms);
            }
            
            // merge non-OR children into each OR term
            if(!andChildren.empty()) {
                std::vector<std::shared_ptr<DAGNode>> finalOrTerms;
                
                for(size_t i = 0; i < distributedOr->getChildrenSize(); i++) {
                    std::vector<std::shared_ptr<DAGNode>> andTerm;
                    
                    // add original AND terms
                    if(distributedOr->getChild(i)->isAnd()) {
                        for(size_t j = 0; j < distributedOr->getChild(i)->getChildrenSize(); j++) {
                            andTerm.push_back(distributedOr->getChild(i)->getChild(j));
                        }
                    } else {
                        andTerm.push_back(distributedOr->getChild(i));
                    }
                    
                    // add additional AND children
                    andTerm.insert(andTerm.end(), andChildren.begin(), andChildren.end());
                    
                    finalOrTerms.push_back(mkAnd(andTerm));
                }
                
                distributedOr = mkOr(finalOrTerms);
            }
            
            visited[expr] = distributedOr;
            return distributedOr;
        }
        else {
            // other node types (should not appear, because all complex operators have been processed through NNF conversion)
            err_all(ERROR_TYPE::ERR_TYPE_MIS, "unsupported node type in DNF conversion: " + kindToString(expr->getKind()));
            return expr;
        }
    }
        

    // eliminate xor
    std::shared_ptr<DAGNode> Parser::toDNFEliminateXOR(const std::vector<std::shared_ptr<DAGNode>>& children) {
        if (children.size() == 0) {
            return mkFalse(); // return false
        }
        if (children.size() == 1) {
            return toDNFEliminateAll(children[0]); // return the single child
        }
        if (children.size() == 2) {
            // A XOR B = (A ∧ ¬B) ∨ (¬A ∧ B)
            std::shared_ptr<DAGNode> A = children[0];
            std::shared_ptr<DAGNode> B = children[1];
            return mkOr({
                mkAnd({A, mkNot(B)}),
                mkAnd({mkNot(A), B})
            });
        }
        
        // multiple variables XOR: recursive implementation
        // (A XOR B XOR C) = (A XOR B) XOR C
        std::vector<std::shared_ptr<DAGNode>> first_part(children.begin(), children.begin() + children.size() - 1);
        std::shared_ptr<DAGNode> rest = toDNFEliminateXOR(first_part);
        std::shared_ptr<DAGNode> last = children.back();
        
        return mkOr({
            mkAnd({rest, mkNot(last)}),
            mkAnd({mkNot(rest), last})
        });
    }

    // eliminate eq
    std::shared_ptr<DAGNode> Parser::toDNFEliminateEq(const std::vector<std::shared_ptr<DAGNode>>& children) {
        if (children.size() <= 1) {
            return mkTrue(); // return true
        }
        if (children.size() == 2) {
            // A = B <=> (A ∧ B) ∨ (¬A ∧ ¬B)
            std::shared_ptr<DAGNode> A = children[0];
            std::shared_ptr<DAGNode> B = children[1];
            return mkOr({
                mkAnd({A, B}),
                mkAnd({mkNot(A), mkNot(B)})
            });
        }
        
        // multiple variables eq: A = B = C = ... <=> (A = B) ∧ (B = C) ∧ ...
        std::vector<std::shared_ptr<DAGNode>> eq_terms;
        for (size_t i = 0; i < children.size() - 1; i++) {
            std::vector<std::shared_ptr<DAGNode>> pair = {children[i], children[i+1]};
            eq_terms.push_back(toDNFEliminateEq(pair));
        }
        
        return mkAnd(eq_terms);
    }

    // eliminate distinct
    std::shared_ptr<DAGNode> Parser::toDNFEliminateDistinct(const std::vector<std::shared_ptr<DAGNode>>& children) {
        if (children.size() <= 1) {
            return mkTrue(); // return true
        }
        if (children.size() == 2) {
            // A ≠ B <=> (A ∧ ¬B) ∨ (¬A ∧ B)
            std::shared_ptr<DAGNode> A = children[0];
            std::shared_ptr<DAGNode> B = children[1];
            return mkOr({
                mkAnd({A, mkNot(B)}),
                mkAnd({mkNot(A), B})
            });
        }
        
        // multiple variables distinct: at least one pair is distinct
        // for boolean variables, more than 2 variables distinct is unsatisfiable
        return mkFalse(); // boolean domain, more than 2 variables distinct is unsatisfiable
    }

    // convert a list of expressions to NNF
    std::shared_ptr<DAGNode> Parser::toNNF(std::vector<std::shared_ptr<DAGNode>> exprs) {
        std::shared_ptr<DAGNode> target = mkAnd(exprs);
        std::shared_ptr<DAGNode> result = toNNF(target);
        nnf_map[target] = result;
        return result;
    }

    // convert a single expression to NNF
    std::shared_ptr<DAGNode> Parser::toNNF(std::shared_ptr<DAGNode> expr, bool is_not) {
        // handle constants and variables
        if(expr->isCBool() || expr->isVBool()){
            if(is_not){
                return mkNot(expr);
            }
            return expr;
        }

        if(expr->isLet()){
            std::shared_ptr<DAGNode> result = toNNF(expandLet(expr), is_not);
            nnf_map[expr] = result;
            return result;
        }

        // handle NOT node
        if(expr->isNot()){
            // negate propagation: ¬¬A → A
            return toNNF(expr->getChild(0), !is_not);
        }
        
        // handle AND node
        if(expr->isAnd()){
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                // recursively handle each child node
                children.emplace_back(toNNF(expr->getChild(i), is_not));
            }
            
            // apply De Morgan's law based on is_not: ¬(A∧B) → (¬A∨¬B)
            if(is_not){
                return mkOr(children);
            } else {
                return mkAnd(children);
            }
        }
        
        // handle OR node
        if(expr->isOr()){
            std::vector<std::shared_ptr<DAGNode>> children;
            for(size_t i = 0; i < expr->getChildrenSize(); i++){
                // recursively handle each child node
                children.emplace_back(toNNF(expr->getChild(i), is_not));
            }
            
            // apply De Morgan's law based on is_not: ¬(A∨B) → (¬A∧¬B)
            if(is_not){
                return mkAnd(children);
            } else {
                return mkOr(children);
            }
        }
        
        // handle IMPLIES node
        if(expr->isImplies()){
            std::shared_ptr<DAGNode> A = expr->getChild(0);
            std::shared_ptr<DAGNode> B = expr->getChild(1);
            
            if(is_not){
                // ¬(A→B) → (A∧¬B)
                return mkAnd({toNNF(A, false), toNNF(B, true)});
            } else {
                // A→B → (¬A∨B)
                return mkOr({toNNF(A, true), toNNF(B, false)});
            }
        }
        
        // handle ITE node (conditional expression)
        if(expr->isIte() && expr->getChild(1)->getSort()->isBool()){
            std::shared_ptr<DAGNode> A = expr->getChild(0);  // condition
            std::shared_ptr<DAGNode> B = expr->getChild(1);  // then branch
            std::shared_ptr<DAGNode> C = expr->getChild(2);  // else branch
            
            if(is_not){
                // ¬(if A then B else C) → (A∧¬B)∨(¬A∧¬C)
                return mkOr({
                    mkAnd({toNNF(A, false), toNNF(B, true)}),
                    mkAnd({toNNF(A, true), toNNF(C, true)})
                });
            } else {
                // if A then B else C → (A∧B)∨(¬A∧C)
                return mkOr({
                    mkAnd({toNNF(A, false), toNNF(B, false)}),
                    mkAnd({toNNF(A, true), toNNF(C, false)})
                });
            }
        }
        
        // handle other types of nodes (e.g., arithmetic operators)
        if(is_not){
            return mkNot(expr);
        }
        return expr;
    }

    // convert a single expression to NNF
    std::shared_ptr<DAGNode> Parser::toNNF(std::shared_ptr<DAGNode> expr) {
        if(nnf_map.find(expr) != nnf_map.end()){
            return nnf_map[expr];
        }
        std::shared_ptr<DAGNode> result = toNNF(expr, false);
        nnf_map[expr] = result;
        return result;
    }
}
