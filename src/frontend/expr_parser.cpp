/* -*- Source -*-
 *
 * An SMT/OMT Parser (Expression part)
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
#include "somtparser/core/timing.h"
#include <stack>
#include <cctype>

namespace SOMTParser{

    // Internal marker used to route ((_ update <selector>) t v) through the
    // regular operator path while carrying the selector name. The leading
    // control character cannot appear in an SMT-LIB symbol, so this can never
    // be confused with a user-declared function.
    static constexpr const char* DT_UPDATE_PREFIX = "\x01update-";

    // State of the parser
    enum class FrameState {
        Start,
        ReadingHead,           // reading head symbol
        ProcessingSpecial,     // processing special syntax
        ProcessingParams,      // processing parameter list
        ProcessingBindings,    // processing let bindings
        ProcessingLetBody,     // processing let body
        ProcessingQuantVars,   // processing quantifier variables
        ProcessingQuantBody,   // processing quantifier body
        ProcessingParamFuncArgs, // processing parameter function arguments
        ProcessingParamFuncParams, // processing parameter function parameters
        Finish
    };

    // special processing type
    enum class SpecialType {
        None,
        Let,
        Exists,
        Forall,
        ParamFunc,
        BV,
        Underscore
    };

    // Frame of the parser
    struct ExprFrame {
        FrameState state = FrameState::Start;
        SpecialType special_type = SpecialType::None;
        std::string headSymbol;                      // head symbol
        std::string second_symbol;                   // f of "(_ f …)"
        std::vector<std::shared_ptr<DAGNode>> func_args;  // function arguments
        std::vector<std::shared_ptr<DAGNode>> oper_params;// operator parameters
        int line = 0;                                // line number
        std::shared_ptr<DAGNode> result;             // result

        ExprFrame() {
            // pre-allocate container space, reduce dynamic expansion overhead
            // according to common expressions, most operators have 2-4 args and a lot of params
            func_args.reserve(4);
            oper_params.reserve(64);
        }
    };
    
	// expr ::= const | func | (<identifier> <expr>+)
	std::shared_ptr<DAGNode> Parser::parseExpr() {
        TIME_FUNC();
        std::stack<ExprFrame> st;
        st.push(ExprFrame());

        while(!st.empty()){
            ExprFrame &frame = st.top();
            switch(frame.state){
                case FrameState::Start:{
                    TIME_BLOCK("parseExpr FrameState::Start");
                    // handle the simplest symbol or constant
                    if(*bufptr != '('){
                        size_t ln = line_number;
                        std::string sym = getSymbol();
                        auto node = parseConstFunc(sym);
                        if(node->isErr()) err_unkwn_sym(sym, ln);
                        frame.result = node;
                        frame.state = FrameState::Finish;
                        break;
                    }

                    // complex expression
                    parseLpar();
                    frame.line = line_number;

                    // check ((_ f args) ...) or ((as const T) value)
                    if(*bufptr == '('){
                        parseLpar();
                        std::string s = getSymbol();
                        if(s == "_"){
                            std::string idx = getSymbol();
                            if(idx == "is"){
                                std::string ctor = getSymbol();
                                parseRpar();
                                frame.headSymbol = "_";
                                frame.second_symbol = "is-" + ctor;
                                frame.state = FrameState::ProcessingParams;
                                break;
                            }
                            // ((_ update <selector>) t v): like (_ is C), the
                            // index is a selector *symbol* rather than a
                            // numeral, so it cannot go through the generic
                            // ParamFunc path (which parses indices as terms).
                            // Ported from the SMTStabilizer fork.
                            if(idx == "update"){
                                std::string sel = getSymbol();
                                parseRpar();
                                frame.headSymbol = "_";
                                frame.second_symbol = DT_UPDATE_PREFIX + sel;
                                frame.state = FrameState::ProcessingParams;
                                break;
                            }
                            frame.special_type = SpecialType::ParamFunc;
                            frame.second_symbol = idx;
                            frame.state = FrameState::ProcessingParamFuncArgs;
                            break;
                        }
                        else if(s == "as"){
                            // Handle ((as const T) value) pattern
                            std::string second = getSymbol();
                            if(second == "const"){
                                // Parse array sort
                                std::shared_ptr<Sort> array_sort = parseSort();
                                parseRpar(); // close (as const T)
                                
                                // Parse value
                                std::shared_ptr<DAGNode> value = parseExpr();
                                
                                // Create constant array
                                frame.result = mkConstArray(array_sort, value);
                                parseRpar(); // close outer parentheses
                                frame.state = FrameState::Finish;
                                break;
                            }
                            else{
                                err_unkwn_sym("as " + second, frame.line);
                                frame.result = mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
                                frame.state = FrameState::Finish;
                                break;
                            }
                        }
                        else if(s == "root-obj"){
                            // (root-obj (+ (^ x 2) (- 3)) 1)
                            // (root-obj <expr> <index>)
                            // Enable placeholder variables for polynomial expressions
                            bool old_allow_placeholder = allow_placeholder_vars;
                            std::shared_ptr<Sort> old_placeholder_sort = placeholder_var_sort;
                            allow_placeholder_vars = true;
                            placeholder_var_sort = SortManager::REAL_SORT; // Default to Real sort for polynomials
                            
                            std::shared_ptr<DAGNode> expr = parseExpr();
                            std::string index_str = getSymbol();
                            parseRpar(); // close (root-obj ...)
                            
                            // Restore original settings
                            allow_placeholder_vars = old_allow_placeholder;
                            placeholder_var_sort = old_placeholder_sort;
                            
                            // Parse index as integer
                            int index = std::stoi(index_str);
                            
                            // Create root-obj node
                            frame.result = mkRootObj(expr, index);
                            frame.state = FrameState::Finish;
                            break;
                        }
                        else if(s == "root-of-with-interval"){
                            // ((root-of-with-interval (coeffs 1 (- 2)) 1 2))
                            // Parse (coeffs ...) as a whole
                            parseLpar(); // for (coeffs ...)
                            std::string coeffs_tag = getSymbol(); // read "coeffs"
                            // We don't validate coeffs_tag, just skip it
                            
                            // Parse coefficients
                            std::vector<std::shared_ptr<DAGNode>> coeffs_list;
                            scanToNextSymbol(); // Skip whitespace before checking for ')'
                            while(*bufptr != ')'){
                                std::shared_ptr<DAGNode> coeff = parseExpr();
                                coeffs_list.push_back(coeff);
                                scanToNextSymbol(); // Skip whitespace before checking for ')' again
                            }
                            parseRpar(); // close (coeffs ...)
                            
                            // Parse lower bound
                            std::shared_ptr<DAGNode> lower_bound = parseExpr();
                            
                            // Parse upper bound
                            std::shared_ptr<DAGNode> upper_bound = parseExpr();
                            
                            parseRpar(); // close (root-of-with-interval ...)
                            
                            // Create root-of-with-interval node
                            frame.result = mkRootOfWithInterval(coeffs_list, lower_bound, upper_bound);
                            frame.state = FrameState::Finish;
                            break;
                        }
                        else{
                            err_unkwn_sym(s, frame.line);
                            frame.result = mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
                            frame.state = FrameState::Finish;
                            break;
                        }
                    }

                    // read the head symbol
                    std::string head = getSymbol();
                    frame.headSymbol = head;

                    if(head == "exists" || head == "forall"){
                        in_quantifier_scope = true;
                        quant_nesting_depth++;
                        auto res = parseQuant(head);
                        parseRpar();
                        frame.result = res;
                        frame.state = FrameState::Finish;
                        quant_nesting_depth--;
                        if(quant_nesting_depth == 0){
                            in_quantifier_scope = false;
                        }
                    }
                    else if(head == "_"){
                        std::string second = getSymbol();
                        if(second.size() >= 2 && second[0]=='b' && second[1]=='v'){
                            std::string width_str = getSymbol();
                            size_t width = std::stoul(width_str);
                            std::string num = second.substr(2);
                            frame.result = mkConstBv(num, width);
                            parseRpar();
                            frame.state = FrameState::Finish;
                        }
                        // Handle floating point special constants: (_ +zero eb sb), (_ -zero eb sb), (_ +oo eb sb), (_ -oo eb sb), (_ NaN eb sb)
                        else if(second == "+zero" || second == "-zero" || second == "+oo" || second == "-oo" || second == "NaN" || second == "+NaN" || second == "-NaN"){
                            std::string eb_str = getSymbol();
                            std::string sb_str = getSymbol();
                            size_t eb = std::stoul(eb_str);
                            size_t sb = std::stoul(sb_str);
                            
                            std::shared_ptr<Sort> fp_sort = getSortManager()->createFPSort(eb, sb);
                            
                            // Use the appropriate mk* function which will create nodes with full names
                            if (second == "NaN" || second == "+NaN" || second == "-NaN") {
                                frame.result = mkNaN(fp_sort);
                            } else if (second == "+oo") {
                                frame.result = mkPosInfinity(fp_sort);
                            } else if (second == "-oo") {
                                frame.result = mkNegInfinity(fp_sort);
                            } else {
                                // For +zero and -zero, create as NT_CONST with full name
                                std::string const_name = "(_ " + second + " " + eb_str + " " + sb_str + ")";
                                frame.result = getNodeManager()->createNode(fp_sort, NODE_KIND::NT_CONST, const_name);
                            }
                            
                            parseRpar();
                            frame.state = FrameState::Finish;
                        }
                        // Handle as-array: (_ as-array k!name)
                        else if(second == "as-array"){
                            std::string func_name = getSymbol();
                            // Create a placeholder variable for the array represented by the function
                            // The function will be defined elsewhere in the model
                            frame.result = mkPlaceholderVar(func_name);
                            parseRpar();
                            frame.state = FrameState::Finish;
                        }
                        // Handle real_algebraic_number: (_ real_algebraic_number <polynomial>, (lower_bound, upper_bound)>)
                        // Format: (_ real_algebraic_number polynomial lower_bound upper_bound)
                        // or: (_ real_algebraic_number <polynomial>, (lower_bound, upper_bound)>)
                        else if(second == "real_algebraic_number"){
                            // Enable placeholder variables for polynomial expressions
                            bool old_allow_placeholder = allow_placeholder_vars;
                            std::shared_ptr<Sort> old_placeholder_sort = placeholder_var_sort;
                            allow_placeholder_vars = true;
                            placeholder_var_sort = SortManager::REAL_SORT;
                            
                            // Check if we have angle brackets (CVC5 format)
                            scanToNextSymbol();
                            std::shared_ptr<DAGNode> polynomial;
                            std::shared_ptr<DAGNode> lower_bound;
                            std::shared_ptr<DAGNode> upper_bound;
                            
                            if(*bufptr == '<'){
                                // CVC5 format: <polynomial>, (lower_bound, upper_bound)>
                                bufptr++; // skip '<'
                                scanToNextSymbol();
                                
                                // Parse polynomial (until we find ',')
                                polynomial = parseExpr();
                                
                                // Skip comma and whitespace
                                scanToNextSymbol();
                                if(*bufptr == ','){
                                    bufptr++;
                                    scanToNextSymbol();
                                }
                                
                                // Parse interval: (lower_bound, upper_bound)
                                if(*bufptr == '('){
                                    parseLpar();
                                    lower_bound = parseExpr();
                                    scanToNextSymbol();
                                    if(*bufptr == ','){
                                        bufptr++;
                                        scanToNextSymbol();
                                    }
                                    upper_bound = parseExpr();
                                    parseRpar();
                                } else {
                                    // Fallback: parse two separate expressions
                                    lower_bound = parseExpr();
                                    upper_bound = parseExpr();
                                }
                                
                                // Skip '>'
                                scanToNextSymbol();
                                if(*bufptr == '>'){
                                    bufptr++;
                                }
                            } else {
                                // Simple format: polynomial lower_bound upper_bound
                                polynomial = parseExpr();
                                lower_bound = parseExpr();
                                upper_bound = parseExpr();
                            }
                            
                            // Restore original settings
                            allow_placeholder_vars = old_allow_placeholder;
                            placeholder_var_sort = old_placeholder_sort;
                            
                            // Create real-algebraic-number node
                            frame.result = mkRealAlgebraicNumber(polynomial, lower_bound, upper_bound);
                            parseRpar();
                            frame.state = FrameState::Finish;
                        }
                        else if(second == "is"){
                            std::string ctor = getSymbol();
                            parseRpar();
                            frame.second_symbol = "is-" + ctor;
                            frame.state = FrameState::ProcessingParams;
                        }
                        else{
                            frame.second_symbol = second;
                            frame.state = FrameState::ProcessingParams;
                        }
                    }
                    // Handle floating point bit representation: (fp sign exp mant)
                    else if(head == "fp"){
                        // Parse (fp sign exp mant) where sign, exp, mant are bit vectors
                        std::vector<std::shared_ptr<DAGNode>> fp_args;
                        
                        // Parse sign bit
                        std::shared_ptr<DAGNode> sign = parseExpr();
                        fp_args.push_back(sign);
                        
                        // Parse exponent
                        std::shared_ptr<DAGNode> exp = parseExpr();
                        fp_args.push_back(exp);
                        
                        // Parse mantissa
                        std::shared_ptr<DAGNode> mant = parseExpr();
                        fp_args.push_back(mant);
                        
                        // Create fp constant using the new mkFpConst function
                        frame.result = mkFpConst(sign, exp, mant);
                        parseRpar();
                        frame.state = FrameState::Finish;
                    }
                    else if(head == "let"){
                        if(let_nesting_depth == 0){
                            current_let_mode = LET_MODE::LM_START_LET;
                        }else if(current_let_mode == LET_MODE::LM_START_LET){
                            current_let_mode = LET_MODE::LM_IN_LET;
                        }
                        let_nesting_depth++;

                        std::shared_ptr<DAGNode> res;
                        if(getOptions()->parsing_preserve_let){
                            res = parsePreservingLet();
                        }else{
                            res = parseLet();
                        }

                        let_nesting_depth--;
                        if(let_nesting_depth == 0){
                            current_let_mode = LET_MODE::LM_NON_LET;
                        }

                        if(res->isErr()) err_all(res, "let", frame.line);
                        parseRpar();
                        frame.result = res;
                        frame.state = FrameState::Finish;
                    }else if (head == "!"){
                        // (! <term> <attr>+), <attr> ::= :keyword [value].
                        // Only :named is recorded (unsat cores); every other
                        // annotation (:pattern (...), :weight n, :qid s,
                        // :lblpos l, ...) is a hint and is skipped — including
                        // parenthesised pattern lists.  Previously only :named
                        // was handled, so quantified benchmarks carrying
                        // :pattern (almost all of UFNIA/UFDT/...) failed to parse.
                        // With preserve_annotations enabled the understood
                        // annotations are kept as nodes so printing round-trips
                        // triggers instead of silently dropping them (ported
                        // from the SMTStabilizer fork); otherwise the original
                        // skip-everything behaviour is retained.
                        const bool keep_annots = getOptions()->getPreserveAnnotations();
                        std::vector<std::shared_ptr<DAGNode>> annotations;
                        std::shared_ptr<DAGNode> formula = parseExpr();
                        scanToNextSymbol();
                        while(*bufptr && *bufptr != ')'){
                            if(*bufptr == ':'){
                                std::string kw;
                                while(*bufptr && !isspace((unsigned char)*bufptr)
                                      && *bufptr != '(' && *bufptr != ')'){
                                    kw += *bufptr++;
                                }
                                scanToNextSymbol();
                                if(kw == ":named" && *bufptr
                                   && *bufptr != ':' && *bufptr != ')'){
                                    size_t name_ln = line_number;
                                    std::string name = getSymbol();
                                    warn_named_displaced(context_.nameAssertion(name, formula),
                                                         name, name_ln);
                                    scanToNextSymbol();
                                }else if(keep_annots && kw == ":pattern" && *bufptr == '('){
                                    parseLpar();
                                    std::vector<std::shared_ptr<DAGNode>> patterns;
                                    scanToNextSymbol();
                                    while(*bufptr && *bufptr != ')'){
                                        patterns.emplace_back(parseExpr());
                                        scanToNextSymbol();
                                    }
                                    parseRpar();
                                    annotations.emplace_back(mkPattern(patterns));
                                    scanToNextSymbol();
                                }else if(keep_annots && kw == ":no-pattern"
                                         && *bufptr && *bufptr != ':' && *bufptr != ')'){
                                    annotations.emplace_back(mkNoPattern(parseExpr()));
                                    scanToNextSymbol();
                                }else if(keep_annots && kw == ":weight"
                                         && *bufptr && *bufptr != ':' && *bufptr != ')'){
                                    annotations.emplace_back(mkWeight(parseExpr()));
                                    scanToNextSymbol();
                                }else if(keep_annots && kw == ":qid"
                                         && *bufptr && *bufptr != ':' && *bufptr != ')'
                                         && *bufptr != '('){
                                    annotations.emplace_back(mkQid(getSymbol()));
                                    scanToNextSymbol();
                                }else if(*bufptr == '('){
                                    // skip a balanced parenthesised value
                                    int depth = 0;
                                    do{
                                        if(*bufptr == '(') depth++;
                                        else if(*bufptr == ')') depth--;
                                        bufptr++;
                                    }while(*bufptr && depth > 0);
                                    scanToNextSymbol();
                                }else if(*bufptr && *bufptr != ':' && *bufptr != ')'){
                                    getSymbol();  // single-token value
                                    scanToNextSymbol();
                                }
                            }else{
                                getSymbol();  // unexpected token; make progress
                                scanToNextSymbol();
                            }
                        }
                        if(!annotations.empty()){
                            formula = mkAttribute(formula, annotations);
                        }
                        frame.result = formula;
                        parseRpar();
                        frame.state = FrameState::Finish;
                    }
                    else if(head == "root-of-with-interval"){
                        // (root-of-with-interval (coeffs 1 (- 2)) 1 2)
                        // Parse (coeffs ...) as a whole
                        parseLpar(); // for (coeffs ...)
                        std::string coeffs_tag = getSymbol(); // read "coeffs"
                        // We don't validate coeffs_tag, just skip it
                        
                        // Parse coefficients
                        std::vector<std::shared_ptr<DAGNode>> coeffs_list;
                        scanToNextSymbol(); // Skip whitespace before checking for ')'
                        while(*bufptr != ')'){
                            std::shared_ptr<DAGNode> coeff = parseExpr();
                            coeffs_list.push_back(coeff);
                            scanToNextSymbol(); // Skip whitespace before checking for ')' again
                        }
                        parseRpar(); // close (coeffs ...)
                        
                        // Parse lower bound
                        std::shared_ptr<DAGNode> lower_bound = parseExpr();
                        
                        // Parse upper bound
                        std::shared_ptr<DAGNode> upper_bound = parseExpr();
                        
                        parseRpar(); // close (root-of-with-interval ...)
                        
                        // Create root-of-with-interval node
                        frame.result = mkRootOfWithInterval(coeffs_list, lower_bound, upper_bound);
                        frame.state = FrameState::Finish;
                    }
                    else if(head == "match"){
                        auto res = parseMatch();
                        parseRpar();
                        frame.result = res;
                        frame.state = FrameState::Finish;
                    }

                    else{
                        frame.state = FrameState::ProcessingParams;
                    }
                    break;
                }

                case FrameState::ProcessingParams:{
                    TIME_BLOCK("parseExpr FrameState::ProcessingParams");
                    
                    // Special handling for root-obj and root-of-with-interval: enable placeholder variables
                    bool old_allow_placeholder = allow_placeholder_vars;
                    std::shared_ptr<Sort> old_placeholder_sort = placeholder_var_sort;
                    const std::string& opName = (frame.headSymbol == "_") ? frame.second_symbol : frame.headSymbol;
                    if(opName == "root-obj" || opName == "root-of-with-interval" || opName == "real_algebraic_number"){
                        allow_placeholder_vars = true;
                        placeholder_var_sort = SortManager::REAL_SORT;
                    }
                    
                    // escape the space and comment
                    scanToNextSymbol();
                    if(*bufptr == ')'){
                        parseRpar();
                        std::shared_ptr<DAGNode> res;
                        
                        // inline the most common operators, avoid parseOper overhead
                        const std::string& opName = (frame.headSymbol == "_") ? frame.second_symbol : frame.headSymbol;
                        
                        // fast path: directly handle high frequency operators
                        // 按使用频率排序，最高频的放前面
                        if (opName == "=") {
                            res = mkEq(frame.oper_params);
                        } else if (opName == "and") {
                            res = mkAnd(frame.oper_params);
                        } else if (opName == "=>") {
                            res = mkImplies(frame.oper_params);
                        } else if (opName == "or") {
                            res = mkOr(frame.oper_params);
                        } else if (opName == "not") {
                            condAssert(frame.oper_params.size() == 1, "Invalid number of parameters for not");
                            res = mkNot(frame.oper_params[0]);
                        } else if (opName == "ite") {
                            condAssert(frame.oper_params.size() == 3, "Invalid number of parameters for ite");
                            res = mkIte(frame.oper_params);
                        } else if (opName == "distinct") {
                            res = mkDistinct(frame.oper_params);
                        } else if (opName == "+") {
                            res = mkAdd(frame.oper_params);
                        } else if (opName == "-") {
                            res = mkSub(frame.oper_params);
                        } else if (opName == "*") {
                            res = mkMul(frame.oper_params);
                        } else if (opName == "<=") {
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for <=");
                            res = mkLe(frame.oper_params[0], frame.oper_params[1]);
                        } else if (opName == "<") {
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for <");
                            res = mkLt(frame.oper_params[0], frame.oper_params[1]);
                        } else if (opName == ">=") {
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for >=");
                            res = mkGe(frame.oper_params[0], frame.oper_params[1]);
                        } else if (opName == ">") {
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for >");
                            res = mkGt(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvult"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvult");
                            res = mkBvUlt(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvugt"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvugt");
                            res = mkBvUgt(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvule"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvule");
                            res = mkBvUle(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvuge"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvuge");
                            res = mkBvUge(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvsgt"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvsgt");
                            res = mkBvSgt(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvsle"){   
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvsle");
                            res = mkBvSle(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvslt"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvslt");
                            res = mkBvSlt(frame.oper_params[0], frame.oper_params[1]);
                        } else if(opName == "bvsge"){
                            condAssert(frame.oper_params.size() == 2, "Invalid number of parameters for bvsge");
                            res = mkBvSge(frame.oper_params[0], frame.oper_params[1]);
                        } else {
                            // slow path: call parseOper
                            res = parseOper(opName, frame.func_args, frame.oper_params);
                        }
                        
                        if(res->isErr()) err_all(res, frame.headSymbol, frame.line);
                        frame.result = res;
                        frame.state = FrameState::Finish;
                        
                        // Restore placeholder settings
                        if(opName == "root-obj" || opName == "root-of-with-interval" || opName == "real_algebraic_number"){
                            allow_placeholder_vars = old_allow_placeholder;
                            placeholder_var_sort = old_placeholder_sort;
                        }
                        break;
                    }
                    st.push(ExprFrame());
                    break;
                }

                case FrameState::ProcessingParamFuncArgs:{
                    TIME_BLOCK("parseExpr FrameState::ProcessingParamFuncArgs");
                    // escape the space and comment
                    scanToNextSymbol();
                    if(*bufptr == ')'){
                        parseRpar();
                        frame.state = FrameState::ProcessingParamFuncParams;
                        break;
                    }
                    st.push(ExprFrame());
                    break;
                }

                case FrameState::ProcessingParamFuncParams:{
                    TIME_BLOCK("parseExpr FrameState::ProcessingParamFuncParams");
                    // escape the space and comment
                    scanToNextSymbol();
                    if(*bufptr == ')'){
                        parseRpar();
                        std::shared_ptr<DAGNode> res;
                        
                        // inline the most common operators
                        const std::string& opName = frame.second_symbol;
                        
                        res = parseOper(opName, frame.func_args, frame.oper_params, true);
                        
                        if(res->isErr()) err_all(res, frame.second_symbol, frame.line);
                        frame.result = res;
                        frame.state = FrameState::Finish;
                        break;
                    }
                    st.push(ExprFrame());
                    break;
                }

                case FrameState::Finish:{
                    TIME_BLOCK("parseExpr FrameState::Finish");
                    auto res = frame.result;
                    st.pop();
                    if(st.empty()) return res;
                    ExprFrame &parent = st.top();
                    if(parent.state == FrameState::ProcessingParams){
                        parent.oper_params.emplace_back(res);
                    }else if(parent.state == FrameState::ProcessingParamFuncArgs){
                        parent.func_args.emplace_back(res);
                    }else if(parent.state == FrameState::ProcessingParamFuncParams){
                        parent.oper_params.emplace_back(res);
                    }
                    break;
                }
                default:{
                    TIME_BLOCK("parseExpr Default");
                    st.pop();
                    break;
                }
            }
        }

        return mkErr(ERROR_TYPE::ERR_UNKWN_SYM); // cannot reach here
    }

	
	std::shared_ptr<DAGNode> Parser::parseConstFunc(const std::string& s){
        
		// first handle the special symbols
		if (s == "true") {
			return mkTrue();
		}
		else if (s == "false") {
			return mkFalse();
		}
		// numerals must be parsed as constants before symbol lookup; otherwise e.g. (>= 486 x)
		// would resolve "486" as symbol |486| (declare-fun |486| () Bool) via resolveTerm bar fallback
		if(TypeChecker::isInt(s)){
			return mkConstInt(s);
		}
		else if(TypeChecker::isReal(s)){
			return mkConstReal(s);
		}
		else if(TypeChecker::isScientificNotation(s)){
			std::string parsed = ConversionUtils::parseScientificNotation(s);
			return mkConstReal(parsed);
		}
		{
			ResolveScope scope;
			scope.check_let = (current_let_mode != LET_MODE::LM_NON_LET);
			scope.in_quantifier_scope = in_quantifier_scope;
			std::shared_ptr<DAGNode> var = getSymbolManager()->resolveTerm(s, scope);
			if (var) return var;
			// 0-arity defined/declared functions (e.g. define-fun ref!3 () ...) are stored in fun_key_map;
			// resolveTerm does not look there.
			std::shared_ptr<DAGNode> func = getSymbolManager()->resolveFun(s);
			if (func && func->getFuncParamsSize() == 0) {
				// Check if this is a nullary DT constructor
				auto ret_sort = func->getSort();
				if(ret_sort && ret_sort->isDatatype() && ret_sort->hasDtConstructor(s)) {
					return getNodeManager()->createNode(ret_sort, NODE_KIND::NT_DT_CONSTRUCTOR, s, {});
				}
				// Nullary functions go through applyFun uniformly so that
				// expand_functions / expand_recursive_functions are handled
				// in a single place (applyFun), not duplicated here.
				return applyFun(func, std::vector<std::shared_ptr<DAGNode>>{});
			}
		}
		// otherwise, it is a constant
		if(s == "pi"){
			return mkPi();
		}
		else if(s == "e"){
			return mkE();
		}
		else if(s == "inf"){
			return mkInfinity(getOptions()->isIntTheory()? SortManager::INT_SORT : SortManager::REAL_SORT);
		}
		else if(s == "+inf"){
			return mkPosInfinity(getOptions()->isIntTheory()? SortManager::INT_SORT : SortManager::REAL_SORT);
		}
		else if(s == "-inf"){
			return mkNegInfinity(getOptions()->isIntTheory()? SortManager::INT_SORT : SortManager::REAL_SORT);
		}
		else if(s == "epsion"){
			return mkEpsilon();
		}
		else if(s == "+epsilon"){
			return mkPosEpsilon();
		}
		else if(s == "-epsilon"){
			return mkNegEpsilon();
		}
		else if(s == "NaN" || s == "+NaN" || s == "-NaN"){
			return mkNaN();
		}
		else if(TypeChecker::isBV(s)){
			size_t width;
			if (s[1] == 'b' || s[1] == 'B') {
				// Binary: each character is 1 bit
				width = s.size() - 2;
			} else if (s[1] == 'x' || s[1] == 'X') {
				// Hexadecimal: each character is 4 bits
				width = (s.size() - 2) * 4;
			} else if (s[1] == 'd' || s[1] == 'D') {
				// Decimal: convert to binary to determine width
				std::string decimal_part = s.substr(2);
				Integer value(decimal_part);
				std::string binary = value.toString(2);
				width = binary.size();
			} else {
				// Fallback to original behavior
				width = s.size() - 2;
			}
			return mkConstBv(s, width);
		}
		else if(TypeChecker::isFP(s)){
			return mkConstFP(s);
		}
		else if(TypeChecker::isString(s)){
			return mkConstStr(s);
		}
		// no parameters
		// tuple.unit is the nullary tuple constant, so it appears as a bare
		// symbol rather than an application. Ported from the SMTStabilizer fork.
		else if (s == "tuple.unit"){
			return mkTuple({});
		}
		else if (s == "re.none"){
			return mkRegNone();
		}
		else if (s == "re.all"){
			return mkRegAll();
		}
		else if (s == "re.allchar"){
			return mkRegAllChar();
		}
		// Rounding mode constants for floating point operations
		else if (s == "RNE" || s == "roundNearestTiesToEven"){
			return mkRoundingMode("RNE");
		}
		else if (s == "RNA" || s == "roundNearestTiesToAway"){
			return mkRoundingMode("RNA");
		}
		else if (s == "RTP" || s == "roundTowardPositive"){
			return mkRoundingMode("RTP");
		}
		else if (s == "RTN" || s == "roundTowardNegative"){
			return mkRoundingMode("RTN");
		}
		else if (s == "RTZ" || s == "roundTowardZero"){
			return mkRoundingMode("RTZ");
		}
		else {
			// If allow_placeholder_vars is true, create a placeholder variable
			if(allow_placeholder_vars && !TypeChecker::isNumber(s)){
				return mkPlaceholderVar(s);
			}
			err_unkwn_sym(s, line_number);
		}
		return mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
	}

    NODE_KIND Parser::getKind(const std::string& s){
        auto kind = SOMTParser::getOperKind(s, getOptions()->getStrictSmtlib());
        if(kind == NODE_KIND::NT_UNKNOWN && getSymbolManager()->resolveFun(s)){
            if(!SOMTParser::isBuiltinNameReservedAgainstUserFun(s)){
                kind = NODE_KIND::NT_FUNC_APPLY;
            }
        }
        return kind;
    }
    
	std::shared_ptr<DAGNode> Parser::parseOper(const std::string& s, const std::vector<std::shared_ptr<DAGNode>>& func_args, const std::vector<std::shared_ptr<DAGNode>> &oper_params, bool indexed_under_score){
		TIME_FUNC();
		auto func = getSymbolManager()->resolveFun(s);
		// ((_ update <sel>) t v) arrives with the selector folded into the name
		// under a marker prefix that cannot occur in an SMT-LIB symbol, so this
		// can never collide with a user-declared function.
		if(!func && s.rfind(DT_UPDATE_PREFIX, 0) == 0 && oper_params.size() == 2){
			return mkDtUpdate(s.substr(std::string(DT_UPDATE_PREFIX).size()), oper_params[0], oper_params[1]);
		}
		// Datatype tester/constructor/selector applied here: the indexed tester
		// form (_ is C) registers as a FuncDec named "is-C", and an applied
		// constructor/selector also resolves as a FuncDec — applyFun would emit a
		// generic UF apply, losing the DT kind so downstream datatype reasoning
		// treats the term as opaque (the QF_DT tester-on-constructor false-SAT
		// class). Tag it with its true DT node kind first. getDtFunctionKind
		// returns NT_UF_APPLY for non-DT functions, so this only affects genuine
		// datatype applications (constructor: return sort is the DT; tester/
		// selector: first arg sort is the DT and the name matches).
		if(func){
			NODE_KIND dtk = getDtFunctionKind(func->getSort(), s, oper_params);
			if(dtk == NODE_KIND::NT_DT_TESTER || dtk == NODE_KIND::NT_DT_CONSTRUCTOR ||
			   dtk == NODE_KIND::NT_DT_SELECTOR){
				return getNodeManager()->createNode(func->getSort(), dtk, s, oper_params);
			}
		}
		// Non-indexed (op ...): user define-fun / define-fun-rec / pending declare-fun (body) may shadow only allow_builtin_shadow names.
		if(!indexed_under_score && func && (func->isFuncDef() || func->isFuncRec() || func->isFuncDec())){
			if(!SOMTParser::isBuiltinNameReservedAgainstUserFun(s)){
				return applyFun(func, oper_params);
			}
		}
		// Indexed (_ op ...): builtins first so (declare-fun to_real ...) does not swallow (_ to_real x).
		NODE_KIND kind = SOMTParser::getOperKind(s, getOptions()->getStrictSmtlib());
		if(kind != NODE_KIND::NT_UNKNOWN){
		switch(kind){
			case NODE_KIND::NT_AND:
				return mkAnd(oper_params);
			case NODE_KIND::NT_OR:
				return mkOr(oper_params);
			case NODE_KIND::NT_NOT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for not");
				return mkNot(oper_params[0]);
			case NODE_KIND::NT_IMPLIES:
				return mkImplies(oper_params);
			case NODE_KIND::NT_XOR:
				return mkXor(oper_params);
			case NODE_KIND::NT_EQ:
				return mkEq(oper_params);
			case NODE_KIND::NT_DISTINCT:
				return mkDistinct(oper_params);
			case NODE_KIND::NT_ITE:
				return mkIte(oper_params);
			case NODE_KIND::NT_ADD:
				return mkAdd(oper_params);
            case NODE_KIND::NT_NEG:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for neg");
                return mkNeg(oper_params[0]);
			case NODE_KIND::NT_SUB:
				return mkSub(oper_params);
			case NODE_KIND::NT_MUL:
				return mkMul(oper_params);
			case NODE_KIND::NT_IAND:
				return mkIand(oper_params);
			case NODE_KIND::NT_POW2:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for pow2");
				return mkPow2(oper_params[0]);
			case NODE_KIND::NT_POW:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for pow");
				return mkPow(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_DIV_INT:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for div");
				return mkDivInt(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_DIV_REAL:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for div");
				return mkDivReal(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_MOD:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for mod");
				return mkMod(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_ABS:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for abs");
				return mkAbs(oper_params[0]);
			case NODE_KIND::NT_SQRT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for sqrt");
				return mkSqrt(oper_params[0]);
			case NODE_KIND::NT_SAFESQRT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for safeSqrt");
				return mkSafeSqrt(oper_params[0]);
			case NODE_KIND::NT_CEIL:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for ceil");
				return mkCeil(oper_params[0]);
			case NODE_KIND::NT_FLOOR:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for floor");
				return mkFloor(oper_params[0]);
			case NODE_KIND::NT_ROUND:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for round");
				return mkRound(oper_params[0]);
			case NODE_KIND::NT_EXP:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for exp");
				return mkExp(oper_params[0]);
			case NODE_KIND::NT_LN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for ln");
				return mkLn(oper_params[0]);
			case NODE_KIND::NT_LG:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for lg");
				return mkLg(oper_params[0]);
			case NODE_KIND::NT_LB:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for lb");
				return mkLb(oper_params[0]);
			case NODE_KIND::NT_LOG:
				if(oper_params.size() == 1){
					return mkLn(oper_params[0]);
				}
				else if(oper_params.size() == 2){
					return mkLog(oper_params[0], oper_params[1]);
				}
				else {
					err_param_mis("log", line_number);
					return mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
				}
			case NODE_KIND::NT_SIN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for sin");
				return mkSin(oper_params[0]);
			case NODE_KIND::NT_COS:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for cos");
				return mkCos(oper_params[0]);
			case NODE_KIND::NT_TAN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for tan");
				return mkTan(oper_params[0]);
			case NODE_KIND::NT_COT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for cot");
				return mkCot(oper_params[0]);
			case NODE_KIND::NT_SEC:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for sec");
				return mkSec(oper_params[0]);
			case NODE_KIND::NT_CSC:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for csc");
				return mkCsc(oper_params[0]);
			case NODE_KIND::NT_ASIN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for asin");
				return mkAsin(oper_params[0]);
			case NODE_KIND::NT_ACOS:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acos");
				return mkAcos(oper_params[0]);
			case NODE_KIND::NT_ATAN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for atan");
				return mkAtan(oper_params[0]);
			case NODE_KIND::NT_ACOT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acot");
				return mkAcot(oper_params[0]);
			case NODE_KIND::NT_ASEC:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for asec");
				return mkAsec(oper_params[0]);
			case NODE_KIND::NT_ACSC:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acsc");
				return mkAcsc(oper_params[0]);
			case NODE_KIND::NT_SINH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for sinh");
				return mkSinh(oper_params[0]);
			case NODE_KIND::NT_COSH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for cosh");
				return mkCosh(oper_params[0]);
			case NODE_KIND::NT_TANH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for tanh");
				return mkTanh(oper_params[0]);
			case NODE_KIND::NT_ASECH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for asech");
				return mkAsech(oper_params[0]);
			case NODE_KIND::NT_ACSCH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acsch");
				return mkAcsch(oper_params[0]);
			case NODE_KIND::NT_ACOTH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acoth");
				return mkAcoth(oper_params[0]);
			case NODE_KIND::NT_ATAN2:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for atan2");
				return mkAtan2(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_ASINH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for asinh");
				return mkAsinh(oper_params[0]);
			case NODE_KIND::NT_ACOSH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for acosh");
				return mkAcosh(oper_params[0]);
			case NODE_KIND::NT_ATANH:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for atanh");
				return mkAtanh(oper_params[0]);
            case NODE_KIND::NT_SECH:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for sech");
                return mkSech(oper_params[0]);
            case NODE_KIND::NT_CSCH:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for csch");
                return mkCsch(oper_params[0]);
            case NODE_KIND::NT_LE:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for <= ");
				return mkLe(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_LT:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for <");
				return mkLt(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_GE:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for >= ");
				return mkGe(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_GT:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for >");
				return mkGt(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_TO_REAL:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for to_real");
				return mkToReal(oper_params[0]);
			case NODE_KIND::NT_TO_INT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for to_int");
				return mkToInt(oper_params[0]);
			case NODE_KIND::NT_IS_INT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for is_int");
				return mkIsInt(oper_params[0]);
			case NODE_KIND::NT_IS_DIVISIBLE:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for is_divisible");
				return mkIsDivisible(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_IS_PRIME:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for is_prime");
				return mkIsPrime(oper_params[0]);
			case NODE_KIND::NT_IS_EVEN:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for is_even");
				return mkIsEven(oper_params[0]);
			case NODE_KIND::NT_IS_ODD:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for is_odd");
				return mkIsOdd(oper_params[0]);
			case NODE_KIND::NT_GCD:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for gcd");
				return mkGcd(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_LCM:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for lcm");
				return mkLcm(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_FACT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for factorial");
				return mkFact(oper_params[0]);
			case NODE_KIND::NT_BV_NOT:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for bvnot");
				return mkBvNot(oper_params[0]);
			case NODE_KIND::NT_BV_NEG:
				condAssert(oper_params.size() == 1, "Invalid number of parameters for bvneg");
				return mkBvNeg(oper_params[0]);
			case NODE_KIND::NT_BV_AND:
				return mkBvAnd(oper_params);
			case NODE_KIND::NT_BV_OR:
				return mkBvOr(oper_params);
			case NODE_KIND::NT_BV_XOR:
				return mkBvXor(oper_params);
			case NODE_KIND::NT_BV_NAND:
				return mkBvNand(oper_params);
			case NODE_KIND::NT_BV_NOR:
				return mkBvNor(oper_params);
			case NODE_KIND::NT_BV_XNOR:
				return mkBvXnor(oper_params);
			case NODE_KIND::NT_BV_COMP:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvcomp");
				return mkBvComp(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_ADD:
				return mkBvAdd(oper_params);
			case NODE_KIND::NT_BV_SUB:
				return mkBvSub(oper_params);
			case NODE_KIND::NT_BV_MUL:
				return mkBvMul(oper_params);
			case NODE_KIND::NT_BV_UDIV:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvudiv");
				return mkBvUdiv(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_UREM:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvurem");
				return mkBvUrem(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_SDIV:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsdiv");
				return mkBvSdiv(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_SREM:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsrem");
				return mkBvSrem(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_SMOD:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsrem");
                return mkBvSmod(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_SHL:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvshl");
				return mkBvShl(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_LSHR:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvlshr");
                return mkBvLshr(oper_params[0], oper_params[1]);
			case NODE_KIND::NT_BV_ASHR:
				condAssert(oper_params.size() == 2, "Invalid number of parameters for bvashr");
				return mkBvAshr(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_ULT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvult");
                return mkBvUlt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_ULE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvule");
                return mkBvUle(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_UGT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvugt");
                return mkBvUgt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_UGE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvuge");
                return mkBvUge(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_SLT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvslt");
                return mkBvSlt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_SLE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsle");
                return mkBvSle(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_SGT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsgt");
                return mkBvSgt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_SGE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for bvsge");
                return mkBvSge(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_BV_CONCAT:
				return mkBvConcat(oper_params);
            case NODE_KIND::NT_BV_TO_INT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for bv2int");
                return mkBvToInt(oper_params[0]);
            case NODE_KIND::NT_BV_TO_NAT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for bv2nat");
                return mkBvToNat(oper_params[0]);
            case NODE_KIND::NT_NAT_TO_BV:
                // Both the indexed SMT-LIB form ((_ nat2bv m) t) — where the
                // width arrives in func_args — and the flat (nat2bv t m) form.
                // The indexed form used to abort on the 2-parameter assert.
                if(func_args.size() == 1 && oper_params.size() == 1){
                    return mkNatToBv(oper_params[0], func_args[0]);
                }
                condAssert(oper_params.size() == 2, "Invalid number of parameters for nat2bv");
                return mkNatToBv(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_UBV_TO_INT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for ubv_to_int");
                return mkUbvToInt(oper_params[0]);
            case NODE_KIND::NT_SBV_TO_INT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for sbv_to_int");
                return mkSbvToInt(oper_params[0]);
            case NODE_KIND::NT_INT_TO_BV:
                condAssert(func_args.size() == 1, "Invalid number of arguments for int_to_bv");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for int_to_bv");
                return mkIntToBv(oper_params[0], func_args[0]);
            case NODE_KIND::NT_BV_EXTRACT:
                condAssert(func_args.size() == 2, "Invalid number of arguments for extract");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for extract");
                return mkBvExtract(oper_params[0], func_args[0], func_args[1]);
            case NODE_KIND::NT_BV_REPEAT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for repeat");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for repeat");
                return mkBvRepeat(oper_params[0], func_args[0]);
            case NODE_KIND::NT_BV_ZERO_EXT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for zero_extend");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for zero_extend");
                return mkBvZeroExt(oper_params[0], func_args[0]);
            case NODE_KIND::NT_BV_SIGN_EXT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for sign_extend");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for sign_extend");
                return mkBvSignExt(oper_params[0], func_args[0]);
            case NODE_KIND::NT_BV_ROTATE_LEFT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for rotate_left");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for rotate_left");
                return mkBvRotateLeft(oper_params[0], func_args[0]);
            case NODE_KIND::NT_BV_ROTATE_RIGHT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for rotate_right");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for rotate_right");
                return mkBvRotateRight(oper_params[0], func_args[0]);
            case NODE_KIND::NT_FP_ABS:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.abs");
                return mkFpAbs(oper_params[0]);
            case NODE_KIND::NT_FP_NEG:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.neg");
                return mkFpNeg(oper_params[0]);
            case NODE_KIND::NT_FP_ADD:
                return mkFpAdd(oper_params);
            case NODE_KIND::NT_FP_SUB:
                return mkFpSub(oper_params);
            case NODE_KIND::NT_FP_MUL:
                return mkFpMul(oper_params);
            case NODE_KIND::NT_FP_DIV:
                return mkFpDiv(oper_params);
            case NODE_KIND::NT_FP_FMA:
                condAssert(oper_params.size() == 4, "Invalid number of parameters for fp.fma");
                return mkFpFma(oper_params);
            case NODE_KIND::NT_FP_SQRT:
                if(oper_params.size() == 1) {
                    if(getOptions()->getStrictSmtlib()) {
                        err_param_mis("fp.sqrt", line_number);
                        return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                    }
                    return mkFpSqrt(mkRoundingMode("RNE"), oper_params[0]);
                } else if(oper_params.size() == 2) {
                    return mkFpSqrt(oper_params[0], oper_params[1]);
                } else {
                    err_param_mis("fp.sqrt", line_number);
                    return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                }
            case NODE_KIND::NT_FP_REM:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for fp.rem");
                return mkFpRem(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_FP_ROUND_TO_INTEGRAL:
                if(oper_params.size() == 1) {
                    if(getOptions()->getStrictSmtlib()) {
                        err_param_mis("fp.roundToIntegral", line_number);
                        return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                    }
                    return mkFpRoundToIntegral(mkRoundingMode("RNE"), oper_params[0]);
                } else if(oper_params.size() == 2) {
                    return mkFpRoundToIntegral(oper_params[0], oper_params[1]);
                } else {
                    err_param_mis("fp.roundToIntegral", line_number);
                    return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                }
            case NODE_KIND::NT_FP_MIN:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for fp.min");
                return mkFpMin(oper_params);
            case NODE_KIND::NT_FP_MAX:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for fp.max");
                return mkFpMax(oper_params);
            case NODE_KIND::NT_FP_LE:
                condAssert(oper_params.size() >= 2, "Invalid number of parameters for fp.leq (chainable, at least 2)");
                if (oper_params.size() == 2) return mkFpLe(oper_params[0], oper_params[1]);
                else {
                    std::vector<std::shared_ptr<DAGNode>> chain;
                    for (size_t i = 0; i + 1 < oper_params.size(); ++i)
                        chain.push_back(mkFpLe(oper_params[i], oper_params[i + 1]));
                    return mkAnd(chain);
                }
            case NODE_KIND::NT_FP_LT:
                condAssert(oper_params.size() >= 2, "Invalid number of parameters for fp.lt (chainable, at least 2)");
                if (oper_params.size() == 2) return mkFpLt(oper_params[0], oper_params[1]);
                else {
                    std::vector<std::shared_ptr<DAGNode>> chain;
                    for (size_t i = 0; i + 1 < oper_params.size(); ++i)
                        chain.push_back(mkFpLt(oper_params[i], oper_params[i + 1]));
                    return mkAnd(chain);
                }
            case NODE_KIND::NT_FP_GE:
                condAssert(oper_params.size() >= 2, "Invalid number of parameters for fp.geq (chainable, at least 2)");
                if (oper_params.size() == 2) return mkFpGe(oper_params[0], oper_params[1]);
                else {
                    std::vector<std::shared_ptr<DAGNode>> chain;
                    for (size_t i = 0; i + 1 < oper_params.size(); ++i)
                        chain.push_back(mkFpGe(oper_params[i], oper_params[i + 1]));
                    return mkAnd(chain);
                }
            case NODE_KIND::NT_FP_GT:
                condAssert(oper_params.size() >= 2, "Invalid number of parameters for fp.gt (chainable, at least 2)");
                if (oper_params.size() == 2) return mkFpGt(oper_params[0], oper_params[1]);
                else {
                    std::vector<std::shared_ptr<DAGNode>> chain;
                    for (size_t i = 0; i + 1 < oper_params.size(); ++i)
                        chain.push_back(mkFpGt(oper_params[i], oper_params[i + 1]));
                    return mkAnd(chain);
                }
            case NODE_KIND::NT_FP_EQ:
                condAssert(oper_params.size() >= 2, "Invalid number of parameters for fp.eq (chainable, at least 2)");
                if (oper_params.size() == 2) return mkFpEq(oper_params[0], oper_params[1]);
                else {
                    std::vector<std::shared_ptr<DAGNode>> chain;
                    for (size_t i = 0; i + 1 < oper_params.size(); ++i)
                        chain.push_back(mkFpEq(oper_params[i], oper_params[i + 1]));
                    return mkAnd(chain);
                }
            case NODE_KIND::NT_FP_TO_UBV:
                condAssert(func_args.size() == 1 && oper_params.size() == 2, "Invalid number of parameters for fp.to_ubv");
                return mkFpToUbv(oper_params[0], oper_params[1], func_args[0]);
            case NODE_KIND::NT_FP_TO_SBV:
                condAssert(func_args.size() == 1 && oper_params.size() == 2, "Invalid number of parameters for fp.to_sbv");
                return mkFpToSbv(oper_params[0], oper_params[1], func_args[0]);
            case NODE_KIND::NT_FP_TO_REAL:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.to_real");
                return mkFpToReal(oper_params[0]);
            case NODE_KIND::NT_FP_TO_FP:
                // Handle different to_fp syntax:
                // 1. ((_ to_fp eb sb) RoundingMode Real) -> func_args: [eb, sb], oper_params: [RoundingMode, Real]
                // 2. ((_ to_fp eb sb) RoundingMode (_ BitVec m)) -> func_args: [eb, sb], oper_params: [RoundingMode, BitVec]
                // 3. ((_ to_fp eb sb) (_ BitVec m)) -> func_args: [eb, sb], oper_params: [BitVec]
                if(func_args.size() == 2) {
                    if(oper_params.size() == 2) {
                        // Case 1 and 2: RoundingMode + Real/BitVec
                        return mkToFp(func_args[0], func_args[1], oper_params[0], oper_params[1]);
                    } else if(oper_params.size() == 1) {
                        // Case 3: BitVec only
                        return mkToFp(func_args[0], func_args[1], oper_params[0]);
                    } else {
                        err_param_mis("to_fp", line_number);
                        return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                    }
                } else if(oper_params.size() == 3) {
                    if(getOptions()->getStrictSmtlib()) {
                        err_param_mis("to_fp", line_number);
                        return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                    }
                    // Lenient dialect: flat (to_fp eb sb arg) without indexed head
                    return mkToFp(oper_params[0], oper_params[1], oper_params[2]);
                } else {
                    err_param_mis("to_fp", line_number);
                    return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                }
            case NODE_KIND::NT_FP_TO_FP_UNSIGNED:
                // Handle to_fp_unsigned syntax:
                // ((_ to_fp_unsigned eb sb) RoundingMode (_ BitVec m)) -> func_args: [eb, sb], oper_params: [RoundingMode, BitVec]
                if(func_args.size() == 2 && oper_params.size() == 2) {
                    return mkToFpUnsigned(func_args[0], func_args[1], oper_params[0], oper_params[1]);
                } else {
                    err_param_mis("to_fp_unsigned", line_number);
                    return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
                }

            case NODE_KIND::NT_FP_IS_NORMAL:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isNormal");
                return mkFpIsNormal(oper_params[0]);
            case NODE_KIND::NT_FP_IS_SUBNORMAL:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isSubnormal");
                return mkFpIsSubnormal(oper_params[0]);
            case NODE_KIND::NT_FP_IS_ZERO:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isZero");
                return mkFpIsZero(oper_params[0]);
            case NODE_KIND::NT_FP_IS_INF:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isInfinite");
                return mkFpIsInf(oper_params[0]);
            case NODE_KIND::NT_FP_IS_NAN:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isNaN");
                return mkFpIsNaN(oper_params[0]);
            case NODE_KIND::NT_FP_IS_NEG:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isNegative");
                return mkFpIsNeg(oper_params[0]);
            case NODE_KIND::NT_FP_IS_POS:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for fp.isPositive");
                return mkFpIsPos(oper_params[0]);
            case NODE_KIND::NT_SELECT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for select");
                return mkSelect(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STORE:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for store");
                return mkStore(oper_params[0], oper_params[1], oper_params[2]);
            // TUPLE OPERATORS — ported from the SMTStabilizer fork. The
            // select/update/project indices arrive as func_args because the
            // operators are indexed: ((_ tuple.select i) t).
            case NODE_KIND::NT_TUPLE_CONSTRUCTOR:
                return mkTuple(oper_params);
            case NODE_KIND::NT_TUPLE_UNIT:
                condAssert(oper_params.empty(), "tuple.unit takes no parameters");
                return mkTuple({});
            case NODE_KIND::NT_TUPLE_SELECT:
                condAssert(func_args.size() == 1, "Invalid number of arguments for tuple.select");
                condAssert(oper_params.size() == 1, "Invalid number of parameters for tuple.select");
                return mkTupleSelect(oper_params[0], func_args[0]);
            case NODE_KIND::NT_TUPLE_UPDATE:
                condAssert(func_args.size() == 1, "Invalid number of arguments for tuple.update");
                condAssert(oper_params.size() == 2, "Invalid number of parameters for tuple.update");
                return mkTupleUpdate(oper_params[0], func_args[0], oper_params[1]);
            case NODE_KIND::NT_TUPLE_PROJECT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for tuple.project");
                return mkTupleProject(oper_params[0], func_args);
            case NODE_KIND::NT_ROOT_OBJ:
                condAssert(oper_params.size() == 2, "root-obj requires exactly 2 parameters");
                {
                    auto expr = oper_params[0];
                    auto index_node = oper_params[1];
                    condAssert(index_node->isConst() && index_node->isCInt(),
                               "root-obj index must be integer constant");
                    int index = std::stoi(index_node->getName());
                    return mkRootObj(expr, index);
                }
            case NODE_KIND::NT_STR_LEN:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.len");
                return mkStrLen(oper_params[0]);
            case NODE_KIND::NT_STR_CONCAT:
                return mkStrConcat(oper_params);
            case NODE_KIND::NT_STR_SUBSTR:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.substr");
                return mkStrSubstr(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_PREFIXOF:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.prefixof");
			    return mkStrPrefixof(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_SUFFIXOF:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.suffixof");
                return mkStrSuffixof(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_INDEXOF:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.indexof");
                return mkStrIndexof(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_CHARAT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.at");
                return mkStrCharat(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_UPDATE:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.update");
                return mkStrUpdate(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_REPLACE:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.replace");
                return mkStrReplace(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_REPLACE_ALL:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.replace_all");
                return mkStrReplaceAll(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_REPLACE_REG:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.replace_re");
                return mkStrReplaceReg(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_REPLACE_REG_ALL:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.replace_re_all");
                return mkStrReplaceRegAll(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_INDEXOF_REG:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.indexof_re");
                return mkStrIndexofReg(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_TO_LOWER:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.to_lower");
                return mkStrToLower(oper_params[0]);
            case NODE_KIND::NT_STR_TO_UPPER:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.to_upper");
                return mkStrToUpper(oper_params[0]);
            case NODE_KIND::NT_STR_REV:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.rev");
                return mkStrRev(oper_params[0]);
            case NODE_KIND::NT_STR_SPLIT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.split");
                return mkStrSplit(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_SPLIT_AT:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.split_at");
                return mkStrSplitAt(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_SPLIT_REST:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.split_rest");
                return mkStrSplitRest(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_NUM_SPLITS:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.num_splits");
                return mkStrNumSplits(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_SPLIT_AT_RE:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.split_at_re");
                return mkStrSplitAtRe(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_SPLIT_REST_RE:
                condAssert(oper_params.size() == 3, "Invalid number of parameters for str.split_rest_re");
                return mkStrSplitRestRe(oper_params[0], oper_params[1], oper_params[2]);
            case NODE_KIND::NT_STR_NUM_SPLITS_RE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.num_splits_re");
                return mkStrNumSplitsRe(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_LT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.<");
                return mkStrLt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_LE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.<=");
                return mkStrLe(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_GT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.>");
                return mkStrGt(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_GE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.>=");
                return mkStrGe(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_IN_REG:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.in_re");
                return mkStrInReg(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_CONTAINS:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for str.contains");
                return mkStrContains(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_STR_IS_DIGIT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.is_digit");
                return mkStrIsDigit(oper_params[0]);
            case NODE_KIND::NT_STR_FROM_INT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.from_int");
                return mkStrFromInt(oper_params[0]);
            case NODE_KIND::NT_STR_TO_INT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.to_int");
                return mkStrToInt(oper_params[0]);
            case NODE_KIND::NT_STR_TO_REG:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.to_re");
                return mkStrToReg(oper_params[0]);
            case NODE_KIND::NT_STR_TO_CODE:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.to_code");
                return mkStrToCode(oper_params[0]);
            case NODE_KIND::NT_STR_FROM_CODE:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for str.from_code");
                return mkStrFromCode(oper_params[0]);
            case NODE_KIND::NT_REG_CONCAT:
                return mkRegConcat(oper_params);
            case NODE_KIND::NT_REG_UNION:
                return mkRegUnion(oper_params);
            case NODE_KIND::NT_REG_INTER:
                return mkRegInter(oper_params);
            case NODE_KIND::NT_REG_DIFF:
                return mkRegDiff(oper_params);
            case NODE_KIND::NT_REG_STAR:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for re.*");
                return mkRegStar(oper_params[0]);
            case NODE_KIND::NT_REG_PLUS:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for re.+");
                return mkRegPlus(oper_params[0]);
            case NODE_KIND::NT_REG_OPT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for re.?");
                return mkRegOpt(oper_params[0]);
            case NODE_KIND::NT_REG_RANGE:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for re.range");
                return mkRegRange(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_REG_REPEAT:
                condAssert(oper_params.size() == 2, "Invalid number of parameters for re.repeat");
                return mkRegRepeat(oper_params[0], oper_params[1]);
            case NODE_KIND::NT_REG_COMPLEMENT:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for re.comp");
                return mkRegComplement(oper_params[0]);
            case NODE_KIND::NT_REG_LOOP:
                condAssert(oper_params.size() == 1, "Invalid number of parameters for re.loop");
                condAssert(func_args.size() == 2, "Invalid number of arguments for re.loop");
                return mkRegLoop(oper_params[0], func_args[0], func_args[1]);
            case NODE_KIND::NT_UNKNOWN:
            case NODE_KIND::NT_ERROR:
            case NODE_KIND::NT_NULL:
            case NODE_KIND::NT_CONST:
            case NODE_KIND::NT_VAR:
            case NODE_KIND::NT_CONST_ARRAY:
            case NODE_KIND::NT_CONST_TRUE:
            case NODE_KIND::NT_CONST_FALSE:
            case NODE_KIND::NT_TEMP_VAR:
            case NODE_KIND::NT_MAX:
            case NODE_KIND::NT_MIN:
            case NODE_KIND::NT_LET:
            case NODE_KIND::NT_LET_CHAIN:
            case NODE_KIND::NT_LET_BIND_VAR:
            case NODE_KIND::NT_LET_BIND_VAR_LIST:
            case NODE_KIND::NT_FORALL:
            case NODE_KIND::NT_EXISTS:
            case NODE_KIND::NT_QUANT_VAR:
            case NODE_KIND::NT_FUNC_PARAM:
            case NODE_KIND::NUM_KINDS:
                condAssert(false, "Invalid kind");
            default:
                err_unkwn_sym(s, line_number);
                return mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
		}
		}

        if(func && !SOMTParser::isBuiltinNameReservedAgainstUserFun(s)){
            return applyFun(func, oper_params);
        }

        err_unkwn_sym(s, line_number);
        return mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
	}
}
