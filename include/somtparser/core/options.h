/* -*- Header -*-
 *
 * The Option Class
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
#ifndef _OPTIONS_H
#define _OPTIONS_H

#include "somtparser/core/asserting.h"
#include <mpfr.h>
#include <unordered_map>

namespace SOMTParser{
    class GlobalOptions {
    public:
        // ENUM_LOGIC logic = UNKNOWN_LOGIC;
        std::string logic = "UNKNOWN_LOGIC";
        bool check_sat = false;
        bool get_assertions = false;
        bool get_assignment = false;
        // set-info
        std::unordered_map<std::string, std::string> info;
        // get-info
        std::unordered_map<std::string, std::string> get_info;
        bool get_model = false;
        // set-option
        std::unordered_map<std::string, std::string> options;
        // get-option
        std::unordered_map<std::string, std::string> get_options;
        bool get_proof = false;
        bool get_unsat_assumptions = false;
        bool get_unsat_core = false;
        // get-value
        std::unordered_map<std::string, std::string> values;

        // get-objectives
        bool get_objectives = false;

        // evaluate precision
        bool evaluate_use_floating = true;
        mpfr_prec_t evaluate_precision = 128;

        // whether perserve let-binding
        bool parsing_preserve_let = true;

        // whether to expand function applications (define-fun, define-fun-rec)
        // if true, function calls are inlined with their definitions
        // if false (default), function calls are preserved as function applications
        bool expand_functions = false;

        // whether to expand recursive functions (define-fun-rec)
        // if true, recursive functions will be expanded like define-fun
        // if false (default), recursive functions will not be expanded
        bool expand_recursive_functions = false;

        // When true: reject lenient FloatingPoint surface syntax (unary fp.sqrt /
        // fp.roundToIntegral without RM; non-standard operator spellings like fp.=;
        // flat (to_fp eb sb x) without indexed head). Default false keeps backward compatibility.
        bool strict_smtlib_fp = false;

    public:
        GlobalOptions() = default;
        ~GlobalOptions() = default;

        bool setLogic(const std::string& logic_name) {
            if( logic_name == "QF_ABV" || 
                logic_name == "QF_ABVFP" ||
                logic_name == "QF_ABVFPLRA" ||
                logic_name == "QF_ALIA" ||
                logic_name == "QF_ALRA" ||
                logic_name == "QF_ANIA" ||
                logic_name == "QF_AUFBV" ||
                logic_name == "QF_AUFBVFP" ||
                logic_name == "QF_AUFBVLIA" ||
                logic_name == "QF_AUFBVNIA" ||
                logic_name == "QF_AUFLIA" ||
                logic_name == "QF_AUFLRA" ||
                logic_name == "QF_AUFNIA" ||
                logic_name == "QF_AX" ||
                logic_name == "QF_BV" ||
                logic_name == "QF_BVFP" ||
                logic_name == "QF_BVLRA" ||
                logic_name == "QF_DT" ||
                logic_name == "QF_BVFPLRA" ||
                logic_name == "QF_FP" ||
                logic_name == "QF_FPLRA" ||
                logic_name == "QF_IDL" ||
                logic_name == "QF_LIA" ||
                logic_name == "QF_LIRA" ||
                logic_name == "QF_LRA" ||
                logic_name == "QF_NIA" ||
                logic_name == "QF_NIRA" ||
                logic_name == "QF_NRA" ||
                logic_name == "QF_RDL" ||
                logic_name == "QF_S" ||
                logic_name == "QF_SLIA" ||
                logic_name == "QF_SNIA" ||
                logic_name == "QF_UF" ||
                logic_name == "QF_UFBV" ||
                logic_name == "QF_UFBVDT" ||
                logic_name == "QF_UFBVLIA" ||
                logic_name == "QF_UFDT" ||
                logic_name == "QF_UFDTLIA" ||
                logic_name == "QF_UFDTLIRA" ||
                logic_name == "QF_UFDTNIA" ||
                logic_name == "QF_UFFP" ||
                logic_name == "QF_UFFPDTNIRA" ||
                logic_name == "QF_UFIDL" ||
                logic_name == "QF_UFLIA" ||
                logic_name == "QF_UFLRA" ||
                logic_name == "QF_UFNIA" ||
                logic_name == "QF_UFNRA" ||
                logic_name == "AUFLIA" ||
                logic_name == "AUFLIRA" ||
                logic_name == "AUFNIRA" ||
                logic_name == "LIA" ||
                logic_name == "LRA" ||
                logic_name == "UFLRA" ||
                logic_name == "UFNIA" ||
                logic_name == "BV" ||
                logic_name == "ALL") {
                    logic = logic_name;
                    return true;
                } else {
                    logic = "UNKNOWN_LOGIC";
                    return false;
                }
        }

        void setOption(const std::string& key, const std::string& value) {
            options[key] = value;
            if(key == "keep_let") {
                setKeepLet(value == "true");
            }
            else if(key == "precision"){
                setEvaluatePrecision((size_t)(std::stoi(value)));
            }
            else if(key == "float_evaluate"){
                setEvaluateUseFloating(value == "true");
            }
            else if(key == "expand_functions"){
                setExpandFunctions(value == "true");
            }
            else if(key == "expand_recursive_functions"){
                setExpandRecursiveFunctions(value == "true");
            }
            else if(key == "strict_smtlib_fp"){
                setStrictSmtlib(value == "true");
            }
        }

        void setInfo(const std::string& key, const std::string& value) {
            info[key] = value;
        }

        std::string getLogic() const {
            return logic;
        }

        void getValue(const std::string& key, const std::string& value) {
            values[key] = value;
        }

        void getInfo(const std::string& key, const std::string& value) {
            get_info[key] = value;
        }

        void getOption(const std::string& key, const std::string& value) {
            get_options[key] = value;
        }

        
        // logic checking
        bool isIntTheory() const {
            return logic.find("I") != std::string::npos && logic.find("R") == std::string::npos;
        }
        bool isRealTheory() const {
            return logic.find("R") != std::string::npos;
        }
        bool isIntRealTheory() const {
            return logic.find("I") != std::string::npos && logic.find("R") != std::string::npos;
        }

        void setEvaluatePrecision(mpfr_prec_t precision) {
            evaluate_precision = precision;
        }

        mpfr_prec_t getEvaluatePrecision() const {
            return evaluate_precision;
        }

        void setEvaluateUseFloating(bool use_floating) {
            evaluate_use_floating = use_floating;
        }

        bool getEvaluateUseFloating() const {
            return evaluate_use_floating;
        }

        void setKeepLet(bool keep){ parsing_preserve_let = keep; }
        bool getKeepLet() const { return parsing_preserve_let; }
        
        void setExpandFunctions(bool expand){ expand_functions = expand; }
        bool getExpandFunctions() const { return expand_functions; }
        
        void setExpandRecursiveFunctions(bool expand){ expand_recursive_functions = expand; }
        bool getExpandRecursiveFunctions() const { return expand_recursive_functions; }

        void setStrictSmtlib(bool strict) { strict_smtlib_fp = strict; }
        bool getStrictSmtlib() const { return strict_smtlib_fp; }
        
        /**
         * @brief Generate a detailed configuration report
         * 
         * This method generates a formatted string containing detailed information about
         * all configuration options including: option name, default value, current value,
         * and description.
         * 
         * @return std::string A formatted string with complete configuration information
         */
        std::string toString() const {
            std::string result;
            result += "=================================================================\n";
            result += "                SOMTParser Configuration Report\n";
            result += "=================================================================\n\n";
            
            // Logic setting
            result += "1. Logic\n";
            result += "   Option: logic\n";
            result += "   Default: UNKNOWN_LOGIC\n";
            result += "   Current: " + logic + "\n";
            result += "   Description: The SMT-LIB2 logic to use for parsing and reasoning.\n";
            result += "                Determines which theories and quantifiers are allowed.\n\n";
            
            // Evaluation precision
            result += "2. Evaluation Precision\n";
            result += "   Option: precision (set via setOption or setEvaluatePrecision)\n";
            result += "   Default: 128\n";
            result += "   Current: " + std::to_string(evaluate_precision) + "\n";
            result += "   Description: The precision (in bits) for MPFR floating-point evaluation.\n";
            result += "                Higher values provide more accurate results but slower computation.\n\n";
            
            // Floating point evaluation
            result += "3. Floating-Point Evaluation Mode\n";
            result += "   Option: float_evaluate (set via setOption or setEvaluateUseFloating)\n";
            result += "   Default: true\n";
            result += "   Current: " + std::string(evaluate_use_floating ? "true" : "false") + "\n";
            result += "   Description: When enabled, uses floating-point arithmetic for evaluation.\n";
            result += "                When disabled, uses exact rational arithmetic where possible.\n\n";
            
            // Preserve let bindings
            result += "4. Preserve Let Bindings\n";
            result += "   Option: keep_let (set via setOption or setKeepLet)\n";
            result += "   Default: true\n";
            result += "   Current: " + std::string(parsing_preserve_let ? "true" : "false") + "\n";
            result += "   Description: When enabled, preserves let-binding structures during parsing.\n";
            result += "                When disabled, automatically expands let-bindings inline.\n\n";
            
            // Expand functions
            result += "5. Expand Function Applications\n";
            result += "   Option: expand_functions (set via setOption or setExpandFunctions)\n";
            result += "   Default: true\n";
            result += "   Current: " + std::string(expand_functions ? "true" : "false") + "\n";
            result += "   Description: When enabled (default), function calls (define-fun) are inlined with\n";
            result += "                their definitions. When disabled, function applications are preserved\n";
            result += "                as-is, keeping the function call structure in the AST.\n\n";
            
            // Expand recursive functions
            result += "6. Expand Recursive Functions\n";
            result += "   Option: expand_recursive_functions (set via setOption or setExpandRecursiveFunctions)\n";
            result += "   Default: false\n";
            result += "   Current: " + std::string(expand_recursive_functions ? "true" : "false") + "\n";
            result += "   Status: NOT CURRENTLY SUPPORTED\n";
            result += "   Description: When enabled, recursive functions (define-fun-rec) would be expanded\n";
            result += "                like regular function definitions. This feature is planned for future\n";
            result += "                implementation using a 'this' placeholder mechanism to handle recursive\n";
            result += "                self-references during function body parsing and expansion.\n\n";

            result += "7. Strict SMT-LIB FloatingPoint surface syntax\n";
            result += "   Option: strict_smtlib_fp (set-option / GlobalOptions::setStrictSmtlib / Parser::setStrictSmtlib)\n";
            result += "   Default: false\n";
            result += "   Current: " + std::string(strict_smtlib_fp ? "true" : "false") + "\n";
            result += "   Description: When true, rejects unary fp.sqrt/fp.roundToIntegral (requires explicit RM),\n";
            result += "                non-standard FP operator aliases (e.g. fp.=, fp.toUbv), and non-indexed\n";
            result += "                (to_fp eb sb x) calls. When false (default), lenient extensions are accepted\n";
            result += "                and unary fp.sqrt is canonicalized to (fp.sqrt RNE x) in the IR.\n\n";
            
            // Command flags
            result += "8. Command Flags\n";
            result += "   check_sat: " + std::string(check_sat ? "true" : "false") + "\n";
            result += "   get_assertions: " + std::string(get_assertions ? "true" : "false") + "\n";
            result += "   get_assignment: " + std::string(get_assignment ? "true" : "false") + "\n";
            result += "   get_model: " + std::string(get_model ? "true" : "false") + "\n";
            result += "   get_proof: " + std::string(get_proof ? "true" : "false") + "\n";
            result += "   get_unsat_assumptions: " + std::string(get_unsat_assumptions ? "true" : "false") + "\n";
            result += "   get_unsat_core: " + std::string(get_unsat_core ? "true" : "false") + "\n";
            result += "   get_objectives: " + std::string(get_objectives ? "true" : "false") + "\n";
            result += "   Description: Flags indicating which SMT-LIB2 commands have been encountered.\n\n";
            
            // Set-info
            if (!info.empty()) {
                result += "10. Set-Info Attributes\n";
                for (const auto& kv : info) {
                    result += "   " + kv.first + " = " + kv.second + "\n";
                }
                result += "\n";
            }
            
            // Set-option
            if (!options.empty()) {
                result += "11. Set-Option Values\n";
                for (const auto& kv : options) {
                    result += "   " + kv.first + " = " + kv.second + "\n";
                }
                result += "\n";
            }
            
            // Get-info
            if (!get_info.empty()) {
                result += "12. Get-Info Queries\n";
                for (const auto& kv : get_info) {
                    result += "   " + kv.first + " = " + kv.second + "\n";
                }
                result += "\n";
            }
            
            // Get-option
            if (!get_options.empty()) {
                result += "13. Get-Option Queries\n";
                for (const auto& kv : get_options) {
                    result += "   " + kv.first + " = " + kv.second + "\n";
                }
                result += "\n";
            }
            
            // Get-value
            if (!values.empty()) {
                result += "14. Get-Value Queries\n";
                for (const auto& kv : values) {
                    result += "   " + kv.first + " = " + kv.second + "\n";
                }
                result += "\n";
            }
            
            result += "=================================================================\n";
            return result;
        }
    };
}
#endif // _OPTIONS_H