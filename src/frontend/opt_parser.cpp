/* -*- Source -*-
 *
 * An SMT/OMT Parser (OMT part)
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

#include "somtparser/frontend/parser.h"

namespace SOMTParser{
    
    KEYWORD Parser::attemptParseKeywords(){
        if(*bufptr == ':'){
            // :id
            return parseKeyword();
        }
        else{
            return KEYWORD::KW_NULL;
        }
    }
    // (assert-soft <expr> [:weight <numeral>] [:id <symbol>])
    void Parser::parseAssertSoft(){
        size_t key_ln = line_number;
        std::string grp_id = "";
        std::string weight = "";
        size_t params_size = 2;
        KEYWORD key = KEYWORD::KW_NULL;
        // NOTE: it can be (assert-soft <expr> [:id <symbol>] [:id <symbol>]), but only the last one will be used
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :id or :weight
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else if(key == KEYWORD::KW_WEIGHT){
                // :weight
                weight = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }
        std::shared_ptr<DAGNode> assert_expr = parseExpr();
        size_t index = context_.soft_assertions.size();
        context_.soft_assertions.emplace_back(assert_expr);
        
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :id or :weight
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else if(key == KEYWORD::KW_WEIGHT){
                // :weight
                weight = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }

        // add the weight
        if(weight != ""){
            if(TypeChecker::isInt(weight)){
                context_.soft_weights.emplace_back(mkConstInt(weight));
            }
            else if(TypeChecker::isReal(weight)){
                context_.soft_weights.emplace_back(mkConstReal(weight));
            }
            else{
                err_param_nnum("Weight", key_ln);
            }
        }
        else{
            context_.soft_weights.emplace_back(mkConstInt(1));
        }
        // add the group id
        if(grp_id != ""){
            if(context_.soft_assertion_groups.find(grp_id) == context_.soft_assertion_groups.end()){
                context_.soft_assertion_groups.insert(std::pair<std::string, std::unordered_set<size_t>>(grp_id, {index}));
            }
            else{
                context_.soft_assertion_groups[grp_id].insert(index);
            }
        }
    }
    // (maximize <expr> [:comp <symbol>] [:epsilon <symbol>] [:M <symbol>] [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMaximize(){
        return parseSingleObj(OPT_KIND::OPT_MAXIMIZE);
    }
    // (minimize <expr> [:comp <symbol>] [:epsilon <symbol>] [:M <symbol>] [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMinimize(){
        return parseSingleObj(OPT_KIND::OPT_MINIMIZE);
    }
    // (maxsat <expr> [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMaxsat(){
        return parseSingleObj(OPT_KIND::OPT_MAXSAT);
    }
    // (minsat <expr> [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMinsat(){
        return parseSingleObj(OPT_KIND::OPT_MINSAT);
    }
    std::shared_ptr<Objective> Parser::parseSingleObj(const OPT_KIND& opt_type){
        size_t key_ln = line_number;
        std::string comp = "";
        std::string epsilon = "";
        std::string M = "";
        std::string grp_id = "";
        size_t params_size = 4;
        KEYWORD key = KEYWORD::KW_NULL;
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :comp, :epsilon, :M or :id
            else if(key == KEYWORD::KW_COMP){
                // :comp
                comp = getSymbol();
            }
            else if(key == KEYWORD::KW_EPSILON){
                // :epsilon
                epsilon = getSymbol();
            }
            else if(key == KEYWORD::KW_M){
                // :M
                M = getSymbol();
            }
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }
        std::shared_ptr<DAGNode> min_expr = parseExpr();
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :comp, :epsilon, :M or :id
            else if(key == KEYWORD::KW_COMP){
                // :comp
                comp = getSymbol();
            }
            else if(key == KEYWORD::KW_EPSILON){
                // :epsilon
                epsilon = getSymbol();
            }
            else if(key == KEYWORD::KW_M){
                // :M
                M = getSymbol();
            }
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }

        if(opt_type == OPT_KIND::OPT_MAXSAT || opt_type == OPT_KIND::OPT_MINSAT){
            return getObjectiveManager()->addSingleObjective(opt_type, grp_id);
        }
        COMP_KIND comp_op = comp == "" ? getDefaultCompareOperator(getOptions()->logic, opt_type) : getCompareOperator(comp);
        std::shared_ptr<DAGNode> epsilon_node = epsilon == "" ? NodeManager::NULL_NODE : NodeManager::EPSILON_NODE;
        std::shared_ptr<DAGNode> M_node = M == "" ? NodeManager::NULL_NODE : NodeManager::INT_INF_NODE;
        return getObjectiveManager()->addSingleObjective(opt_type, min_expr, comp_op, epsilon_node, M_node, grp_id);
    }
    // NOTE: the most internal id will be used
    // (define-objective <symbol> signle_opt [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseDefObj(){
        std::string obj_name = getSymbol();
        size_t key_ln = line_number;
        std::string grp_id = "";
        KEYWORD key = attemptParseKeywords();
        if(key == KEYWORD::KW_ID){
            grp_id = getSymbol();
        }
        // single_opt
        parseLpar();
        std::shared_ptr<Objective> objective = nullptr;
        std::string opt_type = getSymbol();
        if(opt_type == "maximize"){
            objective = parseMaximize();
        }
        else if(opt_type == "minimize"){
            objective = parseMinimize();
        }
        else{
            err_unkwn_sym("Objective type", key_ln);
        }
        parseRpar();
        // mk objective
        getObjectiveManager()->registerNamed(obj_name, objective);
        return objective;
    }

    std::shared_ptr<Objective> Parser::parseMultiObj(const OPT_KIND& opt_type){
        std::string grp_id = "";
        KEYWORD key = attemptParseKeywords();
        if(key == KEYWORD::KW_ID){
            grp_id = getSymbol();
        }
        // (<symbol>+)
        parseLpar();
        std::vector<std::string> obj_names;
        while(*bufptr != ')'){
            obj_names.emplace_back(getSymbol());
        }
        parseRpar();
        
        key = attemptParseKeywords();
        if(key == KEYWORD::KW_ID){
            std::string grp_id = getSymbol();
        }
        
        // mk multi-objective
        std::shared_ptr<Objective> objective = getObjectiveManager()->createObjective(opt_type, grp_id);
        for(auto& obj_name : obj_names){
            objective->addObjective(getObjectiveManager()->getObjective(obj_name));
        }
        getObjectiveManager()->addObjective(objective);
        return objective;
    }
    // (lex-optimize (<symbol>+) [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseLexOpt(){
        return parseMultiObj(OPT_KIND::OPT_LEX_OPTIMIZE);
    }
    // (pareto-optimize (<symbol>+) [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseParetoOpt(){
        return parseMultiObj(OPT_KIND::OPT_PARETO_OPTIMIZE);
    }
    // (box-optimize (<symbol>+) [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseBoxOpt(){
        return parseMultiObj(OPT_KIND::OPT_BOX_OPTIMIZE);
    }
    // (minmax (<symbol>+) [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMinmax(){
        return parseMultiObj(OPT_KIND::OPT_MINMAX);
    }
    // (maxmin (<symbol>+) [:id <symbol>])
    std::shared_ptr<Objective> Parser::parseMaxmin(){
        return parseMultiObj(OPT_KIND::OPT_MAXMIN);
    }

    // (optimize (<symbol>+) [:id <symbol>] [:opt_kind <symbol>])
    std::shared_ptr<Objective> Parser::parseOptimize(){
        size_t key_ln = line_number;
        std::string grp_id = "";
        std::string opt_kind = "";
        size_t params_size = 4;
        KEYWORD key = KEYWORD::KW_NULL;
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :id or :opt_kind
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else if(key == KEYWORD::KW_OPT_KIND){
                // :opt_kind
                opt_kind = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }

        // (<symbol>+)
        parseLpar();
        std::vector<std::string> obj_names;
        while(*bufptr != ')'){
            obj_names.emplace_back(getSymbol());
        }
        parseRpar();
        
        for(size_t i=0;i<params_size;i++){
            key = attemptParseKeywords();
            if(key == KEYWORD::KW_NULL){ break; } // no :id or :opt_kind
            else if(key == KEYWORD::KW_ID){
                // :id
                grp_id = getSymbol();
            }
            else if(key == KEYWORD::KW_OPT_KIND){
                // :opt_kind
                opt_kind = getSymbol();
            }
            else{
                // error
                err_keyword("Error: wrong keywords at line ", key_ln);
            }
        }

        // mk multi-objective (not added to list; caller may use returned objective)
        std::shared_ptr<Objective> objective = getObjectiveManager()->createObjective(
            opt_kind == "" ? OPT_KIND::OPT_LEX_OPTIMIZE : getOptKind(opt_kind), grp_id);
        for(auto& obj_name : obj_names){
            objective->addObjective(getObjectiveManager()->getObjective(obj_name));
        }
        return objective;
    }
}