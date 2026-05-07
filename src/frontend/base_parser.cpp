/* -*- Source -*-
 *
 * An SMT/OMT Parser (Base part)
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
#include <queue>
#include <stack>
#include <algorithm>
#include <functional>
#include <unordered_set>

namespace SOMTParser{

	Parser::Parser(){
		buffer = nullptr;
		bufptr = nullptr;
		buflen = 0;
		line_number = 0;
		scan_mode = SCAN_MODE::SM_COMMON;
		preserving_let_counter = 0;
		let_nesting_depth = 0;
		parsing_file = false;
		in_quantifier_scope = false;
		allow_placeholder_vars = false;
		placeholder_var_sort = nullptr;
		quant_nesting_depth = 0;
		context_.setNodeManager(std::make_shared<NodeManager>());
		context_.setSortManager(std::make_shared<SortManager>());
		auto sym_mgr = std::make_shared<SymbolManager>();
		sym_mgr->reserve(1024);
		context_.setSymbolManager(std::move(sym_mgr));
		context_.setObjectiveManager(std::make_shared<ObjectiveManager>());
		context_.setOptions(std::make_shared<GlobalOptions>());

		// array cache
		array_select_cache.reserve(1024);
		array_normalize_cache.reserve(1024);
	}

	
	bool Parser::parse(const std::string& filename){
		return parseSmtlib2File(filename);
	}

	Parser::Parser(const std::string& filename) {
		buffer = nullptr;
		bufptr = nullptr;
		buflen = 0;
		line_number = 0;
		scan_mode = SCAN_MODE::SM_COMMON;
		preserving_let_counter = 0;
		let_nesting_depth = 0;
		parsing_file = true;
		in_quantifier_scope = false;
		allow_placeholder_vars = false;
		placeholder_var_sort = nullptr;
		quant_nesting_depth = 0;
		context_.setNodeManager(std::make_shared<NodeManager>());
		context_.setSortManager(std::make_shared<SortManager>());
		auto sym_mgr = std::make_shared<SymbolManager>();
		sym_mgr->reserve(1024);
		context_.setSymbolManager(std::move(sym_mgr));
		context_.setObjectiveManager(std::make_shared<ObjectiveManager>());
		context_.setOptions(std::make_shared<GlobalOptions>());

		// array cache
		array_select_cache.reserve(1024);
		array_normalize_cache.reserve(1024);

		parseSmtlib2File(filename);
	}

	Parser::~Parser() {	}

	RESULT_TYPE Parser::getResultType(){
		return result_type;
	}

	std::shared_ptr<DAGNode> Parser::getResult(){
		return result_node;
	}
	
	RESULT_TYPE Parser::checkSat(){
		if(result_type != RESULT_TYPE::RT_UNKNOWN){
			return result_type;
		}

		// simple check
		bool all_true = true;
		for(auto& assertion : context_.assertions){
			if(assertion->isErr()){
				result_type = RESULT_TYPE::RT_ERROR;
				return result_type;
			}
			else if(assertion->isFalse()){
				all_true = false;
				return RESULT_TYPE::RT_UNSAT;
			}
			else if(assertion->isTrue()){
				continue;
			}
			else{
				// unknown assertion
				result_type = RESULT_TYPE::RT_UNKNOWN;
				return result_type;
			}
		}
		if(all_true){
			result_type = RESULT_TYPE::RT_SAT;
		}
		return result_type;
	}
	
	std::shared_ptr<Model> Parser::getModel(){
		return result_model;
	}

	size_t Parser::getNodeCount(){
		// return getNodeManager()->size();
		// BFS to count the number of nodes
		// only count the nodes in assertions, assumptions, soft_assertions, soft_weights, objectives
		std::unordered_set<std::shared_ptr<DAGNode>> visited;
		std::queue<std::shared_ptr<DAGNode>> q;
		for(size_t i=0;i<context_.assertions.size();i++){
			auto node = context_.assertions[i];
			q.push(node);
			visited.insert(node);
		}
		for(size_t i=0;i<context_.assumptions.size();i++){
			for(size_t j=0;j<context_.assumptions[i].size();j++){
				auto node = context_.assumptions[i][j];
				q.push(node);
				visited.insert(node);
			}
		}
		for(size_t i=0;i<context_.soft_assertions.size();i++){
			auto node = context_.soft_assertions[i];
			q.push(node);
			visited.insert(node);
		}
		for(size_t i=0;i<context_.soft_weights.size();i++){
			auto node = context_.soft_weights[i];
			q.push(node);
			visited.insert(node);
		}
		for (const auto& obj : context_.getObjectives()) {
			auto node = obj->getObjectiveTerm();
			q.push(node);
			visited.insert(node);
		}
		while(!q.empty()){
			auto node = q.front();
			q.pop();
			for(size_t i=0;i<node->getChildrenSize();i++){
				auto child = node->getChild(i);
				if(visited.find(child) == visited.end()){
					visited.insert(child);
					q.push(child);
				}
			}
		}
		return visited.size();
	}
	
	// to solver
	Context& Parser::context() {
		return context_;
	}
	const Context& Parser::context() const {
		return context_;
	}

	std::vector<std::shared_ptr<DAGNode>> Parser::getAssertions() const{
		return context_.getAssertions();
	}
	std::unordered_map<std::string, std::unordered_set<size_t>> Parser::getGroupedAssertions() const{
		return context_.getGroupedAssertions();
	}
	std::vector<std::vector<std::shared_ptr<DAGNode>>> Parser::getAssumptions() const{
		return context_.getAssumptions();
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getSoftAssertions() const{
		return context_.getSoftAssertions();
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getSoftWeights() const{
		return context_.getSoftWeights();
	}
	std::unordered_map<std::string, std::unordered_set<size_t>> Parser::getGroupedSoftAssertions() const{
		return context_.getGroupedSoftAssertions();
	}
	std::vector<std::shared_ptr<Objective>> Parser::getObjectives() const{
		return context_.getObjectives();
	}
	
	void Parser::setOption(const std::string& key, const std::string& value){
		getOptions()->setOption(key, value);
	}
	void Parser::setOption(const std::string& key, const int& value){
		getOptions()->setOption(key, std::to_string(value));
	}
	void  Parser::setOption(const std::string& key, const double& value){
		getOptions()->setOption(key, std::to_string(value));
	}
	void Parser::setOption(const std::string& key, const bool& value){
		getOptions()->setOption(key, value?"true":"false");
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getVariables() const{
		std::vector<std::shared_ptr<DAGNode>> vars;
		for(const auto& var : getSymbolManager()->getVarNames())
			vars.emplace_back(var.second);
		for(const auto& var : getSymbolManager()->getTempVarNames())
			vars.emplace_back(var.second);
		return vars;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getDeclaredVariables() const{
		std::vector<std::shared_ptr<DAGNode>> vars;
		for(const auto& var : getSymbolManager()->getVarNames())
			vars.emplace_back(var.second);
		return vars;
	}

	// Sort-classified variable accessors
	std::vector<std::shared_ptr<DAGNode>> Parser::getBoolVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isBool()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getIntVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isInt()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getRealVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isReal()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getBvVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isBv()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getFpVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isFp()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getRoundingModeVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isRoundingMode()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getArrayVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isArray()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getDatatypeVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isDatatype()) out.emplace_back(kv.second);
		return out;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getStringVars() const{
		std::vector<std::shared_ptr<DAGNode>> out;
		for(const auto& kv : getSymbolManager()->getVarNames())
			if(kv.second->getSort() && kv.second->getSort()->isStr()) out.emplace_back(kv.second);
		return out;
	}
	size_t Parser::getNumBoolVars() const{ return getBoolVars().size(); }
	size_t Parser::getNumIntVars() const{ return getIntVars().size(); }
	size_t Parser::getNumRealVars() const{ return getRealVars().size(); }
	size_t Parser::getNumBvVars() const{ return getBvVars().size(); }
	size_t Parser::getNumFpVars() const{ return getFpVars().size(); }
	size_t Parser::getNumRoundingModeVars() const{ return getRoundingModeVars().size(); }
	size_t Parser::getNumArrayVars() const{ return getArrayVars().size(); }
	size_t Parser::getNumDatatypeVars() const{ return getDatatypeVars().size(); }
	size_t Parser::getNumStringVars() const{ return getStringVars().size(); }

	bool Parser::isRecursiveDatatype(const std::shared_ptr<Sort>& dt_sort) const {
		if (!dt_sort || !dt_sort->isDatatype()) return false;
		const std::string& target = dt_sort->name;

		// DFS over selector sorts: returns true if sort 's' transitively reaches target.
		// visited tracks explored non-target DT sorts to prevent cycles.
		std::unordered_set<std::string> visited;
		std::function<bool(const std::shared_ptr<Sort>&)> check;
		check = [&](const std::shared_ptr<Sort>& s) -> bool {
			if (!s || !s->isDatatype()) return false;
			if (s->name == target) return true;   // found a reference back to the target DT
			if (visited.count(s->name)) return false; // already explored, no path to target
			visited.insert(s->name);
			for (const auto& ctor : s->getDtConstructors())
				for (const auto& sel : ctor.selectors)
					if (check(sel.sort)) return true;
			return false;
		};

		for (const auto& ctor : dt_sort->getDtConstructors())
			for (const auto& sel : ctor.selectors)
				if (check(sel.sort)) return true;
		return false;
	}

	std::shared_ptr<DAGNode> Parser::mkDefaultDTValue(const std::shared_ptr<Sort>& dt_sort) {
		if (!dt_sort || !dt_sort->isDatatype()) return nullptr;

		// visited tracks DT sorts currently on the recursion stack to break cycles.
		std::unordered_set<std::string> visited;

		std::function<std::shared_ptr<DAGNode>(const std::shared_ptr<Sort>&)> helper;
		helper = [&](const std::shared_ptr<Sort>& s) -> std::shared_ptr<DAGNode> {
			if (!s) return nullptr;

			// For non-DT sorts delegate to getZero()
			if (!s->isDatatype()) {
				auto z = getZero(s);
				return (z && !z->isErr()) ? z : nullptr;
			}

			// Recursion guard: we are already building a default for this sort
			if (visited.count(s->name)) return nullptr;
			visited.insert(s->name);

			const auto& ctors = s->getDtConstructors();
			if (ctors.empty()) return nullptr;

			// Phase 1: prefer nullary constructors (immediate base case, no allocation)
			for (const auto& ctor : ctors) {
				if (ctor.selectors.empty())
					return getNodeManager()->createNode(s, NODE_KIND::NT_DT_CONSTRUCTOR, ctor.name, {});
			}

			// Phase 2: try each non-nullary constructor; use snapshot/restore so a
			// failed branch does not pollute visited for the next constructor.
			for (const auto& ctor : ctors) {
				auto visited_snapshot = visited;
				std::vector<std::shared_ptr<DAGNode>> args;
				bool ok = true;
				for (const auto& sel : ctor.selectors) {
					auto def_val = helper(sel.sort);
					if (!def_val) { ok = false; break; }
					args.push_back(def_val);
				}
				if (ok)
					return getNodeManager()->createNode(s, NODE_KIND::NT_DT_CONSTRUCTOR, ctor.name, args);
				visited = visited_snapshot; // restore for the next constructor attempt
			}

			return nullptr; // no well-founded constructor found
		};

		return helper(dt_sort);
	}

	std::shared_ptr<DAGNode> Parser::getVariable(const std::string& var_name){
		std::shared_ptr<DAGNode> v = getSymbolManager()->getVar(var_name);
		if(v) return v;
		v = getSymbolManager()->getTempVar(var_name);
		return v ? v : NodeManager::NULL_NODE;
	}
	std::vector<std::shared_ptr<DAGNode>> Parser::getFunctions() const{
		return getSymbolManager()->getFunctions();
	}
	void Parser::setEvaluatePrecision(mpfr_prec_t precision){
		getOptions()->setEvaluatePrecision(precision);
	}
	mpfr_prec_t Parser::getEvaluatePrecision() const{
		return getOptions()->getEvaluatePrecision();
	}
	void Parser::setEvaluateUseFloating(bool use_floating){
		getOptions()->setEvaluateUseFloating(use_floating);
	}
	bool Parser::getEvaluateUseFloating() const{
		return getOptions()->getEvaluateUseFloating();
	}
	void Parser::setStrictSmtlib(bool strict){
		getOptions()->setStrictSmtlib(strict);
	}
	bool Parser::getStrictSmtlib() const{
		return getOptions()->getStrictSmtlib();
	}
	Real Parser::toReal(std::shared_ptr<DAGNode> expr){
		ensureNumberValue(expr);
		condAssert(expr->isCReal() || expr->isCInt(), "Cannot convert non-constant expression to real");
		if(expr->isPi()) return Real::pi(getEvaluatePrecision());
		if(expr->isE()) return Real::e(getEvaluatePrecision());
		auto value = expr->getValue();
		if(value == nullptr) {
			// If value is still nullptr after ensureNumberValue, return 0 as default
			// This can happen if the expression cannot be converted to a number
			return Real(0, getEvaluatePrecision());
		}
		return value->getNumberValue().toReal(getEvaluatePrecision());
	}
	Integer Parser::toInt(std::shared_ptr<DAGNode> expr){
		ensureNumberValue(expr);
		condAssert(expr->isCInt(), "Cannot convert non-integer expression to integer");
		auto value = expr->getValue();
		if(value == nullptr) {
			// If value is still nullptr after ensureNumberValue, return 0 as default
			// This can happen if the expression cannot be converted to a number
			return Integer(0);
		}
		return value->getNumberValue().toInteger();
	}
	bool Parser::isZero(std::shared_ptr<DAGNode> expr){
		// cannot check zero for root-obj and root-of-with-interval
		if(expr->isCRootObj()) return false;
		if(expr->isCRootOfWithInterval()) return false;
		// otherwise, check zero
		if(expr->isCReal()) return toReal(expr) == 0.0;
		if(expr->isCInt()) return toInt(expr) == 0;
		return false;
	}
	bool Parser::isOne(std::shared_ptr<DAGNode> expr){
		if(expr->isCReal()) return toReal(expr) == 1.0;
		if(expr->isCInt()) return toInt(expr) == 1;
		return false;
	}

	void Parser::ensureNumberValue(std::shared_ptr<DAGNode> expr){
		if(!expr || !expr->isConst()) return;
		if(expr->getValue()!=nullptr) return;

		std::string s = expr->toString();
		try{
			if(TypeChecker::isInt(s)){
				Integer i(s);
				expr->setValue(i);
			}
			else if(TypeChecker::isReal(s)){
				// dynamic precision
				size_t digits = 0;
				for(char c: s){ if(std::isdigit(c)) digits++; }
				mpfr_prec_t prec = digits*4 + 16;
				Real r(s, prec);
				expr->setValue(r);
			}
		}catch(...){
			// raise error
			err_all(expr, "Cannot convert non-number expression to number", line_number);
		}
	}

	// parse smt-lib2 file
	std::string Parser::getSymbol() {
		condAssert(!buffer || (bufptr >= buffer && bufptr <= buffer + buflen), "getSymbol: buffer bounds");
		char *beg = bufptr;
		bool in_scientific_notation = false;
		bool has_open_bracket = false;
		int bracket_level = 0;

		// first char was already scanned
		bufptr++;

		// while not eof	
		while (*bufptr != 0) {

			switch (scan_mode) {
			case SCAN_MODE::SM_SYMBOL:
				// check if in scientific notation mode
				if (!in_scientific_notation) {
					// check if current symbol is the start of scientific notation
					std::string current(beg, bufptr - beg);
					size_t e_pos = current.find_first_of("Ee");
					if (e_pos != std::string::npos && e_pos > 0 && e_pos == current.size() - 1) {
						// check if the part before E is a valid real number
						std::string mantissa = current.substr(0, e_pos);
						if (TypeChecker::isReal(mantissa)) {
							// confirm the start of scientific notation
							in_scientific_notation = true;
						}
					}
				}

				// if in scientific notation mode
				if (in_scientific_notation) {
					// handle left parenthesis
					if (*bufptr == '(') {
						has_open_bracket = true;
						bracket_level++;
						bufptr++;
						continue;
					}
					// handle right parenthesis
					else if (*bufptr == ')' && has_open_bracket) {
						bracket_level--;
						if (bracket_level == 0) {
							// right parenthesis matched, end scientific notation
							bufptr++;
							std::string tmp_s(beg, bufptr - beg);
							scanToNextSymbol();
							return tmp_s;
						}
						bufptr++;
						continue;
					}
					// handle right parenthesis when no open bracket — end of symbol
					else if (*bufptr == ')' && !has_open_bracket) {
						std::string tmp_s(beg, bufptr - beg);
						return tmp_s;
					}
					// handle space, allow space in scientific notation mode
					else if (isblank(*bufptr)) {
						bufptr++;
						continue;
					}
					// if encounter newline or other special characters, end scientific notation mode
					else if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f' ||
							 *bufptr == ';' || *bufptr == '|' || *bufptr == '"') {
						in_scientific_notation = false;
						// return the parsed part
						std::string tmp_s(beg, bufptr - beg);
						return tmp_s;
					}
				}
				// normal symbol parsing
				else {
					if (isblank(*bufptr)) {
						// out of symbol mode by ' ' and \t
						std::string tmp_s(beg, bufptr - beg);
						// skip space
						bufptr++;
						scanToNextSymbol();
						return tmp_s;
					}
					else if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
						line_number++;
						// out of symbol mode by '\n', '\r', '\v' and '\f'
						std::string tmp_s(beg, bufptr - beg);
						// skip space
						bufptr++;
						scanToNextSymbol();
						return tmp_s;
					}
					else if (*bufptr == ';' || *bufptr == '|' || *bufptr == '"' || *bufptr == '(' || *bufptr == ')') {
						// out of symbol mode by ';', '|', '"', '(' and ')'
						std::string tmp_s(beg, bufptr - beg);
						return tmp_s;
					}
				}
				break;

			case SCAN_MODE::SM_COMP_SYM:
				if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
					line_number++;
				}
				else if (*bufptr == '|') {
					// out of complicated symbol mode
					bufptr++;
					std::string tmp_s(beg, bufptr - beg);
					// skip space
					scanToNextSymbol();
					return tmp_s;
				}
				break;

			case SCAN_MODE::SM_STRING:
				if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
					line_number++;
				}
				else if (*bufptr == '"') {
					// process the nested quotes - check if it is an escape quote
					if (bufptr + 1 < buffer + buflen && *(bufptr + 1) == '"') {
						// two consecutive quotes are escape quotes, skip the second quote
						bufptr++;
					} else {
						// end of string
						bufptr++;
						std::string tmp_s(beg, bufptr - beg);
						// skip space
						scanToNextSymbol();
						return tmp_s;
					}
				}
				break;

			default:
				condAssert(false, "Invalid scan mode");
			}

			// go next char
			bufptr++;
		}

		if(parsing_file){
			err_unexp_eof();
		}
		else{
			std::string tmp_s(beg, bufptr - beg);
			// skip space
			scanToNextSymbol();
			return tmp_s;
		}

		return "";
	}

	void Parser::scanToNextSymbol() {
		condAssert(!buffer || (bufptr >= buffer && bufptr <= buffer + buflen), "scanToNextSymbol: buffer bounds");
		// init scan mode
		scan_mode = SCAN_MODE::SM_COMMON;

		// while not eof
		while (*bufptr != 0) {

			if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {

				line_number++;

				// out of comment mode
				if (scan_mode == SCAN_MODE::SM_COMMENT) scan_mode = SCAN_MODE::SM_COMMON;

			}
			else if (scan_mode == SCAN_MODE::SM_COMMON && !isblank(*bufptr)) {

				switch (*bufptr) {
				case ';':
					// encounter comment
					scan_mode = SCAN_MODE::SM_COMMENT;
					break;
				case '|':
					// encounter next complicated symbol
					scan_mode = SCAN_MODE::SM_COMP_SYM;
					return;
				case '"':
					// encounter next std::string
					scan_mode = SCAN_MODE::SM_STRING;
					return;
				default:
					// encounter next symbol
					scan_mode = SCAN_MODE::SM_SYMBOL;
					return;
				}

			}

			// go next char
			bufptr++;
		}

	}

	void Parser::parseLpar() {
		if (*bufptr == '(') {
			bufptr++;
			scanToNextSymbol();
		}
		else {
			err_sym_mis("(", line_number);
		}
	}

	void Parser::parseRpar() {
		if (*bufptr == ')') {
			bufptr++;
			scanToNextSymbol();
		}
		else {
			err_sym_mis(")", line_number);
		}
	}

	void Parser::skipToRpar() {

		// skip to next right parenthesis with same depth	
		scan_mode = SCAN_MODE::SM_COMMON;
		size_t level = 0;

		while (*bufptr != 0) {

			if (*bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
				// new line
				line_number++;
				if (scan_mode == SCAN_MODE::SM_COMMENT)
					scan_mode = SCAN_MODE::SM_COMMON;
			}
			else if (scan_mode == SCAN_MODE::SM_COMMON) {

				if (*bufptr == '(') level++;
				else if (*bufptr == ')') {
					if (level == 0) return;
					else level--;
				}
				else if (*bufptr == ';')
					scan_mode = SCAN_MODE::SM_COMMENT;
				else if (*bufptr == '|')
					scan_mode = SCAN_MODE::SM_COMP_SYM;
				else if (*bufptr == '"')
					scan_mode = SCAN_MODE::SM_STRING;

			}
			else if (scan_mode == SCAN_MODE::SM_COMP_SYM && *bufptr == '|')
				scan_mode = SCAN_MODE::SM_COMMON;
			else if (scan_mode == SCAN_MODE::SM_STRING && *bufptr == '"')
				scan_mode = SCAN_MODE::SM_COMMON;

			// go to next char
			bufptr++;
		}

	}

	// parse smt-lib2 file
	bool Parser::parseSmtlib2File(const std::string filename) {

		/*
		load file
		*/
		std::ifstream fin(filename, std::ifstream::binary);

		if (!fin) {
			std::cout << "error: Cannot open file \"" << filename << "\"." << std::endl;
			return false;
		}

		fin.seekg(0, std::ios::end);
		buflen = (long)fin.tellg();
		fin.seekg(0, std::ios::beg);

		buffer = new char[buflen + 1];
		fin.read(buffer, buflen);
		buffer[buflen] = 0;

		fin.close();

		/*
		parse command
		*/
		parsing_file = true;
		bufptr = buffer;
		if (buflen > 0) line_number = 1;

		scanToNextSymbol();

		try {
			while (*bufptr) {
				parseLpar();
				CMD_TYPE type = parseCommand();
				if (command_logging_) {
					Command cmd(type);
					cmd.line_number = line_number;
					script_.addCommand(cmd);
				}
				if (type == CMD_TYPE::CT_EXIT) break;
				parseRpar();
			}
		} catch (const ParseErrorException&) {
			bufptr = nullptr;
			if (buffer) { delete[] buffer; buffer = nullptr; }
			buflen = 0;
			return false;
		}
		bufptr = nullptr;
		delete[] buffer;
		buffer = nullptr;
		return true;
	}

	char* safe_strdup(const std::string& str) {
		if (str.empty()) return nullptr;
		
		char* new_str = new (std::nothrow) char[str.length() + 1];
		if (!new_str) return nullptr;
		
		std::memcpy(new_str, str.c_str(), str.length() + 1);
		return new_str;
	}

	bool Parser::parseStr(const std::string& constraint) {
		if (constraint.empty()) return true;
		buffer = safe_strdup(constraint);
		if (!buffer) return false;
		buflen = constraint.length();
		bufptr = buffer;
		if (buflen > 0) line_number = 1;
		scanToNextSymbol();
		try {
			while (*bufptr) {
				parseLpar();
				CMD_TYPE type = parseCommand();
				if (command_logging_) {
					Command cmd(type);
					cmd.line_number = line_number;
					script_.addCommand(cmd);
				}
				if (type == CMD_TYPE::CT_EXIT) break;
				parseRpar();
			}
		} catch (const ParseErrorException&) {
			bufptr = nullptr;
			if (buffer) { delete[] buffer; buffer = nullptr; }
			buflen = 0;
			return false;
		}
		bufptr = nullptr;
		delete[] buffer;
		buffer = nullptr;
		return true;
	}

	bool Parser::assert(const std::string& constraint) {
		parsing_file = false;
		std::shared_ptr<DAGNode> expr = mkExpr(constraint);
		context_.assertions.emplace_back(expr);
		return true;
	}

	bool Parser::assert(std::shared_ptr<DAGNode> node) {
		context_.assertions.emplace_back(node);
		return true;
	}

	std::shared_ptr<DAGNode> Parser::mkExpr(const std::string& expression) {
		parsing_file = false;
		if (expression.empty()) {
			return mkErr(ERROR_TYPE::ERR_UNEXP_EOF);
		}
		
		buffer = safe_strdup(expression);
		if (!buffer) {
			return mkErr(ERROR_TYPE::ERR_UNEXP_EOF);
		}
		
		buflen = expression.length();
		bufptr = buffer;
		if (buflen > 0) line_number = 1;
		scanToNextSymbol();
		std::shared_ptr<DAGNode> expr = parseExpr();
		
		bufptr = nullptr;
		delete[] buffer;
		buffer = nullptr;
		return expr;
	}
	

	KEYWORD Parser::parseKeyword(){
		
		size_t key_ln = line_number;
		//std::cout << "line_number = " << key_ln << std::endl;
		std::string key = getSymbol();
		//std::cout << "key = " << key << std::endl;
		if(key == ":id"){
			return KEYWORD::KW_ID;
		}
		else if(key == ":weight"){
			return KEYWORD::KW_WEIGHT;
		}
		else if (key == ":comp"){
			return KEYWORD::KW_COMP;
		}
		else if (key == ":epsilon"){
			return KEYWORD::KW_EPSILON;
		}
		else if(key == ":M"){
			return KEYWORD::KW_M;
		}else if (key == ":named"){
			return KEYWORD::KW_NAMED;
		}
		else{
			err_unkwn_sym(key, key_ln);
		}
		return KEYWORD::KW_NULL;
	}

	CMD_TYPE Parser::parseCommand() {

		size_t command_ln = line_number;
		std::string command = getSymbol();

		// (assert <expr>) or (assert <expr> [:id <symbol>])
		if (command == "assert") {
			std::string grp_id = "";
			std::string named_name = "";

			KEYWORD key = attemptParseKeywords();
			if(key == KEYWORD::KW_ID){
				// (assert [:id <symbol>] <expr>)
				grp_id = getSymbol();
			}
			std::shared_ptr<DAGNode> assert_expr = parseExpr();
			size_t index = context_.assertions.size();
			context_.assertions.emplace_back(assert_expr);
			// 
			if(grp_id == ""){
				KEYWORD key_ = attemptParseKeywords();
				if(key_ == KEYWORD::KW_ID){
					// (assert <expr> [:id <symbol>])
					grp_id = getSymbol();
				}
			}
			if (named_name == ""){
				KEYWORD key_ = attemptParseKeywords();
				if(key_ == KEYWORD::KW_NAMED){
					// (assert <expr> (! expr :named <symbol>))
					named_name = getSymbol();
				}
			}
			// if grp_id is not empty, insert to assertion_groups
			if(grp_id != ""){
				if(context_.assertion_groups.find(grp_id) == context_.assertion_groups.end()){
					context_.assertion_groups.insert(std::pair<std::string, std::unordered_set<size_t>>(grp_id, {index}));
				}
				else{
					context_.assertion_groups[grp_id].insert(index);
				}
			}
			//if named_name is not empty, insert to named_assertions
			if (named_name != ""){
				context_.named_assertions[named_name] = assert_expr;
				context_.registerNamedAssertionInScope(named_name);
			}
			if(grp_id != ""){
				context_.registerAssertionGroupInScope(grp_id);
			}
			skipToRpar();
			return CMD_TYPE::CT_ASSERT;
		}
		// (assert-soft <expr> [:weight <numeral>] [:id <symbol>])
		if (command == "assert-soft") {
			parseAssertSoft();
			skipToRpar();
			return CMD_TYPE::CT_ASSERT_SOFT;
		}

		// (check-sat)
		if (command == "check-sat") {
			getOptions()->check_sat = true;
			skipToRpar();
			return CMD_TYPE::CT_CHECK_SAT;
		}

		// (check-sat-assuming (a1, ..., an)), maybe for future incremental mode.
		if (command == "check-sat-assuming") {
			//parse (a1, ..., an)
			parseLpar();
			std::vector<std::shared_ptr<DAGNode>> cur_assumptions;
			while (*bufptr != ')') {
				std::shared_ptr<DAGNode> assump = parseExpr();
				cur_assumptions.emplace_back(assump);
			}
			context_.assumptions.emplace_back(cur_assumptions);
			skipToRpar();
			return CMD_TYPE::CT_CHECK_SAT_ASSUMING;
		}

		// (declare-const <symbol> <sort>)
		if (command == "declare-const") {

			// get name
			size_t name_ln = line_number;
			std::string name = getSymbol();

			// get returned type
			std::shared_ptr<DAGNode> res = nullptr;
			std::shared_ptr<Sort> sort = parseSort();
			res = mkVar(sort, name);

			// multiple declarations
			if (res->isErr()) err_all(res, name, name_ln);
			else context_.registerVarInScope(name);
			skipToRpar();

			return CMD_TYPE::CT_DECLARE_CONST;
		}

		// (declare-fun <symbol> (<sort>*) <sort>)
		if (command == "declare-fun") {

			// get name
			size_t name_ln = line_number;
			std::string name = getSymbol();

			// (declare-fun <symbol> (<sort>*) <sort>)
			//                       ^
			parseLpar();
			// (declare-fun <symbol> (<sort>*) <sort>)
			//                               ^
			std::shared_ptr<DAGNode> res = nullptr;
			if(*bufptr == ')'){
				// (declare-fun <symbol> () <sort>)
				parseRpar();
				std::shared_ptr<Sort> out_sort = parseSort();
				res = mkVar(out_sort, name);
				if(!res->isErr()) context_.registerVarInScope(name);
			}
			else{
				// (declare-fun <symbol> (<sort>+) <sort>)
				std::vector<std::shared_ptr<Sort>> params;
				while(*bufptr != ')'){
					params.emplace_back(parseSort());
				}
				parseRpar();
				std::shared_ptr<Sort> out_sort = parseSort();
				res = mkFuncDec(name, params, out_sort);
				if(!res->isErr()){
					getSymbolManager()->addFunctionName(name);
					context_.registerFunInScope(name);
				}
			}

			//multiple declarations
			if (res->isErr()) err_all(res, name, name_ln);
			skipToRpar();

			return CMD_TYPE::CT_DECLARE_FUN;
		}

		// (declare-sort <symbol> <numeral>)
		if (command == "declare-sort") {
			//get name
			std::string name = getSymbol();

			//get numeral
			std::string numeral = getSymbol();
			size_t num = std::stoi(numeral);

			// make sort
			std::shared_ptr<Sort> sort = mkSortDec(name, num);
			getSymbolManager()->registerSort(name, sort);
			context_.registerSortInScope(name);
			skipToRpar();

			return CMD_TYPE::CT_DECLARE_SORT;
		}

		// (declare-datatype <symbol> <datatype-dec>)
		// <datatype-dec> ::= (<constructor-dec>+)
		if (command == "declare-datatype") {
			std::string dt_name = getSymbol();
			auto ps = getSortManager()->createSort(SORT_KIND::SK_DATATYPE, dt_name, 0);
			getSymbolManager()->registerSort(dt_name, ps);
			context_.registerSortInScope(dt_name);
			parseLpar();
			defineDatatypeConstructors(ps);
			skipToRpar();
			return CMD_TYPE::CT_DECLARE_DATATYPES;
		}

		// (declare-datatypes (<sort-dec>+) (<datatype-dec>+))
		// where:
		//   <sort-dec>    ::= (<symbol> <numeral>)
		//   <datatype-dec>::= (<constructor-dec>+)
		//   <constructor-dec>::= (<symbol> <selector-dec>*)
		//   <selector-dec>::= (<symbol> <sort>)
		if (command == "declare-datatypes") {
			// -- Phase 1: read sort names so we can register forward references --
			parseLpar(); // outer '(' for sort list
			std::vector<std::pair<std::string, size_t>> sort_decls;
			while(*bufptr != ')') {
				parseLpar();
				std::string dt_name = getSymbol();
				std::string arity_str = getSymbol();
				size_t dt_arity = static_cast<size_t>(std::stoi(arity_str));
				sort_decls.emplace_back(dt_name, dt_arity);
				parseRpar();
			}
			parseRpar(); // close sort list

			// Pre-register placeholder sorts so mutual recursion works
			std::vector<std::shared_ptr<Sort>> placeholder_sorts;
			for(auto& sd : sort_decls) {
				auto ps = getSortManager()->createSort(SORT_KIND::SK_DATATYPE, sd.first, sd.second);
				getSymbolManager()->registerSort(sd.first, ps);
				context_.registerSortInScope(sd.first);
				placeholder_sorts.push_back(ps);
			}

			// -- Phase 2: parse constructor lists --
			parseLpar(); // outer '(' for datatype list
			for(size_t ti = 0; ti < sort_decls.size(); ++ti) {
				parseLpar(); // '(' for this datatype
				defineDatatypeConstructors(placeholder_sorts[ti]);
			}
			parseRpar(); // close outer datatype list
			skipToRpar(); // close the declare-datatypes command

			return CMD_TYPE::CT_DECLARE_DATATYPES;
		}

		// (define-const <symbol> <sort> <expr>)
		if (command == "define-const") {
			// get name
			size_t name_ln = line_number;
			std::string name = getSymbol();

			std::shared_ptr<DAGNode> check_fun = getSymbolManager()->getFun(name);
			if(check_fun && check_fun->getKind() == NODE_KIND::NT_FUNC_DEF){
				err_mul_def(name, name_ln);
			}
			if(check_fun){
				return CMD_TYPE::CT_DEFINE_FUN;
			}
			// keep the function name with the same order
			getSymbolManager()->addFunctionName(name);
			context_.registerFunInScope(name);

			// get returned type and body
			std::shared_ptr<Sort> out_sort = parseSort();
			std::shared_ptr<DAGNode> func_body = parseExpr();
			std::vector<std::shared_ptr<DAGNode>> params; // empty params for constant
			std::shared_ptr<DAGNode> res = mkFuncDef(name, params, out_sort, func_body);
			skipToRpar();
			
			return CMD_TYPE::CT_DEFINE_FUN;
		}

		//(define-fun <symbol> ((<symbol> <sort>)*) <sort> <expr>)
		if (command == "define-fun") {

			// get name
			size_t name_ln = line_number;
			std::string name = getSymbol();

			std::shared_ptr<DAGNode> check_fun = getSymbolManager()->getFun(name);
			if(check_fun && check_fun->getKind() == NODE_KIND::NT_FUNC_DEF){
				err_mul_def(name, name_ln);
			}
			if(check_fun){
				return CMD_TYPE::CT_DEFINE_FUN;
			}
			// keep the function name with the same order
			getSymbolManager()->addFunctionName(name);
			context_.registerFunInScope(name);

			// parse ((x Int))
			//       ^
			parseLpar();
			std::vector<std::shared_ptr<DAGNode>> params;
			std::vector<std::string> key_list;
			while(*bufptr!=')'){ // there are still (x Int) left.
				// (x Int)
				// ^
				parseLpar();
				std::string pname = getSymbol();
				std::shared_ptr<Sort> ptype = parseSort();
				key_list.emplace_back(pname);
				std::shared_ptr<DAGNode> expr = nullptr;
				expr = mkFunParamVar(ptype, pname);
				// multiple declarations
				if(getSymbolManager()->hasFunVar(pname)){
					err_mul_decl(pname, line_number);
				}
				else{
					getSymbolManager()->registerFunVar(pname, expr);
					params.emplace_back(expr);
				}
				// (x Int)
				//		 ^
				parseRpar();
			}
			
			//(define-fun <symbol> ((<symbol> <sort>)*) <sort> <expr>)
			//					                      ^
			parseRpar();

			//get returned type
			std::shared_ptr<Sort> out_sort = parseSort();
			std::shared_ptr<DAGNode> func_body = parseExpr();
			std::shared_ptr<DAGNode> res = mkFuncDef(name, params, out_sort, func_body);
			skipToRpar();

			//remove key bindings: for let uses local variables. 
			while (key_list.size() > 0) {
				getSymbolManager()->eraseFunVar(key_list.back());
				key_list.pop_back();
			}
			
			return CMD_TYPE::CT_DEFINE_FUN;
		}

		// (define-fun-rec <symbol> ((<symbol> <sort>)*) <sort> <expr>)
		// recursive function definition
		if (command == "define-fun-rec") {
			// get name
			size_t name_ln = line_number;
			std::string name = getSymbol();

			std::shared_ptr<DAGNode> check_fun = getSymbolManager()->getFun(name);
			if(check_fun && check_fun->getKind() == NODE_KIND::NT_FUNC_DEF){
				err_mul_def(name, name_ln);
			}
			if(check_fun){
				return CMD_TYPE::CT_DEFINE_FUN_REC;
			}
			// keep the function name with the same order
			getSymbolManager()->addFunctionName(name);
			context_.registerFunInScope(name);

			// parse ((x Int))
			//       ^
			parseLpar();
			std::vector<std::shared_ptr<DAGNode>> params;
			std::vector<std::string> key_list;
			std::vector<std::shared_ptr<Sort>> param_sorts;
			while(*bufptr!=')'){ // there are still (x Int) left.
				// (x Int)
				// ^
				parseLpar();
				std::string pname = getSymbol();
				std::shared_ptr<Sort> ptype = parseSort();
				key_list.emplace_back(pname);
				param_sorts.emplace_back(ptype);
				std::shared_ptr<DAGNode> expr = nullptr;
				expr = mkFunParamVar(ptype, pname);
				// multiple declarations
				if(getSymbolManager()->hasFunVar(pname)){
					err_mul_decl(pname, line_number);
				}
				else{
					getSymbolManager()->registerFunVar(pname, expr);
					params.emplace_back(expr);
				}
				// (x Int)
				//		 ^
				parseRpar();
			}
			
			//(define-fun-rec <symbol> ((<symbol> <sort>)*) <sort> <expr>)
			//					                        ^
			parseRpar();

			//get returned type
			std::shared_ptr<Sort> out_sort = parseSort();
			
			// For recursive functions, we need to create a function declaration first
			// so it can be referenced in its own body
			std::shared_ptr<DAGNode> func_dec = mkFuncDec(name, param_sorts, out_sort);
			
			// Now parse the function body (which can reference the function itself)
			std::shared_ptr<DAGNode> func_body = parseExpr();
			std::shared_ptr<DAGNode> res = mkFuncRec(name, params, out_sort, func_body);
			skipToRpar();

			//remove key bindings: for let uses local variables. 
			while (key_list.size() > 0) {
				getSymbolManager()->eraseFunVar(key_list.back());
				key_list.pop_back();
			}
			
			return CMD_TYPE::CT_DEFINE_FUN_REC;
		}

		if (command == "define-funs-rec") {
			// (define-funs-rec ((name1 ((param1 type1)...) ret_type1)...) (body1 body2...))
			
			// Parse function declarations first
			parseLpar(); // for function declarations list
			std::vector<std::string> func_names;
			std::vector<std::vector<std::shared_ptr<DAGNode>>> all_params;
			std::vector<std::vector<std::string>> all_key_lists;
			std::vector<std::vector<std::shared_ptr<Sort>>> all_param_sorts;
			std::vector<std::shared_ptr<Sort>> return_sorts;
			
			while(*bufptr != ')') {
				// Parse each function declaration: (name ((param1 type1)...) ret_type)
				parseLpar();
				std::string name = getSymbol();
				
				std::shared_ptr<DAGNode> check_fun = getSymbolManager()->getFun(name);
				if(check_fun && check_fun->getKind() == NODE_KIND::NT_FUNC_DEF){
					err_mul_def(name, line_number);
				}
				if(check_fun){
					skipToRpar();
					continue;
				}
				
				func_names.emplace_back(name);
				getSymbolManager()->addFunctionName(name);
				
				// Parse parameters: ((param1 type1)...)
				parseLpar();
				std::vector<std::shared_ptr<DAGNode>> params;
				std::vector<std::string> key_list;
				std::vector<std::shared_ptr<Sort>> param_sorts;
				
				while(*bufptr != ')') {
					parseLpar();
					std::string pname = getSymbol();
					std::shared_ptr<Sort> ptype = parseSort();
					key_list.emplace_back(pname);
					param_sorts.emplace_back(ptype);
					std::shared_ptr<DAGNode> expr = mkFunParamVar(ptype, pname);
					params.emplace_back(expr);
					parseRpar();
				}
				parseRpar(); // end of parameters
				
				// Parse return type
				std::shared_ptr<Sort> out_sort = parseSort();
				return_sorts.emplace_back(out_sort);
				
				all_params.emplace_back(params);
				all_key_lists.emplace_back(key_list);
				all_param_sorts.emplace_back(param_sorts);
				
				parseRpar(); // end of function declaration
			}
			parseRpar(); // end of function declarations list
			
			// Create function declarations for all functions first
			// so they can be referenced in each other's bodies
			for(size_t i = 0; i < func_names.size(); i++) {
				mkFuncDec(func_names[i], all_param_sorts[i], return_sorts[i]);
			}
			
			// Parse function bodies
			parseLpar(); // for function bodies list
			for(size_t i = 0; i < func_names.size(); i++) {
				// Add parameter bindings for this function
				for(size_t j = 0; j < all_key_lists[i].size(); j++) {
					getSymbolManager()->registerFunVar(all_key_lists[i][j], all_params[i][j]);
				}
				
				// Parse function body
				std::shared_ptr<DAGNode> func_body = parseExpr();
				std::shared_ptr<DAGNode> res = mkFuncRec(func_names[i], all_params[i], return_sorts[i], func_body);
				
				// Remove parameter bindings for this function
				for(const auto& key : all_key_lists[i]) {
					getSymbolManager()->eraseFunVar(key);
				}
			}
			parseRpar(); // end of function bodies list
			
			skipToRpar();
			return CMD_TYPE::CT_DEFINE_FUNS_REC;
		}

		// (define-sort <symbol> (<symbol>*) <sort>)
		// <symbol>* is a list of symbols that represent template parameters.
		// for example, (define-sort List (T) (List T))
		// T is a template parameter.
		// then (define-sort IntList () (List Int)) is a valid command.
		if (command == "define-sort") {
			// get name
			std::string name = getSymbol();

			// get params (symbols)
			std::vector<std::string> param_names;
			parseLpar();
			while(*bufptr != ')'){
				param_names.push_back(getSymbol());
			}
			parseRpar();
			
			// convert param names to Sort parameters
			std::vector<std::shared_ptr<Sort>> params;
			for(const auto& name : param_names) {
				params.push_back(getSortManager()->createSort(name));
			}

			// get out sort
			std::shared_ptr<Sort> out_sort = parseSort();
			if(params.size() == 0){
				// it means an alias of the out sort
				getSymbolManager()->registerSort(name, out_sort);
			}
			else{
				std::shared_ptr<Sort> sort = mkSortDef(name, params, out_sort);
				getSymbolManager()->registerSort(name, sort);
			}
			skipToRpar();
			return CMD_TYPE::CT_DEFINE_SORT;
		}

		if (command == "echo") {
			// ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_ECHO;
		}

		// (exit)
		if (command == "exit") {
			skipToRpar();
			return CMD_TYPE::CT_EXIT;
		}

		// (get-assertions)
		// but used in interactive mode, so ignore it.
		if (command == "get-assertions") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_ASSERTIONS;
		}

		if (command == "get-assignment") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_ASSIGNMENT;
		}

		if (command == "get-info") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_INFO;
		}

		if (command == "get-option") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_OPTION;
		}

		if (command == "get-model") {
			//ignore
			getOptions()->get_model = true;
			skipToRpar();
			return CMD_TYPE::CT_GET_MODEL;
		}

		if (command == "get-option") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_OPTION;
		}

		if (command == "get-proof") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_PROOF;
		}

		if (command == "get-unsat-assumptions") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_UNSAT_ASSUMPTIONS;
		}

		if (command == "get-unsat-core") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_UNSAT_CORE;
		}

		if (command == "get-value") {
			//ignore
			warn_cmd_nsup(command, command_ln);
			skipToRpar();
			return CMD_TYPE::CT_GET_VALUE;
		}

		if (command == "pop") {
			size_t n = 1;
			if (*bufptr != ')') {
				std::string num = getSymbol();
				n = std::stoul(num);
			}
			context_.popScope(n);
			skipToRpar();
			return CMD_TYPE::CT_POP;
		}

		if (command == "push") {
			size_t n = 1;
			if (*bufptr != ')') {
				std::string num = getSymbol();
				n = std::stoul(num);
			}
			context_.pushScope(n);
			skipToRpar();
			return CMD_TYPE::CT_PUSH;
		}

		if (command == "reset") {
			context_.resetAll();
			skipToRpar();
			return CMD_TYPE::CT_RESET;
		}

		if (command == "reset-assertions") {
			context_.resetAssertions();
			skipToRpar();
			return CMD_TYPE::CT_RESET_ASSERTIONS;
		}

		//<attribute ::= <keyword> | <keyword> <attribute_value>
		//(set-info <attribute>)
		if (command == "set-info") {
			skipToRpar();
			return CMD_TYPE::CT_SET_INFO;
		}

		//(set-logic <symbol>)
		if (command == "set-logic") {
			size_t type_ln = line_number;
			std::string type = getSymbol();
			bool is_valid = getOptions()->setLogic(type);
			if(!is_valid){
				err_unkwn_sym(type, type_ln);
			}

			return CMD_TYPE::CT_SET_LOGIC;
		}

		//<option ::= <attribute>
		//(set-option <option>)
		if (command == "set-option") {
			skipToRpar();
			return CMD_TYPE::CT_SET_OPTION;
		}
		
		// quantifier
		// (quantifier ((<symbol> <sort>)+) <expr>)
		if(command == "exists") {
			in_quantifier_scope = true;
			quant_nesting_depth++;
			parseQuant("exists");
			quant_nesting_depth--;
			if(quant_nesting_depth == 0){
				in_quantifier_scope = false;
			}
			skipToRpar();
			return CMD_TYPE::CT_EXISTS;
		}
		if(command == "forall") {
			in_quantifier_scope = true;
			quant_nesting_depth++;
			parseQuant("forall");
			quant_nesting_depth--;
			if(quant_nesting_depth == 0){
				in_quantifier_scope = false;
			}
			skipToRpar();
			return CMD_TYPE::CT_FORALL;
		}

		// optimization
		if(command == "get-objectives"){
			getOptions()->get_objectives = true;
			skipToRpar();
			return CMD_TYPE::CT_GET_OBJECTIVES;
		}

		// (maximize <expr> [:comp <symbol>] [:epsilon <symbol>] [:M <symbol>] [:id <symbol>])
		if(command == "maximize"){
			parseMaximize();
			skipToRpar();
			return CMD_TYPE::CT_MAXIMIZE;
		}

		// (minimize <expr> [:comp <symbol>] [:epsilon <symbol>] [:M <symbol>] [:id <symbol>])
		if(command == "minimize"){
			parseMinimize();
			skipToRpar();
			return CMD_TYPE::CT_MINIMIZE;
		}

		// multi-objective optimization
		// (define-objective <symbol> <expr> [:comp <symbol>] [:epsilon <symbol>] [:M <symbol>] [:id <symbol>])
		if(command == "define-objective"){
			parseDefObj();
			skipToRpar();
			return CMD_TYPE::CT_DEFINE_OBJ;
		}
		// (lex-optimize (<symbol>+) [:id <symbol>])
		if(command == "lex-optimize"){
			parseLexOpt();
			skipToRpar();
			return CMD_TYPE::CT_LEX_OPTIMIZE;
		}
		// (pareto-optimize (<symbol>+) [:id <symbol>])
		if(command == "pareto-optimize"){
			parseParetoOpt();
			skipToRpar();
			return CMD_TYPE::CT_PARETO_OPTIMIZE;
		}
		// (box-optimize (<symbol>+) [:id <symbol>])
		if(command == "box-optimize"){
			parseBoxOpt();
			skipToRpar();
			return CMD_TYPE::CT_BOX_OPTIMIZE;
		}
		// (minmax (<symbol>+) [:id <symbol>])
		if(command == "minmax"){
			parseMinmax();
			skipToRpar();
			return CMD_TYPE::CT_MINMAX;
		}
		// (maxmin (<symbol>+) [:id <symbol>])
		if(command == "maxmin"){
			parseMaxmin();
			skipToRpar();
			return CMD_TYPE::CT_MAXMIN;
		}
		// (maxsat [:id <symbol>])
		if(command == "maxsat"){
			parseMaxsat();
			skipToRpar();
			return CMD_TYPE::CT_MAXSAT;
		}
		// (minsat [:id <symbol>])
		if(command == "minsat"){
			parseMinsat();
			skipToRpar();
			return CMD_TYPE::CT_MINSAT;
		}
		// (optimize (<symbol>+) [:id <symbol>] [:opt_kind <symbol>])
		if(command == "optimize"){
			parseOptimize();
			skipToRpar();
			return CMD_TYPE::CT_OPTIMIZE;
		}
		err_unkwn_sym(command, command_ln);

		return CMD_TYPE::CT_UNKNOWN;

	}

	// --- Incremental / sequential API ---

	Command Parser::nextCommand() {
		if (!bufptr || !*bufptr) return Command(CMD_TYPE::CT_EOF);
		parseLpar();
		CMD_TYPE type = parseCommand();
		Command cmd(type);
		cmd.line_number = line_number;
		parseRpar();
		if (command_logging_) {
			script_.addCommand(cmd);
		}
		return cmd;
	}

	bool Parser::push(size_t n) {
		context_.pushScope(n);
		if (command_logging_) {
			Command cmd(CMD_TYPE::CT_PUSH);
			cmd.push_pop_level = n;
			script_.addCommand(cmd);
		}
		return true;
	}

	bool Parser::pop(size_t n) {
		if (n > context_.scope_stack_.size()) {
			return false; // pop underflow
		}
		context_.popScope(n);
		if (command_logging_) {
			Command cmd(CMD_TYPE::CT_POP);
			cmd.push_pop_level = n;
			script_.addCommand(cmd);
		}
		return true;
	}

	bool Parser::resetAssertions() {
		context_.resetAssertions();
		if (command_logging_) {
			script_.addCommand(Command(CMD_TYPE::CT_RESET_ASSERTIONS));
		}
		return true;
	}

	bool Parser::reset() {
		context_.resetAll();
		// Reset SymbolManager and ObjectiveManager by creating new instances
		context_.setSymbolManager(std::make_shared<SymbolManager>());
		context_.setObjectiveManager(std::make_shared<ObjectiveManager>());
		// Re-initialize options if needed (keep existing options)
		if (command_logging_) {
			script_.addCommand(Command(CMD_TYPE::CT_RESET));
		}
		return true;
	}

	// sort ::= <identifier> | (<identifier> <sort>+)
	std::shared_ptr<Sort> Parser::parseSort(){
		if(*bufptr == ')'){
			// all ready to return
			return SortManager::NULL_SORT;
		}
		// cache basic sorts
		static const std::unordered_map<std::string, std::shared_ptr<Sort>> BASIC_SORTS = {
			{"Bool", SortManager::BOOL_SORT}, 
			{"Int", SortManager::INT_SORT}, 
			{"Real", SortManager::REAL_SORT}, 
			{"RoundingMode", SortManager::ROUNDING_MODE_SORT},
			{"String", SortManager::STR_SORT}, 
			{"Float16", SortManager::FLOAT16_SORT}, 
			{"Float32", SortManager::FLOAT32_SORT}, 
			{"Float64", SortManager::FLOAT64_SORT},
			{"RegLan", SortManager::REG_SORT},
			{"Reg", SortManager::REG_SORT},
			{"RegEx", SortManager::REG_SORT}
		};
		
		if (*bufptr != '(') {
			// <identifier>
			size_t expr_ln = line_number;
			std::string s = getSymbol();

			// first check the basic type cache
			auto basic_it = BASIC_SORTS.find(s);
			if (basic_it != BASIC_SORTS.end()) {
				return basic_it->second;
			}
			// then check the user-defined type
			else {
				std::shared_ptr<Sort> usort = getSymbolManager()->resolveSort(s);
				if(usort) return usort;
				err_unkwn_sym(s, expr_ln);
			}
		}
		// (<identifier> <sort>+)
		// (_ <identifier> <param>+)
		parseLpar();
		size_t expr_ln = line_number;
		std::string s = getSymbol();

		//parse identifier and get params
		std::shared_ptr<Sort> sort = SortManager::NULL_SORT;
		if (s == "Array") {
			// (Array S T)
			// S: sort of index
			// T: sort of value        
			std::shared_ptr<Sort> sortS = parseSort();
			std::shared_ptr<Sort> sortT = parseSort();
			std::string sort_key_name = "ARRAY_" + sortS->toString() + "_" + sortT->toString();
			sort = getSymbolManager()->resolveSort(sort_key_name);
			if(!sort){
				sort = getSortManager()->createArraySort(sortS, sortT);
				getSymbolManager()->registerSort(sort_key_name, sort);
			}
		}
		else if(s == "Datatype"){}
		else if(s == "Set"){}
		else if(s == "Relation"){}
		else if(s == "Bag"){}
		else if(s == "Sequence"){}
		else if(s == "RegEx"){
			// (RegEx <alphabet-sort>)
			std::shared_ptr<Sort> alphabet_sort = parseSort();
			if(!alphabet_sort->isStr()){
				err_all(ERROR_TYPE::ERR_TYPE_MIS, "RegEx sort expects String alphabet", expr_ln);
			}
			sort = SortManager::REG_SORT;
		}
		else if(s == "UF"){
			// // (UF S T)
			// // S: sort of parameters
			// // T: sort of return value
			// SortS = parseSort();
			// SortT = parseSort();
			// return std::make_shared<Sort>(SORT_KIND::SK_UF, "UF", 2, {sortS, sortT});
		}
		else if(s == "_"){
			// (_ <identifier> <param>+)
			std::string id = getSymbol();

			if(id == "BitVec"){
				// (_ BitVec n)
				// n: bit-width
				std::string n = getSymbol();
				std::string sort_key_name = "BV_" + n;
				sort = getSymbolManager()->resolveSort(sort_key_name);
				if(!sort){
					sort = getSortManager()->createBVSort(std::stoi(n));
					getSymbolManager()->registerSort(sort_key_name, sort);
				}
			}
			else if(id == "FloatingPoint"){
				// (_ FloatingPoint e s)
				// e: exponent width
				// s: significand width
				std::string e = getSymbol();
				std::string s = getSymbol();
				std::string sort_key_name = "FP_" + e + "_" + s;
				sort = getSymbolManager()->resolveSort(sort_key_name);
				if(!sort){
					sort = getSortManager()->createFPSort(std::stoi(e), std::stoi(s));
					getSymbolManager()->registerSort(sort_key_name, sort);
				}
			}
			else err_unkwn_sym(s, expr_ln);
		}
		else {
			// first check the basic type cache
			auto basic_it = BASIC_SORTS.find(s);
			if (basic_it != BASIC_SORTS.end()) {
				sort = basic_it->second;
			}
			// then check the user-defined type
			else {
				sort = getSymbolManager()->resolveSort(s);
				if(!sort) err_unkwn_sym(s, expr_ln);
			}

			if (sort->arity > 0 ){
				// sort may be a shared cached object from symbol manager or BASIC_SORTS.
				// Clone to avoid mutating the canonical sort.
				auto instantiated = std::make_shared<Sort>(*sort);
				instantiated->children.clear();
				for (size_t i = 0; i < sort->arity; i++){
					std::shared_ptr<Sort> sort_child = parseSort();
					instantiated->children.push_back(sort_child);
				}
				sort = instantiated;
			}
		}
		//err_unkwn_sym(s, expr_ln);
		parseRpar();

		return sort;
	}

	std::vector<std::shared_ptr<DAGNode>> Parser::parseParams() {

		std::vector<std::shared_ptr<DAGNode>> params;

		while (*bufptr != ')'){
			std::shared_ptr<DAGNode> expr = parseExpr();
			params.emplace_back(expr);
		}

		return params;

	}

	// struct for let context
	struct LetContext {
		std::vector<std::shared_ptr<DAGNode>> params;  // let bind vars for current level
		std::vector<std::string> key_list;
		std::shared_ptr<DAGNode> result;  // Store the result directly
		std::shared_ptr<DAGNode> bind_var_list;  // LET_BIND_VAR_LIST for current level
		int nesting_level;
		bool is_complete;
		
		LetContext(int level = 0) : result(nullptr), bind_var_list(nullptr), nesting_level(level), is_complete(false) {}
	};

	// parse let expression preserving the let-binding
	// (let (<keybinding>+) expr), return expr
	// In this function, the let-binding is preserved, and the let-binding is not expanded
	// So the bind_var cannot be the same in different let-binding
	// For example, (let ((x 1) (x 2)) x) is not allowed
	// Use let-chain to parse the let expression
	// let-chain: [LET_BIND_VAR_LIST, LET_BIND_VAR_LIST, ..., Body]
	// LET_BIND_VAR_LIST: [(<symbol> expr)]
	// Body: expr
	std::shared_ptr<DAGNode> Parser::parsePreservingLet(){
		// This function uses an iterative approach instead of recursion to handle nested let expressions
		// and constructs let-chain to avoid deep nesting issues
		
		// Create a stack to store parsing states and contexts
		std::vector<LetContext> stateStack;
		
		// Collect all bind_var_lists for final let-chain construction
		std::vector<std::shared_ptr<DAGNode>> all_bind_var_lists;
		
		// Push initial state onto the stack
		stateStack.emplace_back(LetContext(0));
		
		// Enter the initial "("
		parseLpar();
		
		std::string preserving_let_bind_var_suffix = PRESERVING_LET_BIND_VAR_SUFFIX + std::to_string(preserving_let_counter);
		
		// Main loop to handle all nested let expressions
		while (!stateStack.empty()) {
			auto &currentState = stateStack.back();
			auto &params = currentState.params;
			auto &key_list = currentState.key_list;
			
			if(!currentState.is_complete){
				// Parse the current let bindings
				while (*bufptr != ')') {
					// Process binding expression (<symbol> expr)
					parseLpar();
					
					size_t name_ln = line_number;
					std::string name = getSymbol();
					std::string prefixed_name = name + preserving_let_bind_var_suffix;
					
					// Check for duplicate key bindings
					if (getSymbolManager()->hasPreservingLet(prefixed_name)) {
						// Clean up all variable bindings in the state stack
						for (auto &state : stateStack) {
							getSymbolManager()->erasePreservingLetKeys(state.key_list);
						}
						err_sym_mis("Duplicate variable binding: " + name, name_ln);
					}
					
					// Parse the expression value (this won't trigger recursive let parsing)
					std::shared_ptr<DAGNode> expr = parseExpr();
					
					if (expr->isErr()) {
						// Clean up all variable bindings in the state stack
						for (auto &state : stateStack) {
							getSymbolManager()->erasePreservingLetKeys(state.key_list);
						}
						err_all(expr, name, name_ln);
					}
					
				// make let-binding variable
				std::shared_ptr<DAGNode> let_var = mkLetBindVar(prefixed_name, expr);
				// Add the binding inside the mkLetBindVar
				// Add to params in the correct order - bindings first, body later
				params.emplace_back(let_var);
				key_list.emplace_back(prefixed_name);
				
				parseRpar();
			}
			
			// Create LET_BIND_VAR_LIST for current level and add to collection
			currentState.bind_var_list = mkLetBindVarList(params);
			all_bind_var_lists.emplace_back(currentState.bind_var_list);
			
			// Finished parsing all bindings for the current let, handle the closing parenthesis
			parseRpar();
		}
		
		// Process the body of the let expression
			if (*bufptr == '(' && peekSymbol() == "let") {
				// If the body is another let expression, we don't recursively call parseLet
				// Instead, push it as a new state onto the stack
				parseLpar();  // Consume '('
				std::string let_key = getSymbol();  // Consume "let"
				condAssert(let_key == "let", "Invalid keyword for let");
				parseLpar();  // Consume the second let expression's starting '('
				
				stateStack.emplace_back(LetContext(currentState.nesting_level + 1));
			}
			else{
				if(*bufptr != ')'){
					// Parse the let body and store as result
					currentState.result = parseExpr();
				}
				
				// State processing complete, pop from stack
				auto completedState = currentState;
				stateStack.pop_back();
				
				// If stack is empty, construct final let-chain and return
				if (stateStack.empty()) {
					// Create let-chain with all collected bind_var_lists + final body
					std::shared_ptr<DAGNode> result = mkLetChain(all_bind_var_lists, completedState.result);
					return result;
				}
				else{
					// Consume the closing parenthesis if needed
					parseRpar();
					
					// Pass the result to parent level (don't create let-chain yet)
					stateStack.back().result = completedState.result;
					stateStack.back().is_complete = true;
				}
			}
		}
		
		// Should not reach here, but added for safety  
		return mkErr(ERROR_TYPE::ERR_UNEXP_EOF);
	}
	/*
	keybinding ::= (<symbol> expr)
	(let (<keybinding>+) expr), return expr
	*/
	std::shared_ptr<DAGNode> Parser::parseLet() {
		// This function uses an iterative approach instead of recursion to handle nested let expressions
		
		// Create a stack to store parsing states and contexts
		std::vector<LetContext> stateStack;
		
		// Push initial state onto the stack
		stateStack.emplace_back(LetContext(0));
		
		// Enter the initial "("
		parseLpar();
		
		// Main loop to handle all nested let expressions
		while (!stateStack.empty()) {

			auto &currentState = stateStack.back();
			auto &params = currentState.params;
			auto &key_list = currentState.key_list;
			
			if(!currentState.is_complete){
				// Parse the current let bindings
				while (*bufptr != ')') {
					// Process binding expression (<symbol> expr)
					parseLpar();
					
					size_t name_ln = line_number;
					std::string name = getSymbol();
					
					// Check for duplicate key bindings
					if (getSymbolManager()->hasLet(name)) {
						// Clean up all variable bindings in the state stack
						for (auto &state : stateStack) {
							getSymbolManager()->popLetScope(state.key_list);
						}
						err_sym_mis("Duplicate variable binding: " + name, name_ln);
					}
					
					// Parse the expression value (this won't trigger recursive let parsing)
					std::shared_ptr<DAGNode> expr = parseExpr();
					
					if (expr->isErr()) {
						// Clean up all variable bindings in the state stack
						for (auto &state : stateStack) {
							getSymbolManager()->popLetScope(state.key_list);
						}
						err_all(expr, name, name_ln);
					}
					
					// Add the binding
					getSymbolManager()->registerLet(name, expr);
					params.emplace_back(expr);
					key_list.emplace_back(name);
					
					parseRpar();
				}
				
				// Finished parsing all bindings for the current let, handle the closing parenthesis
				parseRpar();
			}
			
			// Body was already parsed by a nested let; we're at ')' closing this let
			if (currentState.is_complete && *bufptr == ')') {
				getSymbolManager()->popLetScope(key_list);
				std::shared_ptr<DAGNode> result = currentState.result;
				stateStack.pop_back();
				if (stateStack.empty()) {
					// Caller (expr_parser) will parseRpar() to consume this ')'
					return result;
				}
				parseRpar();
				stateStack.back().result = result;
				stateStack.back().is_complete = true;
				continue;
			}

			// Process the body of the let expression
			if (*bufptr == '(' && peekSymbol() == "let") {
				// If the body is another let expression, we don't recursively call parseLet
				// Instead, push it as a new state onto the stack
				parseLpar();  // Consume '('
				std::string let_key = getSymbol();  // Consume "let"
				condAssert(let_key == "let", "Invalid keyword for let");
				parseLpar();  // Consume the second let expression's starting '('
				
				stateStack.emplace_back(LetContext(currentState.nesting_level + 1));
			}
			else{
				std::shared_ptr<DAGNode> completedResult;
				if(*bufptr != ')'){
					currentState.result = parseExpr();
					completedResult = currentState.result;
				} else {
					// Empty let body is invalid
					currentState.result = mkErr(ERROR_TYPE::ERR_UNEXP_EOF);
					completedResult = currentState.result;
				}
				// Save result already in completedResult; pop_back invalidates currentState

				// Remove all variable bindings for the current state
				getSymbolManager()->popLetScope(key_list);

				// State processing complete, pop from stack
				stateStack.pop_back();

				// If stack is empty, return the result directly
				if (stateStack.empty()) {
					return completedResult;
				}
				else{
					// Consume the closing parenthesis
					parseRpar();
					// Store the result in the parent context
					stateStack.back().result = completedResult;
					stateStack.back().is_complete = true;
				}
			}
		}
		
		// Should not reach here, but added for safety
		return mkErr(ERROR_TYPE::ERR_UNEXP_EOF);
	}

	// Helper function to preview the next symbol without consuming input
	std::string Parser::peekSymbol() {
		char *save_bufptr = bufptr;
		SCAN_MODE save_mode = scan_mode;
		size_t save_line = line_number;
		
		std::string symbol;
		if (*bufptr == '(') {
			bufptr++;
			scanToNextSymbol();
			symbol = getSymbol();
		} else {
			symbol = getSymbol();
		}
		
		// Restore state
		bufptr = save_bufptr;
		scan_mode = save_mode;
		line_number = save_line;
		
		return symbol;
	}

	std::shared_ptr<DAGNode> Parser::applyFun(std::shared_ptr<DAGNode> fun, const std::vector<std::shared_ptr<DAGNode>> & params){
		// check the number of params
		if (fun->getFuncParamsSize() != params.size()){
			return mkErr(ERROR_TYPE::ERR_PARAM_MIS);
		}

		// For declare-fun (uninterpreted functions), create a function application node
		if(fun->getFuncBody()->isNull()){
			// Determine if this is a datatype constructor/selector/tester
			NODE_KIND nk = getDtFunctionKind(fun->getSort(), fun->getName(), params);
			std::shared_ptr<DAGNode> result = getNodeManager()->createNode(fun->getSort(), nk, fun->getName(), params);
			return result;
		}

	// For recursive functions (define-fun-rec), behavior depends on expand_recursive_functions option
	// If expand_recursive_functions is true, expand it like define-fun
	// If false (default), create a recursive function application node to avoid infinite recursion
	if(fun->isFuncRec()){
		if(!getOptions()->getExpandRecursiveFunctions()){
			// Don't expand recursive functions (default behavior)
			return mkApplyRecFunc(fun, params);
		}
		// Otherwise, fall through to expand it like define-fun
	}
	else if(fun->isFuncDec()){
		// a only declared function, i.e., uninterpreted function or datatype function
		return mkApplyUF(fun->getSort(), fun->getName(), params);
	}

	// For regular functions (define-fun), check expand_functions option
	if(fun->isFuncDef()){
		if(!getOptions()->getExpandFunctions()){
			// Don't expand functions, create a function application node
			return mkApplyFunc(fun, params);
		}
		// Otherwise, fall through to expand the function
	}

	if(fun->getFuncBody()->isErr()){
		return fun->getFuncBody();
	}
	
	// Expand the function: replace parameters with actual arguments
	// variable map for local variables
	std::unordered_map<std::string, std::shared_ptr<DAGNode>> new_params_map;
	std::vector<std::shared_ptr<DAGNode>> func_params = fun->getFuncParams();
	for (size_t i = 0; i < func_params.size(); i++) {
		if(params[i]->isErr()){
			return params[i];
		}
		new_params_map.insert(std::pair<std::string, std::shared_ptr<DAGNode>>(func_params[i]->getName(), params[i]));
	}
	
	// function content
	std::shared_ptr<DAGNode> formula = fun->getFuncBody();

	// Iterative implementation to replace recursive applyFunPostOrder
	return applyFunPostOrder(formula, new_params_map);
	}

	// Iterative version of post-order traversal function application
	std::shared_ptr<DAGNode> Parser::applyFunPostOrder(std::shared_ptr<DAGNode> node, std::unordered_map<std::string, std::shared_ptr<DAGNode>> & params){
		if (!node) return nullptr;
		
		// Stack to track nodes to process
		std::stack<std::pair<std::shared_ptr<DAGNode>, bool>> todo;
		
		// Map to store processed results for each node
		std::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> results;
		
		// Push initial node to stack
		todo.push(std::make_pair(node, false));
		
		while (!todo.empty()) {
			std::shared_ptr<DAGNode> current = todo.top().first;
			bool processed = todo.top().second;
			todo.pop();
			
			if (processed) {
				// Node is being revisited after processing its children
				std::vector<std::shared_ptr<DAGNode>> childResults;
				
				// Collect results from all children
				for (size_t i = 0; i < current->getChildrenSize(); i++) {
					childResults.emplace_back(results[current->getChild(i)]);
				}
					
				// Create a new node with processed children
				std::shared_ptr<DAGNode> result;
				if (current->isUFApplication() || current->isConstructorApp() || current->isSelectorApp() || current->isTesterApp()) {
					// NT_UF_APPLY or NT_DT_*: Must preserve function name when recreating node
					result = mkApplyUF(current->getSort(), current->getName(), childResults);
				} else if (current->isFuncRecApplication() && !getOptions()->getExpandRecursiveFunctions()) {
					// NT_FUNC_REC_APPLY: Recursive function call when not expanding
					// Must preserve function name when recreating node
					result = mkApplyRecFunc(current, childResults);
				} else if (current->isFuncApplication() || (current->isFuncRecApplication() && getOptions()->getExpandRecursiveFunctions())) {
					// NT_FUNC_APPLY or NT_FUNC_REC_APPLY (when expanding)
					// Parameters have been processed, now expand the function
					std::vector<std::shared_ptr<DAGNode>> funcParams;
					for (size_t i = 1; i < childResults.size(); i++) {
						funcParams.emplace_back(childResults[i]);
					}
					result = applyFun(current->getFuncBody(), funcParams);
				} else {
					// For all other cases: regular operators
					result = mkOper(current->getSort(), current->getKind(), childResults);
				}
				results[current] = result;
			} else {
				// First visit to this node
				if (current->isFuncParam()) {
					// Function parameter - replace with actual parameter
					auto it = params.find(current->getName());
					if (it != params.end()) {
						results[current] = it->second;
					} else {
						// Parameter not found, this should not happen if applyFun is called correctly
						results[current] = mkErr(ERROR_TYPE::ERR_FUN_LOCAL_VAR);
					}
				} else if (current->isConst()) {
					// Constants remain unchanged
					results[current] = current;
				} else if (current->isVar()) {
					// Variables (NT_VAR, NT_TEMP_VAR, etc.) are leaves, keep unchanged
					results[current] = current;
				} else {
					// All other cases: operators, function applications, UF applications, etc.
					// Mark the node for revisit after processing children
					todo.push(std::make_pair(current, true));
					
					// For function applications that will be expanded, skip the first child (function definition itself)
					// For all other nodes, process all children
					bool isFuncAppToExpand = current->isFuncApplication() || 
					                        (current->isFuncRecApplication() && getOptions()->getExpandRecursiveFunctions());
					int startIdx = isFuncAppToExpand ? 1 : 0;
					
					// Push all children onto the stack in reverse order
					for (int i = current->getChildrenSize() - 1; i >= startIdx; i--) {
						todo.push(std::make_pair(current->getChild(i), false));
					}
				}
			}
		}
		
		return results[node];
	}
	
	std::shared_ptr<DAGNode> Parser::mkApplyFunc(std::shared_ptr<DAGNode> fun, const std::vector<std::shared_ptr<DAGNode>> &params){
		// Intern function applications through NodeManager to avoid creating大量重复 NT_FUNC_APPLY 节点
		// when expand_functions=false and the same function call appears many times.
		std::vector<std::shared_ptr<DAGNode>> children;
		children.reserve(params.size() + 1);
		children.emplace_back(fun);
		for(const auto& p : params) children.emplace_back(p);
		auto res = getNodeManager()->createNode(fun->getSort(), NODE_KIND::NT_FUNC_APPLY, fun->getName(), std::move(children));
		getSymbolManager()->addStaticFunction(res);
		return res;
	}

	
    std::shared_ptr<DAGNode> Parser::mkApplyRecFunc(std::shared_ptr<DAGNode> fun, const std::vector<std::shared_ptr<DAGNode>> &params){
        // Intern recursive function applications through NodeManager as well.
		std::vector<std::shared_ptr<DAGNode>> children;
		children.reserve(params.size() + 1);
		children.emplace_back(fun);
		for(const auto& p : params) children.emplace_back(p);
		auto res = getNodeManager()->createNode(fun->getSort(), NODE_KIND::NT_FUNC_REC_APPLY, fun->getName(), std::move(children));
		getSymbolManager()->addStaticFunction(res);
		return res;
    }

    NODE_KIND Parser::getDtFunctionKind(const std::shared_ptr<Sort>& return_sort, const std::string &name, const std::vector<std::shared_ptr<DAGNode>> &params) {
        // Constructor: return sort is DT and has a constructor with this name
        if(return_sort && return_sort->isDatatype() && return_sort->hasDtConstructor(name)) {
            return NODE_KIND::NT_DT_CONSTRUCTOR;
        }
        // Selector: first param sort is DT and has a selector with this name
        if(!params.empty()) {
            auto first_sort = params[0]->getSort();
            if(first_sort && first_sort->isDatatype()) {
                if(first_sort->hasDtSelector(name)) {
                    return NODE_KIND::NT_DT_SELECTOR;
                }
                if(first_sort->hasDtTester(name)) {
                    return NODE_KIND::NT_DT_TESTER;
                }
            }
        }
        return NODE_KIND::NT_UF_APPLY;
    }

    std::shared_ptr<DAGNode> Parser::mkApplyUF(const std::shared_ptr<Sort>& sort, const std::string &name, const std::vector<std::shared_ptr<DAGNode>> &params){
        NODE_KIND nk = getDtFunctionKind(sort, name, params);
        return getNodeManager()->createNode(sort, nk, name, params);
    }

    void Parser::defineDatatypeConstructors(const std::shared_ptr<Sort>& dt_sort) {
        std::vector<Sort::DtConstructor> constructors;
        while (*bufptr != ')') {
            parseLpar();
            std::string ctor_name = getSymbol();
            Sort::DtConstructor ctor;
            ctor.name = ctor_name;
            while (*bufptr != ')') {
                parseLpar();
                std::string sel_name = getSymbol();
                std::shared_ptr<Sort> sel_sort = parseSort();
                parseRpar();
                ctor.selectors.push_back({sel_name, sel_sort});
            }
            parseRpar();
            constructors.push_back(ctor);
        }
        parseRpar();

        dt_sort->dt_constructors = std::make_shared<std::vector<Sort::DtConstructor>>(constructors);

        for (auto& ctor : constructors) {
            std::vector<std::shared_ptr<Sort>> sel_sorts;
            for (auto& sel : ctor.selectors)
                sel_sorts.push_back(sel.sort);
            auto ctor_node = mkFuncDec(ctor.name, sel_sorts, dt_sort);
            if (!ctor_node->isErr()) {
                getSymbolManager()->addFunctionName(ctor.name);
            }
            for (auto& sel : ctor.selectors) {
                std::vector<std::shared_ptr<Sort>> sel_params = {dt_sort};
                auto sel_node = mkFuncDec(sel.name, sel_params, sel.sort);
                if (!sel_node->isErr()) {
                    getSymbolManager()->addFunctionName(sel.name);
                }
            }
            std::string tester_name = "is-" + ctor.name;
            std::vector<std::shared_ptr<Sort>> tester_params = {dt_sort};
            auto tester_node = mkFuncDec(tester_name, tester_params, SortManager::BOOL_SORT);
            if (!tester_node->isErr()) {
                getSymbolManager()->addFunctionName(tester_name);
            }
        }
    }

    // MATCH EXPRESSION
    // (match <term> ((<pattern> <body>) ... ))
    std::shared_ptr<DAGNode> Parser::parseMatch(){
        // Parse the scrutinee term
        std::shared_ptr<DAGNode> scrutinee = parseExpr();

        // Collect match cases: alternating (pattern, body) pairs
        std::vector<std::shared_ptr<DAGNode>> children;
        children.push_back(scrutinee);

        auto dt_sort = scrutinee->getSort();

        scanToNextSymbol();
        while(*bufptr != ')'){
            parseLpar(); // open a case: (<pattern> <body>)

            std::vector<std::string> bound_vars;
            std::shared_ptr<DAGNode> pattern;

            scanToNextSymbol();
            if(*bufptr == '('){
                // Constructor pattern: (<ctor> <var1> ... <varN>)
                parseLpar();
                std::string ctor_name = getSymbol();

                // Look up the constructor in the DT sort to get selector sorts
                std::vector<std::shared_ptr<DAGNode>> pat_vars;
                if(dt_sort && dt_sort->isDatatype() && dt_sort->hasDtConstructor(ctor_name)){
                    const auto* ctor = dt_sort->getDtConstructorByName(ctor_name);
                    if(ctor){
                        for(size_t i = 0; i < ctor->selectors.size(); i++){
                            std::string var_name = getSymbol();
                            auto var = getNodeManager()->createNode(ctor->selectors[i].sort, NODE_KIND::NT_QUANT_VAR, var_name);
                            getSymbolManager()->registerQuantVar(var_name, var);
                            bound_vars.push_back(var_name);
                            pat_vars.push_back(var);
                        }
                    }
                }
                parseRpar(); // close the pattern parens

                pattern = getNodeManager()->createNode(dt_sort, NODE_KIND::NT_DT_CONSTRUCTOR, ctor_name, pat_vars);
            } else {
                // Either a nullary constructor name or a catch-all variable
                std::string sym = getSymbol();
                if(dt_sort && dt_sort->isDatatype() && dt_sort->hasDtConstructor(sym)){
                    // Nullary constructor pattern
                    pattern = getNodeManager()->createNode(dt_sort, NODE_KIND::NT_DT_CONSTRUCTOR, sym, {});
                } else {
                    // Catch-all variable pattern — bind the variable to a fresh var node
                    auto var = getNodeManager()->createNode(dt_sort, NODE_KIND::NT_QUANT_VAR, sym);
                    getSymbolManager()->registerQuantVar(sym, var);
                    bound_vars.push_back(sym);
                    pattern = var;
                }
            }

            // Parse the body with quantifier scope active for bound pattern vars
            bool saved_in_quant = in_quantifier_scope;
            in_quantifier_scope = true;
            std::shared_ptr<DAGNode> body = parseExpr();
            in_quantifier_scope = saved_in_quant;

            // Pop bound variables
            getSymbolManager()->popQuantScope(bound_vars);

            children.push_back(pattern);
            children.push_back(body);

            parseRpar(); // close the case

            scanToNextSymbol();
        }
        // The caller (expr_parser) will parseRpar() for the closing ) of (match ...)

        // Determine result sort from the first case body
        auto result_sort = (children.size() >= 3) ? children[2]->getSort() : SortManager::BOOL_SORT;
        return getNodeManager()->createNode(result_sort, NODE_KIND::NT_DT_MATCH, "match", children);
    }


	// QUANTIFIERS
	// (quantifier ((<identifier> <sort>)+） <expr>)
	std::shared_ptr<DAGNode> Parser::mkQuantVar(const std::string& name, std::shared_ptr<Sort> sort){
		std::shared_ptr<DAGNode> var = getSymbolManager()->getQuantVar(name);
		if(var) return var;
		var = getNodeManager()->createNode(sort, NODE_KIND::NT_QUANT_VAR, name);
		getSymbolManager()->registerQuantVar(name, var);
		return var;
	}
	std::shared_ptr<DAGNode> Parser::parseQuant(const std::string& type){
		// (quantifier ((<identifier> <sort>)+） <expr>)
		//             ^
		parseLpar();
		std::vector<std::shared_ptr<DAGNode>> params;
		std::vector<std::string> quant_var_names;
		while (*bufptr != ')') {
			// (quantifier ((<identifier> <sort>)+） <expr>)
			//              ^
			parseLpar();
			std::string var_name = getSymbol();
			std::shared_ptr<Sort> var_sort = parseSort();
			std::shared_ptr<DAGNode> var = mkQuantVar(var_name, var_sort);
			params.emplace_back(var);
			quant_var_names.emplace_back(var_name);
			parseRpar();
		}
		// (quantifier ((<identifier> <sort>)+） <expr>)
		//                                    ^
		parseRpar();
		std::shared_ptr<DAGNode> body = parseExpr();
		params.insert(params.begin(), body);
		std::shared_ptr<DAGNode> res = NodeManager::NULL_NODE;
		if (type == "forall") {
			res = mkForall(params);
		}
		else if (type == "exists") {
			res = mkExists(params);
		}
		else{
			condAssert(false, "Invalid quantifier");
		}
		getSymbolManager()->popQuantScope(quant_var_names);
		return res;
	}

	std::shared_ptr<DAGNode> Parser::mkForall(const std::vector<std::shared_ptr<DAGNode>> &params){
		return mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_FORALL, params);
	}
	std::shared_ptr<DAGNode> Parser::mkExists(const std::vector<std::shared_ptr<DAGNode>> &params){
		return mkOper(SortManager::BOOL_SORT, NODE_KIND::NT_EXISTS, params);
	}

	
	std::shared_ptr<DAGNode> Parser::substitute(std::shared_ptr<DAGNode> expr, std::unordered_map<std::string, std::shared_ptr<DAGNode>> &params){
		std::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> visited;
		return substitute(expr, params, visited);
	}
	// visited is used to avoid infinite loop
	std::shared_ptr<DAGNode> Parser::substitute(std::shared_ptr<DAGNode> expr, std::unordered_map<std::string, std::shared_ptr<DAGNode>> &params, std::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> & visited){
		/*
			Convert the previously recursive implementation into an iterative, stack-based
			post-order traversal to avoid potential stack-overflow on very deep/large DAGs.
			The algorithm mirrors the logic of applyFunPostOrder used elsewhere in this file.
		*/

		// Quick hit: if we already substituted this node, return the cached result.
		if(visited.find(expr) != visited.end()){
			return visited[expr];
		}

		// (node, processed?)  processed==false  => first time we see the node
		//                      processed==true   => all children have been handled
		std::stack<std::pair<std::shared_ptr<DAGNode>, bool>> todo;
		todo.push(std::make_pair(expr, false));

		while(!todo.empty()){
			auto curPair   = todo.top();
			todo.pop();
			std::shared_ptr<DAGNode> current = curPair.first;
			bool processed                  = curPair.second;

			// If we already computed a substitute for this node elsewhere, skip.
			if(visited.find(current) != visited.end()){
				continue;
			}

		if(processed){
			/*
				All children have been processed – build the new node using the
				(possibly substituted) child results that are now stored in
				`visited`.
			*/
			std::vector<std::shared_ptr<DAGNode>> newChildren;
			newChildren.reserve(current->getChildrenSize());
			for(size_t i = 0; i < current->getChildrenSize(); ++i){
				newChildren.emplace_back(visited[current->getChild(i)]);
			}
			std::shared_ptr<DAGNode> newNode;
			// For nodes with meaningful names (UF applications, function applications, etc.),
			// preserve the original name instead of using kindToString
			if(current->isUFApplication() || 
			   current->isFuncApplication() || 
			   current->isFuncRecApplication() ||
			   current->isArray()) {
				// Create node with original name preserved
				newNode = getNodeManager()->createNode(current->getSort(), current->getKind(), current->getName(), newChildren);
			} else {
				// Use standard mkOper for other node types
				newNode = mkOper(current->getSort(), current->getKind(), newChildren);
			}
			visited[current] = newNode;
		}
			else{
				/* First visit */
				if(current->isVar()){
					// Variable: replace if it appears in the substitution map
					auto it = params.find(current->getName());
					visited[current] = (it != params.end()) ? it->second : current;
				}
				else if(current->isConst() || current->isFuncParam()){
					// Constants and function-parameters stay unchanged
					visited[current] = current;
				}
				else{
					// Non-leaf operator node – schedule a second visit after children
					todo.push(std::make_pair(current, true));
					// Push children (reverse order keeps original left-to-right after pop)
					for(int i = static_cast<int>(current->getChildrenSize()) - 1; i >= 0; --i){
						auto child = current->getChild(i);
						if(visited.find(child) == visited.end()){
							todo.push(std::make_pair(child, false));
						}
					}
				}
			}
		}

		return visited[expr];
	}

	std::shared_ptr<DAGNode> Parser::arithNormalize(std::shared_ptr<DAGNode> expr){
		bool is_changed = false;
		return arithNormalize(expr, is_changed);
	}


	std::shared_ptr<DAGNode> Parser::arithNormalize(std::shared_ptr<DAGNode> expr, bool& is_changed){
		// Iterative implementation to avoid stack overflow on very deep/large DAGs
		if(expr->isErr()){
			is_changed = false;
			return expr;
		}

		// Expand outer let expression (expandLet itself can still be recursive, but the overall depth is usually limited)
		if(expr->isLet()){
			expr = expandLet(expr);
		}

		// Use manual post-order traversal stack
		std::stack<std::pair<std::shared_ptr<DAGNode>, bool>> todo;
		std::unordered_map<std::shared_ptr<DAGNode>, std::shared_ptr<DAGNode>> result_map;   // Record normalized nodes
		std::unordered_map<std::shared_ptr<DAGNode>, bool>                     changed_map;  // Record if the node has changed

		todo.push({expr, false});

		while(!todo.empty()){
			auto [node, processed] = todo.top();
			todo.pop();

			// If already processed, skip
			if(result_map.find(node) != result_map.end()) continue;

		if(processed){
			/* Second visit: all children have been processed, can construct the current node */
			if(node->isConst() || node->isVar() || node->isArithTerm() || node->isErr()){
				result_map[node]  = node;
				changed_map[node] = false;
				continue;
			}

			if(node->isArithComp()){
				condAssert(node->getChildrenSize()==2, "ArithComp should have two children");
				std::shared_ptr<DAGNode> leftN  = result_map[node->getChild(0)];
				std::shared_ptr<DAGNode> rightN = result_map[node->getChild(1)];
				bool child_changed = changed_map[node->getChild(0)] || changed_map[node->getChild(1)];

				bool cur_changed = child_changed;
				std::shared_ptr<DAGNode> new_node = node; // default to keep unchanged

				if(!rightN->isConst()){
					cur_changed = true;
					auto left_sub = mkOper(leftN->getSort(), NODE_KIND::NT_SUB, {leftN, rightN});
					auto zero     = getZero(leftN->getSort());
					new_node      = mkOper(SortManager::BOOL_SORT, node->getKind(), {left_sub, zero});
				}else if(child_changed){
					new_node = mkOper(node->getSort(), node->getKind(), {leftN, rightN});
				}

				result_map[node]  = new_node;
				changed_map[node] = cur_changed;
				continue;
			}

			/* General operator */
			std::vector<std::shared_ptr<DAGNode>> new_children;
			bool any_changed = false;
			new_children.reserve(node->getChildrenSize());
			for(size_t i=0;i<node->getChildrenSize();++i){
				auto child = node->getChild(i);
				new_children.emplace_back(result_map[child]);
				any_changed = any_changed || changed_map[child];
			}

			std::shared_ptr<DAGNode> new_node;
			if(any_changed){
				// For nodes with meaningful names (UF applications, function applications, etc.),
				// preserve the original name instead of using kindToString
				if(	node->isUFApplication() || 
					node->isFuncApplication() || 
					node->isFuncRecApplication() ||
					node->isArray()) {
					// Create node with original name preserved
					new_node = getNodeManager()->createNode(node->getSort(), node->getKind(), node->getName(), new_children);
				} else {
					// Use standard mkOper for other node types
					new_node = mkOper(node->getSort(), node->getKind(), new_children);
				}
			} else {
				new_node = node;
			}

			// Apply array simplification if the node is an array operation
			if(new_node->isSelect() || new_node->isStore()){
				std::shared_ptr<DAGNode> simplified = simplifyArray(new_node);
				if(simplified != new_node){
					new_node = simplified;
					any_changed = true;
				}
			}

			// Apply simplification for equality operations (including array equality)
			if(new_node->isEq()){
				std::shared_ptr<DAGNode> simplified;
				if(new_node->getChildrenSize() == 2){
					simplified = simp_oper(new_node->getKind(), new_node->getChild(0), new_node->getChild(1));
				}
				else if(new_node->getChildrenSize() > 2){
					simplified = simp_oper(new_node->getKind(), new_children);
				}
				else{
					simplified = NodeManager::UNKNOWN_NODE;
				}
				if(simplified && !simplified->isUnknown()){
					new_node = simplified;
					any_changed = true;
				}
			}

			result_map[node]  = new_node;
			changed_map[node] = any_changed;
			}else{
				/* First visit */
				// Leaf node directly processed
				if(node->isConst() || node->isVar() || node->isArithTerm() || node->isErr()){
					result_map[node]  = node;
					changed_map[node] = false;
					continue;
				}

				// If it is still let, expand and push back to stack for processing
				if(node->isLet()){
					node = expandLet(node);
					todo.push({node, false});
					continue;
				}

				// Push back self (mark as visited), then push children
				todo.push({node, true});
				for(int i = static_cast<int>(node->getChildrenSize()) - 1; i >= 0; --i){
					todo.push({node->getChild(i), false});
				}
			}
		}

		is_changed = changed_map[expr];
		return result_map[expr];
	}

	std::vector<std::shared_ptr<DAGNode>> Parser::arithNormalize(std::vector<std::shared_ptr<DAGNode>> exprs){
		std::vector<std::shared_ptr<DAGNode>> res;
		for(auto& expr : exprs){
			res.emplace_back(arithNormalize(expr));
		}
		return res;
	}


	// aux functions
	NODE_KIND Parser::getAddOp(std::shared_ptr<Sort> sort){
		if(sort->isInt() || sort->isReal() || sort->isIntOrReal()){
			return NODE_KIND::NT_ADD;
		}
		else if(sort->isBv()){
			return NODE_KIND::NT_BV_ADD;
		}
		else if(sort->isFp()){
			return NODE_KIND::NT_FP_ADD;
		}
		else{
			return NODE_KIND::NT_ERROR;
		}
	}
	NODE_KIND Parser::getNegatedKind(NODE_KIND kind){
		return SOMTParser::getNegatedKind(kind);
	}
	std::shared_ptr<DAGNode> Parser::getZero(std::shared_ptr<Sort> sort){
		if(sort->isInt() || sort->isIntOrReal()){
			return mkConstInt(0);
		}
		else if(sort->isReal()){
			return mkConstReal(0.0);
		}
		else if(sort->isBv()){
			return mkConstBv("0", sort->getBitWidth());
		}
		else if(sort->isFp()){
			return mkConstFp("0.0", sort->getExponentWidth(), sort->getSignificandWidth());
		}
		else if(sort->isStr()){
			return mkConstStr("");
		}
		else{
			return mkErr(ERROR_TYPE::ERR_UNKWN_SYM);
		}
	}

	// Helper function to parse symbol name that may start with invalid characters
	// Returns the symbol name, wrapping with |...| if it starts with invalid characters
	std::string Parser::parseModelSymbolName() {
		if (*bufptr == '(') {
			// Check if this is an empty symbol (next is parameter list) or a symbol starting with '('
			// Skip whitespace after '('
			char* lookahead = bufptr + 1;
			while (*lookahead && (*lookahead == ' ' || *lookahead == '\t' || *lookahead == '\n' || *lookahead == '\r' || *lookahead == '\v' || *lookahead == '\f')) {
				lookahead++;
			}
			// If next is ')' or '(', it's likely a parameter list, so empty symbol
			if (*lookahead == ')' || *lookahead == '(') {
				return "||";
			} else {
				// Symbol starts with '(', need to parse it and wrap with |...|
				// Parse the symbol manually: read all non-whitespace characters until we hit whitespace
				// that is followed by '(' or ')' (parameter list start)
				char* name_start = bufptr;
				while (*bufptr && *bufptr != 0) {
					// Check if current position is whitespace
					if (*bufptr == ' ' || *bufptr == '\t' || *bufptr == '\n' || *bufptr == '\r' || *bufptr == '\v' || *bufptr == '\f') {
						// Found whitespace, check what comes after
						char* after_ws = bufptr + 1;
						while (*after_ws && (*after_ws == ' ' || *after_ws == '\t' || *after_ws == '\n' || *after_ws == '\r' || *after_ws == '\v' || *after_ws == '\f')) {
							after_ws++;
						}
						// If next non-whitespace is '(' or ')', this is the end of symbol
						if (*after_ws == '(' || *after_ws == ')') {
							break;
						}
					}
					bufptr++;
				}
				std::string raw_name(name_start, bufptr - name_start);
				scanToNextSymbol();
				return "|" + raw_name + "|";
			}
		} else {
			std::string name = getSymbol();
			// Check if symbol starts with invalid character (digit, ')', etc.)
			if (!name.empty()) {
				char first_char = name[0];
				if ((first_char >= '0' && first_char <= '9') || first_char == ')' || first_char == ';') {
					// Invalid first character, wrap with |...|
					return "|" + name + "|";
				}
			}
			return name;
		}
		return std::string(); // unreachable — all if/else branches above return
	}

	// parse model
	ModelPtr Parser::parseModel(const std::string& model, bool only_declared){
		std::shared_ptr<Model> model_ptr = std::make_shared<Model>();
		
		// Save original parser state
		char* original_buffer = buffer;
		char* original_bufptr = bufptr;
		size_t original_buflen = buflen;
		bool original_parsing_file = parsing_file;
		
		// Set temporary parsing state
		parsing_file = false;
		size_t original_line_number = line_number;
		line_number = 1;
		buffer = safe_strdup(model);
		if (!buffer) {
			line_number = original_line_number;
			return model_ptr;
		}
		buflen = model.length();
		bufptr = buffer;
		scanToNextSymbol();
		
		try {
			// Check if wrapped with (model ...) or just (...)
			char* start_pos = bufptr;
			if (*bufptr == '(') {
				char* lookahead = bufptr + 1;
				while (*lookahead && (*lookahead == ' ' || *lookahead == '\t' || *lookahead == '\n' || *lookahead == '\r')) {
					lookahead++;
				}
				
				// Check if next symbol is "model"
				if (strncmp(lookahead, "model", 5) == 0 && 
				    (lookahead[5] == ' ' || lookahead[5] == '\t' || lookahead[5] == '\n' || lookahead[5] == '\r' || lookahead[5] == '(')) {
					// Skip outer (model ...)
					parseLpar();
					std::string keyword = getSymbol();
					// Now we're inside (model ...) or just (...), continue parsing define-fun
				} else {
					// Just a plain ( wrapping the define-funs, skip it
					parseLpar();
				}
			}
			start_pos = bufptr;
			
			// FIRST PASS: Declare all functions (including parameterized ones)
			// This allows forward references in expressions
			while (*bufptr && *bufptr != 0) {
				// Skip whitespace
				while (*bufptr && (*bufptr == ' ' || *bufptr == '\t' || *bufptr == '\n' || *bufptr == '\r')) {
					bufptr++;
					if (*bufptr == '\n') line_number++;
				}
				
				if (*bufptr == 0 || *bufptr == ')') break;
				
				if (*bufptr != '(') break;
				
				// Parse one define-fun
				parseLpar();
				std::string keyword = getSymbol();
				
				if (keyword != "define-fun") {
					// Not a define-fun, skip it
					skipToRpar();
					parseRpar();
					continue;
				}
				
				// Parse function name (may start with invalid characters like '(')
				std::string func_name = parseModelSymbolName();
				
				// Parse parameter list
				parseLpar();
				std::vector<std::shared_ptr<Sort>> param_sorts;
				while (*bufptr != ')') {
					parseLpar();
					getSymbol(); // parameter name (ignored)
					std::shared_ptr<Sort> param_sort = parseSort();
					param_sorts.push_back(param_sort);
					parseRpar();
				}
				parseRpar();
				
				// Parse return type
				std::shared_ptr<Sort> return_sort = parseSort();
				
				// Declare the function so it can be referenced
				if (!param_sorts.empty()) {
					// Function with parameters - declare it
					mkFuncDec(func_name, param_sorts, return_sort);
					if (!getSymbolManager()->getFun(func_name) || !getSymbolManager()->hasFunctionName(func_name)) {
						getSymbolManager()->addFunctionName(func_name);
					}
				}
				
				// Skip the function body for now
				skipToRpar();
				parseRpar();
			}
			
			// SECOND PASS: Parse variable values (only for 0-parameter functions)
			// Reset to start position
			bufptr = start_pos;
			line_number = 1;
			
			while (*bufptr && *bufptr != 0) {
				// Skip whitespace
				while (*bufptr && (*bufptr == ' ' || *bufptr == '\t' || *bufptr == '\n' || *bufptr == '\r')) {
					bufptr++;
					if (*bufptr == '\n') line_number++;
				}
				
				if (*bufptr == 0 || *bufptr == ')') break;
				
				if (*bufptr != '(') break;
				
				// Parse one define-fun
				parseLpar();
				std::string keyword = getSymbol();
				
				if (keyword != "define-fun") {
					// Not a define-fun, skip it
					skipToRpar();
					parseRpar();
					continue;
				}
				
				// Parse variable name (can be any symbol including | | or ||)
				// Note: In SMT-LIB, | | means a symbol containing a space, || means an empty symbol
				// May start with invalid characters like '('
				std::string var_name = parseModelSymbolName();
				
				// Parse parameter list
				parseLpar();
				bool has_params = (*bufptr != ')');
				
				// Skip parameters
				while (*bufptr != ')') {
					parseLpar();
					getSymbol(); // parameter name
					parseSort(); // parameter type
					parseRpar();
				}
				parseRpar();
				
				// If has parameters, skip this function definition in second pass
				if (has_params) {
					skipToRpar();
					parseRpar();
					continue;
				}
				
				// Parse type
				std::shared_ptr<Sort> var_sort = parseSort();
				if (!var_sort) {
					skipToRpar();
					parseRpar();
					continue;
				}
				
				// Check if only declared variables
				if (only_declared && !isDeclaredVariable(var_name)) {
					skipToRpar();
					parseRpar();
					continue;
				}
				
				// Define variable first
				std::shared_ptr<DAGNode> var_node = mkVar(var_sort, var_name);
				
				// Parse value expression
				std::shared_ptr<DAGNode> value = parseExpr();
				
				if (value && !value->isErr()) {
					model_ptr->add(var_node, value);
				}
				
				// Close the define-fun
				parseRpar();
			}
			
		} catch (...) {
			// Ignore errors and return what we have
		}
		
		// Restore original state
		delete[] buffer;
		buffer = original_buffer;
		bufptr = original_bufptr;
		buflen = original_buflen;
		parsing_file = original_parsing_file;
		
		return model_ptr;
	}

	ModelPtr Parser::newEmptyModel(){
		// new empty model
		std::shared_ptr<Model> model = std::make_shared<Model>();
		// get all variables
		std::vector<std::shared_ptr<DAGNode>> vars = getDeclaredVariables();
		for(auto& var : vars){
			model->addVar(var);
		}
		return model;
	}

	bool Parser::isDeclaredVariable(const std::string& var_name) const{
		return getSymbolManager()->hasVar(var_name);
	}
	bool Parser::isDeclaredFunction(const std::string& func_name) const{
		return getSymbolManager()->getFun(func_name) != nullptr;
	}


	// error operations: store ERROR_TYPE in error node's name for correct err_all(DAGNode) dispatch
	static std::string errorTypeToName(ERROR_TYPE t) {
		switch (t) {
			case ERROR_TYPE::ERR_UNEXP_EOF: return "ERR_UNEXP_EOF";
			case ERROR_TYPE::ERR_SYM_MIS: return "ERR_SYM_MIS";
			case ERROR_TYPE::ERR_UNKWN_SYM: return "ERR_UNKWN_SYM";
			case ERROR_TYPE::ERR_PARAM_MIS: return "ERR_PARAM_MIS";
			case ERROR_TYPE::ERR_PARAM_NBOOL: return "ERR_PARAM_NBOOL";
			case ERROR_TYPE::ERR_PARAM_NNUM: return "ERR_PARAM_NNUM";
			case ERROR_TYPE::ERR_PARAM_NSAME: return "ERR_PARAM_NSAME";
			case ERROR_TYPE::ERR_LOGIC: return "ERR_LOGIC";
			case ERROR_TYPE::ERR_MUL_DECL: return "ERR_MUL_DECL";
			case ERROR_TYPE::ERR_MUL_DEF: return "ERR_MUL_DEF";
			case ERROR_TYPE::ERR_ZERO_DIVISOR: return "ERR_ZERO_DIVISOR";
			case ERROR_TYPE::ERR_FUN_LOCAL_VAR: return "ERR_FUN_LOCAL_VAR";
			case ERROR_TYPE::ERR_ARI_MIS: return "ERR_ARI_MIS";
			case ERROR_TYPE::ERR_TYPE_MIS: return "ERR_TYPE_MIS";
			case ERROR_TYPE::ERR_NEG_PARAM: return "ERR_NEG_PARAM";
		}
		return "ERR_UNKWN_SYM";
	}
	static ERROR_TYPE errorNameToType(const std::string& name) {
		if (name == "ERR_UNEXP_EOF") return ERROR_TYPE::ERR_UNEXP_EOF;
		if (name == "ERR_SYM_MIS") return ERROR_TYPE::ERR_SYM_MIS;
		if (name == "ERR_UNKWN_SYM") return ERROR_TYPE::ERR_UNKWN_SYM;
		if (name == "ERR_PARAM_MIS") return ERROR_TYPE::ERR_PARAM_MIS;
		if (name == "ERR_PARAM_NBOOL") return ERROR_TYPE::ERR_PARAM_NBOOL;
		if (name == "ERR_PARAM_NNUM") return ERROR_TYPE::ERR_PARAM_NNUM;
		if (name == "ERR_PARAM_NSAME") return ERROR_TYPE::ERR_PARAM_NSAME;
		if (name == "ERR_LOGIC") return ERROR_TYPE::ERR_LOGIC;
		if (name == "ERR_MUL_DECL") return ERROR_TYPE::ERR_MUL_DECL;
		if (name == "ERR_MUL_DEF") return ERROR_TYPE::ERR_MUL_DEF;
		if (name == "ERR_ZERO_DIVISOR") return ERROR_TYPE::ERR_ZERO_DIVISOR;
		if (name == "ERR_FUN_LOCAL_VAR") return ERROR_TYPE::ERR_FUN_LOCAL_VAR;
		if (name == "ERR_ARI_MIS") return ERROR_TYPE::ERR_ARI_MIS;
		if (name == "ERR_TYPE_MIS") return ERROR_TYPE::ERR_TYPE_MIS;
		if (name == "ERR_NEG_PARAM") return ERROR_TYPE::ERR_NEG_PARAM;
		return ERROR_TYPE::ERR_UNKWN_SYM;
	}
	std::shared_ptr<DAGNode> Parser::mkErr(const ERROR_TYPE t){
		return getNodeManager()->createNode(NODE_KIND::NT_ERROR, errorTypeToName(t));
	}
	void Parser::err_all(const ERROR_TYPE e, const std::string s, const size_t ln) const {
		switch (e) {
		case ERROR_TYPE::ERR_UNEXP_EOF:
			err_unexp_eof();
			break;
		case ERROR_TYPE::ERR_SYM_MIS:
			err_sym_mis(s, ln);
			break;
		case ERROR_TYPE::ERR_UNKWN_SYM:
			err_unkwn_sym(s, ln);
			break;
		case ERROR_TYPE::ERR_PARAM_MIS:
			err_param_mis(s, ln);
			break;
		case ERROR_TYPE::ERR_PARAM_NBOOL:
			err_param_nbool(s, ln);
			break;
		case ERROR_TYPE::ERR_PARAM_NNUM:
			err_param_nnum(s, ln);
			break;
		case ERROR_TYPE::ERR_PARAM_NSAME:
			err_param_nsame(s, ln);
			break;
		case ERROR_TYPE::ERR_LOGIC:
			err_logic(s, ln);
			break;
		case ERROR_TYPE::ERR_MUL_DECL:
			err_mul_decl(s, ln);
			break;
		case ERROR_TYPE::ERR_MUL_DEF:
			err_mul_def(s, ln);
			break;
		case ERROR_TYPE::ERR_ZERO_DIVISOR:
			err_zero_divisor(ln);
			break;
		case ERROR_TYPE::ERR_FUN_LOCAL_VAR:
			err_param_nsame(s, ln);
			break;
		case ERROR_TYPE::ERR_ARI_MIS:
			err_arity_mis(s, ln);
			break;
		case ERROR_TYPE::ERR_TYPE_MIS:
			err_type_mis(s, ln);
			break;
		case ERROR_TYPE::ERR_NEG_PARAM:
			err_neg_param(ln);
			break;
		}
	}

	void Parser::err_all(const std::shared_ptr<DAGNode> e, const std::string s, const size_t ln) const {
		if (e->getKind() != NODE_KIND::NT_ERROR) {
			err_all(ERROR_TYPE::ERR_TYPE_MIS, s, ln);
			return;
		}
		err_all(errorNameToType(e->getName()), s, ln);
	}

	// unexpected end of file
	void Parser::err_unexp_eof() const {
		std::cout << "error: Unexpected end of file found." << std::endl;
		throw ParseErrorException();
	}

	// symbol missing
	void Parser::err_sym_mis(const std::string mis, const size_t ln) const {
		std::cout << "error: \"" << mis << "\" missing in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	void Parser::err_sym_mis(const std::string mis, const std::string nm, const size_t ln) const {
		std::cout << "error: \"" << mis << "\" missing before \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// unknown symbol
	void Parser::err_unkwn_sym(const std::string nm, const size_t ln) const {
		if (nm == "") err_unexp_eof();
		std::cout << "error: Unknown or unexpected symbol \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// wrong number of parameters
	void Parser::err_param_mis(const std::string nm, const size_t ln) const {
		std::cout << "error: Wrong number of parameters of \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// paramerter type error
	void Parser::err_param_nbool(const std::string nm, const size_t ln) const {
		std::cout << "error: Invalid command \"" << nm << "\" in line "
			<< ln << ", paramerter is not a boolean." << std::endl;
		throw ParseErrorException();
	}

	void Parser::err_param_nnum(const std::string nm, const size_t ln) const {
		std::cout << "error: Invalid command \"" << nm << "\" in line "
			<< ln << ", paramerter is not an integer or a real." << std::endl;
		throw ParseErrorException();
	}

	// paramerters are not in same type
	void Parser::err_param_nsame(const std::string nm, const size_t ln) const {
		std::cout << "error: Invalid command \"" << nm << "\" in line "
			<< ln << ", paramerters are not in same type." << std::endl;
		throw ParseErrorException();
	}

	// logic doesnt support
	void Parser::err_logic(const std::string nm, const size_t ln) const {
		std::cout << "error: Logic does not support \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// multiple declaration
	void Parser::err_mul_decl(const std::string nm, const size_t ln) const {
		std::cout << "error: Multiple declarations of \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// multiple definition
	void Parser::err_mul_def(const std::string nm, const size_t ln) const {
		std::cout << "error: Multiple definitions or keybindings of \""
			<< nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// divisor is zero
	void Parser::err_zero_divisor(const size_t ln) const {
		std::cout << "error: Divisor is zero in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// arity mismatch
	void Parser::err_arity_mis(const std::string nm, const size_t ln) const{
		std::cout << "error: Arity mismatch of command \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// kind mismatch
	void Parser::err_type_mis(const std::string nm, const size_t ln) const{
		std::cout << "error: Kind mismatch of command \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}


	void Parser::err_neg_param(const size_t ln) const{
		std::cout << "error: Negative parameter in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	// keyword error
	void Parser::err_keyword(const std::string nm, const size_t ln) const{
		std::cout << "error: keyword mismatch of command \"" << nm << "\" in line " << ln << '.' << std::endl;
		throw ParseErrorException();
	}

	/*
	global errors
	*/
	// cannot open file
	void Parser::err_open_file(const std::string filename) const {
		std::cout << "error: Cannot open file \"" << filename << "\"." << std::endl;
		throw ParseErrorException();
	}

	std::shared_ptr<DAGNode> Parser::rename(std::shared_ptr<DAGNode> expr, const std::string& new_name){
		condAssert(expr->isVar(), "Only variable can be renamed");
		std::string old_name = expr->getName();
		if(expr->isTempVar()){
			getSymbolManager()->renameTempVar(old_name, new_name);
		}
		else{
			getSymbolManager()->renameVar(old_name, new_name);
		}
		expr->rename(new_name);

		return expr;
	}	

	std::string Parser::toString(std::shared_ptr<DAGNode> expr){
		return dumpSMTLIB2(expr);
	}

	std::string Parser::toString(std::shared_ptr<Sort> sort){
		return sort->toString();
	}

	std::string Parser::toString(const NODE_KIND& kind){
		return kindToString(kind);
	}

	std::string Parser::optionToString(){
		return getOptions()->toString();
	}

	std::string Parser::dumpSMT2(){
		std::stringstream ss;
		ss << "(set-logic " << getOptions()->getLogic() << ")" << std::endl;
		// custom sorts
		for(const auto& sort_pair : getSymbolManager()->getSortKeyMap()){
			if(sort_pair.second->isDec()){
				ss << "(declare-sort " << sort_pair.first << " " << sort_pair.second->arity << ")" << std::endl;
			}
		}
		// variables
		std::vector<std::shared_ptr<DAGNode>> vars = getVariables();
		for(auto& var : vars){
			ss << "(declare-fun " << var->getName() << " () " << var->getSort()->toString() << ")" << std::endl;
		}
	std::vector<std::shared_ptr<DAGNode>> functions = getFunctions();
	for(auto& func : functions){
		if(func->isFuncDec()){
			// NT_FUNC_DEC: Uninterpreted function declaration (declare-fun)
			ss << dumpFuncDec(func) << std::endl;
		}
		else if(func->isFuncRec()){
			// NT_FUNC_REC: Recursive function definition (define-fun-rec)
			ss << dumpFuncRec(func) << std::endl;
		}
		else{
			// NT_FUNC_DEF: Regular function definition (define-fun)
			ss << dumpFuncDef(func) << std::endl;
		}
	}
		// constraints
		for(auto& constraint : context_.assertions){
			ss << "(assert " << dumpSMTLIB2(constraint) << ")" << std::endl;
		}
		ss << "(check-sat)" << std::endl;
		ss << "(exit)" << std::endl;
		return ss.str();
	}

	std::string Parser::dumpSMT2(const std::string& filename){
		std::ofstream file(filename);
		file << dumpSMT2();
		file.close();
		return filename;
	}

	size_t Parser::removeFuns(const std::vector<std::string>& funcNames){
		size_t removedCount = 0;
		
		for(const auto& funcName : funcNames){
			if(getSymbolManager()->getFun(funcName)){
				getSymbolManager()->removeFun(funcName);
				removedCount++;
			}
		}
		
		return removedCount;
	}

	/*
	warnings
	*/
	// command not support
	void Parser::warn_cmd_nsup(const std::string nm, const size_t ln) const {
		std::cout << "warning: \"" << nm << "\" command is safely ignored in line " << ln << "." << std::endl;
	}

	ParserPtr newParser(){
		return std::make_shared<Parser>();
	}

	ParserPtr newParser(const std::string& filename){
		return std::make_shared<Parser>(filename);
	}

	bool Parser::isUnit(std::shared_ptr<DAGNode> node) const {
		if (!node) {
			return true;
		}
		
		std::stack<std::shared_ptr<DAGNode>> nodeStack;
		std::unordered_set<std::shared_ptr<DAGNode>> visited;
		std::unordered_set<std::shared_ptr<DAGNode>> vars;
		
		nodeStack.push(node);
		
		while (!nodeStack.empty()) {
			std::shared_ptr<DAGNode> currentNode = nodeStack.top();
			nodeStack.pop();
			
			// if visited, skip
			if (visited.find(currentNode) != visited.end()) {
				continue;
			}
			visited.insert(currentNode);
			
			// if current node is a variable, add to the variable set
			if (currentNode->isVar()) {
				vars.insert(currentNode);
				if(vars.size() > 1){
					return false;
				}
			}
			
			// push the children to the stack
			for (size_t i = 0; i < currentNode->getChildrenSize(); i++) {
				std::shared_ptr<DAGNode> child = currentNode->getChild(i);
				if (visited.find(child) == visited.end()) {
					nodeStack.push(child);
				}
			}
		}
		
		// if there is no variable or only one variable, return true
		if (vars.size() <= 1) {
			return true;
		}

		return false;
	}

	bool Parser::isUnitAtom(std::shared_ptr<DAGNode> node) const {
		return node && isUnit(node) && node->isAtom();
	}

	std::shared_ptr<DAGNode> Parser::getUnitVar(std::shared_ptr<DAGNode> node) const {
		if (!node) {
			return NodeManager::NULL_NODE;
		}
		
		// use iterative method to traverse all nodes, collect variables
		std::stack<std::shared_ptr<DAGNode>> nodeStack;
		std::unordered_set<std::shared_ptr<DAGNode>> visited;
		std::unordered_set<std::shared_ptr<DAGNode>> vars;
		
		nodeStack.push(node);
		
		while (!nodeStack.empty()) {
			std::shared_ptr<DAGNode> currentNode = nodeStack.top();
			nodeStack.pop();
			
			// if visited, skip
			if (visited.find(currentNode) != visited.end()) {
				continue;
			}
			visited.insert(currentNode);
			
			// if current node is a variable, add to the variable set
			if (currentNode->isVar()) {
				vars.insert(currentNode);
				if(vars.size() > 1){
					return NodeManager::NULL_NODE;
				}
			}
			
			// push the children to the stack
			for (size_t i = 0; i < currentNode->getChildrenSize(); i++) {
				std::shared_ptr<DAGNode> child = currentNode->getChild(i);
				if (visited.find(child) == visited.end()) {
					nodeStack.push(child);
				}
			}
		}
		
		// if there is only one variable, return it
		if (vars.size() == 1) {
			return *vars.begin();
		}

		return NodeManager::NULL_NODE;
	}

}
