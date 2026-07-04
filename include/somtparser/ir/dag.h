/* -*- Header -*-
 *
 * The Directed Acyclic Graph (DAG) Class
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

#ifndef DAG_HEADER
#define DAG_HEADER

#include "somtparser/core/kind.h"
#include "somtparser/ir/sort.h"
#include "somtparser/core/util.h"
#include "somtparser/ir/value.h"
#include "somtparser/core/timing.h"

#include <iostream>
#include <fstream>

#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <memory>
#include <functional> // for std::hash

#include <unordered_map>
#include <unordered_set>
#include <array>

// II-2b-3: forward-declare the SOMTArena Arena so DAGNode can carry an arena handle (its ExprId +
// the owning arena) WITHOUT pulling the heavy somtarena/Arena.h into this widely-included header.
// A pointer to a forward-declared class is all P0 needs (set/get only — never dereferenced here).
namespace somtarena { class Arena; }

#ifdef SOMTPARSER_WITH_ARENA
// II-2b-3 (P3.a): parser-side ExprId -> Sort read registry (only forward-declares Sort, so no cycle
// with this widely-included header). getSort() reads it under the SOMTP_DAGNODE_ARENA_READS flag.
#include "somtparser/arena/read_registry.h"
// II-2b-3 (foundation): the full SOMTArena Arena (somtarena::Kind + Arena::kind), so arenaKind() can
// read a node's Kind straight from the shared arena. Arena.h is in the LOWER SOMTArena layer and never
// includes any SOMTParser type (layering), so this does NOT cycle — unlike map.h, which pulls parser.h
// back in. mapKind() is only forward-declared for that same reason (its symbol links from
// src/arena/map.cpp); arenaKind()'s field-net calls it to derive the Kind from the NODE_KIND field.
#include "somtarena/Arena.h"
#include <cassert>
namespace xarena_cov { somtarena::Kind mapKind(SOMTParser::NODE_KIND k, bool& mapped); }
#endif

namespace SOMTParser{
#ifdef SOMTPARSER_WITH_ARENA
    // II-2b-3 (foundation): thread_local front-end-phase flag. TRUE across the whole window in which the
    // parser's DAGNode graph is read via is*/getKind — parse + import (the arena builder installInline-
    // ArenaBuilder / buildAssertions set it) + the get-value/declared-var capture — and cleared FALSE by
    // the parent (Solver_impl_solve.cpp) right before parser.reset(), i.e. before solving. arenaKind()
    // reads it as a field-net: while TRUE (or a node is handle-less), is*/getKind derive the Kind from
    // the NODE_KIND field via mapKind rather than the arena, because (1) let scaffolding forwards a
    // child's ExprId whose arena Kind would misclassify the let node, and (2) the NRA discard path leaves
    // stale handles. thread_local because solving runs on a worker thread and a standalone DAGNode cannot
    // reach NodeManager parse-state. Defined in src/arena/build.cpp. UNWIRED this increment.
    extern thread_local bool g_frontendPhase;
    extern thread_local const somtarena::Arena* g_liveArena;
#endif
    // Forward declaration of DAGNode class
    class DAGNode;
    
    // define hash function and equal function for std::pair<const DAGNode*, const DAGNode*>
    struct PairNodePtrHash {
        size_t operator()(const std::pair<const DAGNode*, const DAGNode*>& p) const {
            return std::hash<const void*>()(p.first) ^ std::hash<const void*>()(p.second);
        }
    };

    struct PairNodePtrEqual {
        bool operator()(const std::pair<const DAGNode*, const DAGNode*>& p1, const std::pair<const DAGNode*, const DAGNode*>& p2) const {
            return p1.first == p2.first && p1.second == p2.second;
        }
    };

    class DAGNode {
    // <sort, kind, name> --- <sort, node_kind, name>
    private:
        std::shared_ptr<Sort>                   sort;
        NODE_KIND		                        kind;
        std::string		                        name;
        std::shared_ptr<Value>                  value;
        std::vector<std::shared_ptr<DAGNode>>   children;

        std::string                             children_hash;
        mutable size_t                          cached_hash_code;
        mutable bool                            hash_computed;
        mutable size_t                          _use_count;

#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P0): arena handle. The ExprId of the SOMTArena node this DAGNode was built into by
        // the proven buildArena walk, plus the arena it lives in. Populated for CORE nodes only
        // (post-let-elim: ops/consts/vars/apply/quant); transient scaffolding (let/let-bind/match) is
        // never given one. arenaExprId_ == 0 (somtarena::NullExpr) means "no arena node". Raw uint64 +
        // opaque Arena* keeps somtarena/Arena.h out of this header. Unused until the P2 accessor flip;
        // merely populating it is verdict-neutral — the 808 native parity (cmp_native.sh) is the gate.
        const somtarena::Arena*                 arenaPtr_ = nullptr;
        std::uint64_t                           arenaExprId_ = 0;
        // II-2b-3 (foundation): "this DAGNode OWNS its arena node" (true) vs "it forwards a child's
        // ExprId as an alias" (false — the let case). Set true ONLY at the own-handle setArenaHandle
        // sites (core / quantifier / singleton in build.cpp); left false at the two let-FORWARD sites.
        // arenaKind() asserts on it to trap, in CI/debug, any future is* caller that reaches the arena
        // through a forwarded (misclassifying) handle. Unread until the is* migration.
        bool                                    finalized_ = false;
#endif

    public:
        DAGNode(std::shared_ptr<Sort> sort, NODE_KIND kind, std::string name, std::vector<std::shared_ptr<DAGNode>> children): sort(sort), kind(kind), name(name), value(nullptr), children(children){
            // value is not used for hash
            if(children.empty()) {
                children_hash = "";
            } else {
                size_t combined_hash = 0;
                for(size_t i = 0; i < children.size(); i++) {
                    size_t child_hash = children[i]->hashCode();
                    combined_hash ^= child_hash + 0x9e3779b9 + (combined_hash << 6) + (combined_hash >> 2);
                }
                children_hash = std::to_string(combined_hash);
            }
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;

            if(kind == NODE_KIND::NT_CONST){
                if(TypeChecker::isInt(name)){
                    value = newValue(Number(name, true));
                } else if(TypeChecker::isReal(name)){
                    value = newValue(Number(name, false));
                } else if(sort && sort->isBv() && name.size() >= 2) {
                    // Parse BV constant value from #b / #x / decimal format
                    try {
                        if(name[0] == '#' && name[1] == 'b') {
                            // Binary: #b10110
                            Integer bv_val(0);
                            for(size_t i = 2; i < name.size(); ++i) {
                                bv_val = bv_val * 2 + (name[i] == '1' ? 1 : 0);
                            }
                            value = newValue(Number(bv_val));
                            value->setType(ValueType::BV);
                            value->setBvWidth(static_cast<uint32_t>(sort->getBitWidth()));
                        } else if(name[0] == '#' && name[1] == 'x') {
                            // Hex: #xFF
                            Integer bv_val(0);
                            for(size_t i = 2; i < name.size(); ++i) {
                                bv_val = bv_val * 16;
                                char c = name[i];
                                if(c >= '0' && c <= '9') bv_val = bv_val + (c - '0');
                                else if(c >= 'a' && c <= 'f') bv_val = bv_val + (c - 'a' + 10);
                                else if(c >= 'A' && c <= 'F') bv_val = bv_val + (c - 'A' + 10);
                            }
                            value = newValue(Number(bv_val));
                            value->setType(ValueType::BV);
                            value->setBvWidth(static_cast<uint32_t>(sort->getBitWidth()));
                        }
                    } catch(...) {
                        // If parsing fails, leave value as nullptr
                    }
                }
            }
        }
        DAGNode(std::shared_ptr<Sort> sort, NODE_KIND kind, std::string name): sort(sort), kind(kind), name(name), value(nullptr) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;

            if(kind == NODE_KIND::NT_CONST){
                if(TypeChecker::isInt(name)){
                    value = newValue(Number(name, true));
                } else if(TypeChecker::isReal(name)){
                    value = newValue(Number(name, false));
                } else if(sort && sort->isBv() && name.size() >= 2) {
                    // Parse BV constant value from #b / #x / decimal format
                    try {
                        if(name[0] == '#' && name[1] == 'b') {
                            Integer bv_val(0);
                            for(size_t i = 2; i < name.size(); ++i) {
                                bv_val = bv_val * 2 + (name[i] == '1' ? 1 : 0);
                            }
                            value = newValue(Number(bv_val));
                            value->setType(ValueType::BV);
                            value->setBvWidth(static_cast<uint32_t>(sort->getBitWidth()));
                        } else if(name[0] == '#' && name[1] == 'x') {
                            Integer bv_val(0);
                            for(size_t i = 2; i < name.size(); ++i) {
                                bv_val = bv_val * 16;
                                char c = name[i];
                                if(c >= '0' && c <= '9') bv_val = bv_val + (c - '0');
                                else if(c >= 'a' && c <= 'f') bv_val = bv_val + (c - 'a' + 10);
                                else if(c >= 'A' && c <= 'F') bv_val = bv_val + (c - 'A' + 10);
                            }
                            value = newValue(Number(bv_val));
                            value->setType(ValueType::BV);
                            value->setBvWidth(static_cast<uint32_t>(sort->getBitWidth()));
                        }
                    } catch(...) {}
                }
            }
        }
        DAGNode(std::shared_ptr<Sort> sort, NODE_KIND kind): sort(sort), kind(kind), name(""), value(nullptr) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            
            if(kind == NODE_KIND::NT_CONST){
                value = newValue(Number());
            }
        }
        DAGNode(std::shared_ptr<Sort> sort): sort(sort), kind(NODE_KIND::NT_UNKNOWN), name(""), value(nullptr) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;

            if(kind == NODE_KIND::NT_CONST){
                value = newValue(Number());
            }
        }
        DAGNode(): sort(SortManager::NULL_SORT), kind(NODE_KIND::NT_UNKNOWN), name(""), value(nullptr), children_hash(""), cached_hash_code(0), hash_computed(false), _use_count(1) {
            children_hash = "";
        }
        DAGNode(const DAGNode& other): sort(other.sort), kind(other.kind), name(other.name), value(other.value), children(other.children), children_hash(other.children_hash), cached_hash_code(0), hash_computed(false), _use_count(1) {}

        // other initialization
        DAGNode(NODE_KIND kind, std::string name): sort(SortManager::NULL_SORT), kind(kind), name(name), value(nullptr) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;

            if(kind == NODE_KIND::NT_CONST){
                if(TypeChecker::isInt(name)){
                    value = newValue(Number(name, true));
                } else if(TypeChecker::isReal(name)){
                    value = newValue(Number(name, false));
                } 
            }
        }
        DAGNode(NODE_KIND kind): sort(SortManager::NULL_SORT), kind(kind), name(""), value(nullptr) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;

            if(kind == NODE_KIND::NT_CONST){
                value = newValue(Number());
            }
        }
        DAGNode(std::shared_ptr<Sort> sort, const Integer& v): sort(sort), kind(NODE_KIND::NT_CONST), name(""), value(newValue(v)) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            name = v.toString();
        }
        DAGNode(std::shared_ptr<Sort> sort, const Real& v): sort(sort), kind(NODE_KIND::NT_CONST), name(""), value(newValue(v)) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            name = v.toString();
        }
        DAGNode(std::shared_ptr<Sort> sort, const double& v): sort(sort), kind(NODE_KIND::NT_CONST), name(""), value(newValue(v)) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            name = std::to_string(v);
        }
        DAGNode(std::shared_ptr<Sort> sort, const int& v): sort(sort), kind(NODE_KIND::NT_CONST), name(""), value(newValue(v)) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            name = std::to_string(v);
        }
        DAGNode(std::shared_ptr<Sort> sort, const bool& v): sort(sort), kind(NODE_KIND::NT_CONST), name(""), value(newValue(v)) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            name = v ? "true" : "false";
        }
        
        // only constant
        DAGNode(const std::string& n) {
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            _use_count = 0;
            if (n == "true") {
                sort = SortManager::BOOL_SORT;
                kind = NODE_KIND::NT_CONST_TRUE;
            } else if (n == "false") {
                sort = SortManager::BOOL_SORT;
                kind = NODE_KIND::NT_CONST_FALSE;
            } else if (n == "pi") {
                sort = SortManager::REAL_SORT;
                kind = NODE_KIND::NT_CONST_PI;
            } else if (n == "e") {
                sort = SortManager::REAL_SORT;
                kind = NODE_KIND::NT_CONST_E;
            } else if (n == "inf") {
                sort = SortManager::REAL_SORT;
                kind = NODE_KIND::NT_INFINITY;
            } else if (n == "NaN" || n == "+NaN" || n == "-NaN") {
                sort = SortManager::EXT_SORT;
                kind = NODE_KIND::NT_NAN;
            } else if (n == "epsilon") {
                sort = SortManager::REAL_SORT;
                kind = NODE_KIND::NT_EPSILON;
            } else if(n == "NULL") {
                sort = SortManager::NULL_SORT;
                kind = NODE_KIND::NT_NULL;
            } else if(TypeChecker::isInt(n)){
                sort = SortManager::INT_SORT;
                kind = NODE_KIND::NT_CONST;
                value = newValue(Number(n, true));
            } else if(TypeChecker::isReal(n)){
                sort = SortManager::REAL_SORT;
                kind = NODE_KIND::NT_CONST;
                value = newValue(Number(n, false));
            } 
            else if(TypeChecker::isString(n)){
                sort = SortManager::STR_SORT;
                kind = NODE_KIND::NT_CONST;
            } else {
                sort = SortManager::NULL_SORT;
                kind = NODE_KIND::NT_ERROR;
            }
            name = n;
        }
        
        void clear(){
            kind = NODE_KIND::NT_ERROR;
            children.clear();
            children_hash = "";
            cached_hash_code = 0;
            hash_computed = false;
            name = "";
            _use_count = 0;
        }

        bool operator==(const DAGNode elem)
        {
            return (sort == elem.sort && kind == elem.kind && name == elem.name && children_hash == elem.children_hash);
        }
        bool operator!=(const DAGNode elem)
        {
            return (sort != elem.sort || kind != elem.kind || name != elem.name || children_hash != elem.children_hash);
        }
        
        bool isLeaf() 				const { return children.empty(); };
        bool isInternal() 			const { return !children.empty(); };

        // check null
        bool isNull() 				const { return kind == NODE_KIND::NT_NULL; };
        
        // check error
        bool isErr() 				const { return (kind == NODE_KIND::NT_ERROR); };

        // check unknown
        bool isUnknown() 			const { return kind == NODE_KIND::NT_UNKNOWN; };

        // check const
        bool isCBool() 				const { return (kind == NODE_KIND::NT_CONST_TRUE || kind == NODE_KIND::NT_CONST_FALSE) && sort->isBool(); }; 
        bool isTrue() 				const { return kind == NODE_KIND::NT_CONST_TRUE && sort->isBool(); };
        bool isFalse() 			    const { return kind == NODE_KIND::NT_CONST_FALSE && sort->isBool(); };
        bool isConst() 				const { return  kind == NODE_KIND::NT_CONST || 
                                                    kind == NODE_KIND::NT_CONST_TRUE || kind == NODE_KIND::NT_CONST_FALSE ||
                                                    kind == NODE_KIND::NT_CONST_PI || kind == NODE_KIND::NT_CONST_E ||
                                                    kind == NODE_KIND::NT_INFINITY || kind == NODE_KIND::NT_POS_INFINITY || kind == NODE_KIND::NT_NEG_INFINITY ||
                                                    kind == NODE_KIND::NT_NAN || kind == NODE_KIND::NT_EPSILON || 
                                                    kind == NODE_KIND::NT_POS_EPSILON || kind == NODE_KIND::NT_NEG_EPSILON ||
                                                    kind == NODE_KIND::NT_ROOT_OF_WITH_INTERVAL || kind == NODE_KIND::NT_ROOT_OBJ || kind == NODE_KIND::NT_REAL_ALGEBRAIC_NUMBER; };
        bool isNumeral() 			const { return isCInt() || isCReal(); };
        bool isCInt()       		const { return isConst() && (sort->isInt() || sort->isIntOrReal()); };
        bool isCReal()      		const { return isConst() && (sort->isReal() || sort->isIntOrReal()); };
        bool isCBV()        		const { return isConst() && sort->isBv(); };
        bool isCFP()        		const { return isConst() && sort->isFp(); };
        bool isCRoundingMode()      const { return isConst() && sort->isRoundingMode(); };
        bool isCRootOfWithInterval() const { return isConst() && kind == NODE_KIND::NT_ROOT_OF_WITH_INTERVAL; };
        bool isCRootObj()           const { return isConst() && kind == NODE_KIND::NT_ROOT_OBJ; };
        bool isCRealAlgebraicNumber() const { return isConst() && kind == NODE_KIND::NT_REAL_ALGEBRAIC_NUMBER; };
        bool isCStr()       		const { return isConst() && sort->isStr(); };

        // check var
        bool isVBool() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isBool(); };
        bool isLiteral() 			const { return (isVBool() || (isNot() && getChild(0)->isVBool()) || isCBool()); };
        bool isVInt() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isInt(); };
        bool isVReal() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isReal(); };
        bool isVBV() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isBv(); };
        bool isVFP() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isFp(); };
        bool isVRoundingMode()      const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isRoundingMode(); };
        bool isVStr() 				const { return (kind == NODE_KIND::NT_VAR || kind == NODE_KIND::NT_TEMP_VAR || kind == NODE_KIND::NT_QUANT_VAR || kind == NODE_KIND::NT_LET_BIND_VAR || kind == NODE_KIND::NT_PLACEHOLDER_VAR) && sort->isStr(); };
        bool isTempVar() 			const { return kind == NODE_KIND::NT_TEMP_VAR; };
        bool isQuantVar() 			const { return kind == NODE_KIND::NT_QUANT_VAR; };
        bool isLetBindVar() 		const { return kind == NODE_KIND::NT_LET_BIND_VAR; };
        bool isPlaceholderVar() 	const { return kind == NODE_KIND::NT_PLACEHOLDER_VAR; };
        bool isVar() 				const { return (kind == NODE_KIND::NT_VAR || isVBool() || isVInt() || isVReal() || isVBV() || isVFP() || isVRoundingMode() || isVStr() || isTempVar() || isQuantVar() || isLetBindVar() || isPlaceholderVar()); };
        
        // interval
        bool isMax() 				const { return arenaKind() == somtarena::Kind::Max; };
        bool isMin() 				const { return arenaKind() == somtarena::Kind::Min; };

        // check array
        // An array is: array variable, const array, or store operation (which returns array)
        bool isArray() 			    const { return sort && sort->isArray(); };
        bool isConstArray() 		const { return arenaKind() == somtarena::Kind::ConstArray; };

        bool isAssignableVar() 		const { return isVar() || isUFApplication(); };
        
        // check Boolean operations
        bool isAnd() 				const { return arenaKind() == somtarena::Kind::And; };
        bool isOr() 				const { return arenaKind() == somtarena::Kind::Or; };
        bool isNot() 				const { return arenaKind() == somtarena::Kind::Not; };
        bool isImplies() 				const { return arenaKind() == somtarena::Kind::Implies; };
        bool isXor() 				const { return arenaKind() == somtarena::Kind::Xor; };
        
        // check comparison
        bool isEqBool()             const { return (kind == NODE_KIND::NT_EQ_BOOL); };
        bool isEqOther()            const { return (kind == NODE_KIND::NT_EQ_OTHER); };
        bool isEq() 				const { return (kind == NODE_KIND::NT_EQ || isEqBool() || isEqOther()); };
        bool isDistinctBool()       const { return (kind == NODE_KIND::NT_DISTINCT_BOOL); };
        bool isDistinctOther()      const { return (kind == NODE_KIND::NT_DISTINCT_OTHER); };
        bool isDistinct() 			const { return (kind == NODE_KIND::NT_DISTINCT || isDistinctBool() || isDistinctOther()); };
        bool isNeq() 				const { return isDistinct(); };

        // check UF
        bool isUFApplication() 			const { return (kind == NODE_KIND::NT_UF_APPLY); };
        bool isConstructorApp()             const { return (kind == NODE_KIND::NT_DT_CONSTRUCTOR); };
        bool isSelectorApp()                const { return (kind == NODE_KIND::NT_DT_SELECTOR); };
        bool isTesterApp()                  const { return (kind == NODE_KIND::NT_DT_TESTER); };
        bool isMatchApp()                   const { return (kind == NODE_KIND::NT_DT_MATCH); };
        bool isDtGroundValue()              const {
            if(kind != NODE_KIND::NT_DT_CONSTRUCTOR) return false;
            for(size_t i = 0; i < getChildrenSize(); ++i){
                auto c = getChild(i);
                if(!c->isConst() && !c->isDtGroundValue()) return false;
            }
            return true;
        };

        // check arithmetic operations
        bool isAdd() 				const { return arenaKind() == somtarena::Kind::Add; };
        bool isSub() 				const { return arenaKind() == somtarena::Kind::Sub; };
        bool isMul() 				const { return arenaKind() == somtarena::Kind::Mul; };
        bool isNeg() 				const { return arenaKind() == somtarena::Kind::Neg; };
        bool isDivInt() 			const { return arenaKind() == somtarena::Kind::IntDiv; };
        bool isDivReal() 			const { return arenaKind() == somtarena::Kind::RealDiv; };
        bool isMod() 				const { return arenaKind() == somtarena::Kind::Mod; };
        bool isAbs() 				const { return arenaKind() == somtarena::Kind::Abs; };
        bool isCeil() 				const { return arenaKind() == somtarena::Kind::Ceil; };
        bool isFloor() 				const { return arenaKind() == somtarena::Kind::Floor; };
        bool isRound() 				const { return arenaKind() == somtarena::Kind::Round; };
        bool isArithOp() 			const { return (isAdd() || isSub() || isMul() || isNeg() || isDivInt() || isDivReal() || isMod() || isAbs() || isCeil() || isFloor() || isRound()); };
        
        // check transcendental operations
        bool isIAnd() 				const { return arenaKind() == somtarena::Kind::IntAnd; };
        bool isPow2() 				const { return arenaKind() == somtarena::Kind::Pow2; };
        bool isPow() 				const { return arenaKind() == somtarena::Kind::Pow; };
        bool isSqrt() 				const { return arenaKind() == somtarena::Kind::Sqrt; };
        bool isSafeSqrt() 			const { return arenaKind() == somtarena::Kind::SafeSqrt; };
        bool isRealNonlinearOp() 	const { return (isIAnd() || isPow2() || isPow() || isSqrt() || isSafeSqrt()); };
        bool isExp() 				const { return (kind == NODE_KIND::NT_EXP); };
        bool isLog() 				const { return (kind == NODE_KIND::NT_LOG); };
        bool isLn() 				const { return (kind == NODE_KIND::NT_LN); };
        bool isLb() 				const { return (kind == NODE_KIND::NT_LB); };
        bool isLg() 				const { return (kind == NODE_KIND::NT_LG); };
        bool isSin() 				const { return (kind == NODE_KIND::NT_SIN); };
        bool isCos() 				const { return (kind == NODE_KIND::NT_COS); };
        bool isSec() 				const { return (kind == NODE_KIND::NT_SEC); };
        bool isCsc() 				const { return (kind == NODE_KIND::NT_CSC); };
        bool isTan() 				const { return (kind == NODE_KIND::NT_TAN); };
        bool isCot() 				const { return (kind == NODE_KIND::NT_COT); };
        bool isAsin() 				const { return (kind == NODE_KIND::NT_ASIN); };
        bool isAcos() 				const { return (kind == NODE_KIND::NT_ACOS); };
        bool isAsec() 				const { return (kind == NODE_KIND::NT_ASEC); };
        bool isAcsc() 				const { return (kind == NODE_KIND::NT_ACSC); };
        bool isAtan() 				const { return (kind == NODE_KIND::NT_ATAN); };
        bool isAcot() 				const { return (kind == NODE_KIND::NT_ACOT); };
        bool isSinh() 				const { return (kind == NODE_KIND::NT_SINH); };
        bool isCosh() 				const { return (kind == NODE_KIND::NT_COSH); };
        bool isTanh() 				const { return (kind == NODE_KIND::NT_TANH); };
        bool isSech() 				const { return (kind == NODE_KIND::NT_SECH); };
        bool isCsch() 				const { return (kind == NODE_KIND::NT_CSCH); };
        bool isCoth() 				const { return (kind == NODE_KIND::NT_COTH); };
        bool isAsinh() 				const { return (kind == NODE_KIND::NT_ASINH); };
        bool isAcosh() 				const { return (kind == NODE_KIND::NT_ACOSH); };
        bool isAtanh() 				const { return (kind == NODE_KIND::NT_ATANH); };
        bool isAsech() 				const { return (kind == NODE_KIND::NT_ASECH); };
        bool isAcsch() 				const { return (kind == NODE_KIND::NT_ACSCH); };
        bool isAcoth() 				const { return (kind == NODE_KIND::NT_ACOTH); };
        bool isAtan2() 				const { return (kind == NODE_KIND::NT_ATAN2); };
        bool isTranscendentalOp() 	const { return (isExp() || isLog() || isLn() || isLb() || isLg() || isSin() || isCos() || isSec() || isCsc() || isTan() || isCot() || isAsin() || isAcos() || isAsec() || isAcsc() || isAtan() || isAcot() || isSinh() || isCosh() || isTanh() || isSech() || isCsch() || isCoth() || isAsinh() || isAcosh() || isAtanh() || isAsech() || isAcsch() || isAcoth() || isAtan2()); };

        // check arithmetic comparison
        bool isLe() 				const { return arenaKind() == somtarena::Kind::Le; };
        bool isLt() 				const { return arenaKind() == somtarena::Kind::Lt; };
        bool isGe() 				const { return arenaKind() == somtarena::Kind::Ge; };
        bool isGt() 				const { return arenaKind() == somtarena::Kind::Gt; };
        bool isArithTerm() 			const { return (isArithOp() || isArithConv() || isRealNonlinearOp() || isTranscendentalOp() || 
                                                    (isVar() && (isVInt() || isVReal())) ||
                                                    (isConst() && (isCInt() || isCReal())) ||
                                                    ((isIte() || isMax() || isMin() || isUFApplication()) && (sort->isInt() || sort->isReal() || sort->isIntOrReal()))); };
        bool isArithComp() 			const { return ((isEq() && getChild(0)->isArithTerm())|| 
                                                    (isDistinct() && getChild(0)->isArithTerm()) || 
                                                    isLe() || isLt() || isGe() || isGt()); };

        // check arithmetic covertion
        bool isToReal() 			const { return arenaKind() == somtarena::Kind::ToReal; };
        bool isToInt() 				const { return arenaKind() == somtarena::Kind::ToInt; };
        bool isArithConv() 			const { return (isToReal() || isToInt()); };

        // check arithmetic properties
        bool isInt() 				const { return (kind == NODE_KIND::NT_IS_INT); };
        bool isDivisible() 			const { return (kind == NODE_KIND::NT_IS_DIVISIBLE); };
        bool isPrime() 				const { return (kind == NODE_KIND::NT_IS_PRIME); };
        bool isEven() 				const { return (kind == NODE_KIND::NT_IS_EVEN); };
        bool isOdd() 				const { return (kind == NODE_KIND::NT_IS_ODD); };
        bool isArithProp() 			const { return (isInt() || isDivisible() || isPrime() || isEven() || isOdd()); };
        bool isArithAtom() 			const { return isArithComp() || isArithProp(); }

        // check arithmetic constants
        bool isPi() 				const { return (kind == NODE_KIND::NT_CONST_PI); };
        bool isE() 					const { return (kind == NODE_KIND::NT_CONST_E); };
        bool isInfinity() 			const { return (kind == NODE_KIND::NT_INFINITY || kind == NODE_KIND::NT_POS_INFINITY || kind == NODE_KIND::NT_NEG_INFINITY); };
        bool isPosInfinity() 		const { return (kind == NODE_KIND::NT_POS_INFINITY); };
        bool isNegInfinity() 		const { return (kind == NODE_KIND::NT_NEG_INFINITY); };
        bool isNaN() 				const { return (kind == NODE_KIND::NT_NAN); };
        bool isEpsilon() 			const { return (kind == NODE_KIND::NT_EPSILON || kind == NODE_KIND::NT_POS_EPSILON || kind == NODE_KIND::NT_NEG_EPSILON ); };
        bool isPosEpsilon() 		const { return (kind == NODE_KIND::NT_POS_EPSILON); };
        bool isNegEpsilon() 		const { return (kind == NODE_KIND::NT_NEG_EPSILON); };

        // check arithmetic functions
        // bool isSum() 				const { return (kind == NODE_KIND::NT_SUM); };
        // bool isProd() 				const { return (kind == NODE_KIND::NT_PROD); };
        bool isGcd() 				const { return (kind == NODE_KIND::NT_GCD); };
        bool isLcm() 				const { return (kind == NODE_KIND::NT_LCM); };
        bool isFact() 				const { return (kind == NODE_KIND::NT_FACT); };
        // Bit-wise operations
        bool isBVNot() 				const { return (kind == NODE_KIND::NT_BV_NOT); };
        bool isBVAnd() 				const { return (kind == NODE_KIND::NT_BV_AND); };
        bool isBVOr() 				const { return (kind == NODE_KIND::NT_BV_OR); };
        bool isBVXor() 				const { return (kind == NODE_KIND::NT_BV_XOR); };
        bool isBVNand() 			const { return (kind == NODE_KIND::NT_BV_NAND); };
        bool isBVNor() 				const { return (kind == NODE_KIND::NT_BV_NOR); };
        bool isBVXnor() 			const { return (kind == NODE_KIND::NT_BV_XNOR); };
        bool isBVComp() 			const { return (kind == NODE_KIND::NT_BV_COMP); };
        // Arithmetic operations
        bool isBVNeg() 				const { return (kind == NODE_KIND::NT_BV_NEG); };
        bool isBVAdd() 				const { return (kind == NODE_KIND::NT_BV_ADD); };
        bool isBVSub() 				const { return (kind == NODE_KIND::NT_BV_SUB); };
        bool isBVMul() 				const { return (kind == NODE_KIND::NT_BV_MUL); };
        bool isBVUDiv() 			const { return (kind == NODE_KIND::NT_BV_UDIV); };
        bool isBVURem() 			const { return (kind == NODE_KIND::NT_BV_UREM); };
        bool isBVSDiv() 			const { return (kind == NODE_KIND::NT_BV_SDIV); };
        bool isBVSRem() 			const { return (kind == NODE_KIND::NT_BV_SREM); };
        bool isBVUMod() 			const { return (kind == NODE_KIND::NT_BV_UMOD); };
        bool isBVSMod() 			const { return (kind == NODE_KIND::NT_BV_SMOD); };
        // Arithmetic operations with overflow
        bool isBVNegO() 			const { return (kind == NODE_KIND::NT_BV_NEGO); };
        bool isBVUAddO() 			const { return (kind == NODE_KIND::NT_BV_UADDO); };
        bool isBVSAddO() 			const { return (kind == NODE_KIND::NT_BV_SADDO); };
        bool isBVUMulO() 			const { return (kind == NODE_KIND::NT_BV_UMULO); };
        bool isBVSMulO() 			const { return (kind == NODE_KIND::NT_BV_SMULO); };
        bool isBVUDivO() 			const { return (kind == NODE_KIND::NT_BV_UDIVO); };
        bool isBVSDivO() 			const { return (kind == NODE_KIND::NT_BV_SDIVO); };
        bool isBVURemO() 			const { return (kind == NODE_KIND::NT_BV_UREMO); };
        bool isBVSRemO() 			const { return (kind == NODE_KIND::NT_BV_SREMO); };
        bool isBVUModO() 			const { return (kind == NODE_KIND::NT_BV_UMODO); };
        bool isBVSModO() 			const { return (kind == NODE_KIND::NT_BV_SMODO); };
        // Shift operations
        bool isBVShl() 				const { return (kind == NODE_KIND::NT_BV_SHL); };
        bool isBVLSHR() 			const { return (kind == NODE_KIND::NT_BV_LSHR); };
        bool isBVASHR() 			const { return (kind == NODE_KIND::NT_BV_ASHR); };
        bool isBVConcat() 			const { return (kind == NODE_KIND::NT_BV_CONCAT); };
        bool isBVExtract() 			const { return (kind == NODE_KIND::NT_BV_EXTRACT); };
        bool isBVRepeat() 			const { return (kind == NODE_KIND::NT_BV_REPEAT); };
        bool isBVZeroExt() 			const { return (kind == NODE_KIND::NT_BV_ZERO_EXT); };
        bool isBVSignExt() 			const { return (kind == NODE_KIND::NT_BV_SIGN_EXT); };
        bool isBVRotLeft() 			const { return (kind == NODE_KIND::NT_BV_ROTATE_LEFT); };
        bool isBVRotRight() 		const { return (kind == NODE_KIND::NT_BV_ROTATE_RIGHT); };
        bool isBVOp() 	    		const { return (isBVNot() || isBVAnd() || isBVOr() || isBVXor() || isBVNand() || isBVNor() || isBVXnor() || isBVAdd() || isBVSub() || isBVMul() || isBVUDiv() || isBVURem() || isBVSDiv() || isBVSRem() || isBVSMod() || isBVShl() || isBVLSHR() || isBVASHR() || isBVConcat() || isBVExtract() || isBVRepeat() || isBVZeroExt() || isBVSignExt() || isBVRotLeft() || isBVRotRight()); };

        // check bitvector comparison
        bool isBVUlt() 	    		const { return (kind == NODE_KIND::NT_BV_ULT); };
        bool isBVUle() 	    		const { return (kind == NODE_KIND::NT_BV_ULE); };
        bool isBVUgt() 	    		const { return (kind == NODE_KIND::NT_BV_UGT); };
        bool isBVUge() 	    		const { return (kind == NODE_KIND::NT_BV_UGE); };
        bool isBVSlt() 	    		const { return (kind == NODE_KIND::NT_BV_SLT); };
        bool isBVSle() 	    		const { return (kind == NODE_KIND::NT_BV_SLE); };
        bool isBVSgt() 	    		const { return (kind == NODE_KIND::NT_BV_SGT); };
        bool isBVSge() 	    		const { return (kind == NODE_KIND::NT_BV_SGE); };
        bool isBVTerm()    		    const { return (isBVOp() ||
                                                    (isVar() && isVBV()) ||
                                                    (isConst() && isCBV()) ||
                                                    (isIte() && sort->isBv()) ||
                                                    (isMax() && sort->isBv()) ||
                                                    (isMin() && sort->isBv()) ||
                                                    (isUFApplication() && sort->isBv())); };
        bool isBVCompOp()     		const {
            bool eqOrDistinctBv = (getChildrenSize() >= 1) &&
                ((isEq() && getChild(0)->isBVTerm()) || (isDistinct() && getChild(0)->isBVTerm()));
            return eqOrDistinctBv || isBVUlt() || isBVUle() || isBVUgt() || isBVUge() ||
                   isBVSlt() || isBVSle() || isBVSgt() || isBVSge();
        }
        bool isBVAtom()              const { return isBVCompOp(); } 

        // check bitvector conversion
        bool isBVToNat() 			const { return (kind == NODE_KIND::NT_BV_TO_NAT); };
        bool isNatToBV() 			const { return (kind == NODE_KIND::NT_NAT_TO_BV); };
        bool isBVToInt() 			const { return (kind == NODE_KIND::NT_BV_TO_INT); };
        bool isIntToBV() 			const { return (kind == NODE_KIND::NT_INT_TO_BV); };
        bool isBVConv() 			const { return (isBVToNat() || isNatToBV() || isBVToInt() || isIntToBV()); };

        // check floating point common operators
        bool isFPAdd() 				const { return (kind == NODE_KIND::NT_FP_ADD); };
        bool isFPSub() 				const { return (kind == NODE_KIND::NT_FP_SUB); };
        bool isFPMul() 				const { return (kind == NODE_KIND::NT_FP_MUL); };
        bool isFPDiv() 				const { return (kind == NODE_KIND::NT_FP_DIV); };
        bool isFPAbs() 				const { return (kind == NODE_KIND::NT_FP_ABS); };
        bool isFPNeg() 				const { return (kind == NODE_KIND::NT_FP_NEG); };
        bool isFPRem() 				const { return (kind == NODE_KIND::NT_FP_REM); };
        bool isFPFMA() 				const { return (kind == NODE_KIND::NT_FP_FMA); };
        bool isFPSqrt() 			const { return (kind == NODE_KIND::NT_FP_SQRT); };
        bool isFPRoundToIntegral()  const { return (kind == NODE_KIND::NT_FP_ROUND_TO_INTEGRAL); };
        bool isFPRoToInt()  		const { return (kind == NODE_KIND::NT_FP_ROUND_TO_INTEGRAL); };
        bool isFPMin() 				const { return (kind == NODE_KIND::NT_FP_MIN); };
        bool isFPMax() 				const { return (kind == NODE_KIND::NT_FP_MAX); };
        bool isFPOp() 				const { return (isFPAdd() || isFPSub() || isFPMul() || isFPDiv() || isFPAbs() || isFPNeg() || isFPRem() || isFPFMA() || isFPSqrt() || isFPRoToInt() || isFPMin() || isFPMax() || (isUFApplication() && sort->isFp())); };

        // check floating point comparison
        bool isFPLe() 				const { return (kind == NODE_KIND::NT_FP_LE); };
        bool isFPLt() 				const { return (kind == NODE_KIND::NT_FP_LT); };
        bool isFPGe() 				const { return (kind == NODE_KIND::NT_FP_GE); };
        bool isFPGt() 				const { return (kind == NODE_KIND::NT_FP_GT); };
        bool isFPEq() 				const { return (kind == NODE_KIND::NT_FP_EQ); };
        bool isFPComp() 				const {
            bool eqOrDistinctFp = (getChildrenSize() >= 1) &&
                ((isEq() && getChild(0)->isFPTerm()) || (isDistinct() && getChild(0)->isFPTerm()));
            return eqOrDistinctFp || isFPLe() || isFPLt() || isFPGe() || isFPGt() || isFPEq();
        }

        // check floating point conversion
        bool isFPToUBV() 			const { return (kind == NODE_KIND::NT_FP_TO_UBV); };
        bool isFPToSBV() 			const { return (kind == NODE_KIND::NT_FP_TO_SBV); };
        bool isFPToReal() 			const { return (kind == NODE_KIND::NT_FP_TO_REAL); };
        bool isToFP()     		    const { return (kind == NODE_KIND::NT_FP_TO_FP); };
        bool isToFPUnsigned()       const { return (kind == NODE_KIND::NT_FP_TO_FP_UNSIGNED); };

        bool isFPConv() 			const { return (isFPToUBV() || isFPToSBV() || isFPToReal() || isToFP() || isToFPUnsigned()); };

        bool isFPTerm() 				const {
            return isFPOp() || (isFPConv() && sort->isFp()) ||
                   (isVar() && isVFP()) || (isConst() && isCFP()) ||
                   (isIte() && sort->isFp()) ||
                   (isMax() && sort->isFp()) ||
                   (isMin() && sort->isFp());
        }

        // check floating point properties
        bool isFPIsNormal() 		const { return (kind == NODE_KIND::NT_FP_IS_NORMAL); };
        bool isFPIsSubnormal() 		const { return (kind == NODE_KIND::NT_FP_IS_SUBNORMAL); };
        bool isFPIsZero() 			const { return (kind == NODE_KIND::NT_FP_IS_ZERO); };
        bool isFPIsInf() 			const { return (kind == NODE_KIND::NT_FP_IS_INF); };
        bool isFPIsNaN() 			const { return (kind == NODE_KIND::NT_FP_IS_NAN); };
        bool isFPIsNeg() 			const { return (kind == NODE_KIND::NT_FP_IS_NEG); };
        bool isFPIsPos() 			const { return (kind == NODE_KIND::NT_FP_IS_POS); };
        bool isFPProp() 				const { return isFPIsNormal() || isFPIsSubnormal() || isFPIsZero() || isFPIsInf() || isFPIsNaN() || isFPIsNeg() || isFPIsPos(); }
        bool isFPAtom() 				const { return isFPComp() || isFPProp(); }

        // check array
        bool isSelect() 			const { return arenaKind() == somtarena::Kind::Select; };
        bool isStore() 				const { return arenaKind() == somtarena::Kind::Store; };
        bool isArrayOp() 			const { return (isSelect() || isStore() || (isUFApplication() && sort->isArray())); };

        // check strings common operators
        bool isStrLen() 			const { return (kind == NODE_KIND::NT_STR_LEN); };
        bool isStrConcat() 			const { return (kind == NODE_KIND::NT_STR_CONCAT); };
        bool isStrSubstr() 			const { return (kind == NODE_KIND::NT_STR_SUBSTR); };
        bool isStrPrefixof() 		const { return (kind == NODE_KIND::NT_STR_PREFIXOF); };
        bool isStrSuffixof() 		const { return (kind == NODE_KIND::NT_STR_SUFFIXOF); };
        bool isStrIndexof() 		const { return (kind == NODE_KIND::NT_STR_INDEXOF); };
        bool isStrCharat() 			const { return (kind == NODE_KIND::NT_STR_CHARAT); };
        bool isStrUpdate() 			const { return (kind == NODE_KIND::NT_STR_UPDATE); };
        bool isStrReplace() 		const { return (kind == NODE_KIND::NT_STR_REPLACE); };
        bool isStrReplaceAll() 		const { return (kind == NODE_KIND::NT_STR_REPLACE_ALL); };
        bool isStrToLower() 		const { return (kind == NODE_KIND::NT_STR_TO_LOWER); };
        bool isStrToUpper() 		const { return (kind == NODE_KIND::NT_STR_TO_UPPER); };
        bool isStrRev() 			const { return (kind == NODE_KIND::NT_STR_REV); };
        bool isStrSplit() 			const { return (kind == NODE_KIND::NT_STR_SPLIT); };
        bool isStrSplitAt() 		const { return (kind == NODE_KIND::NT_STR_SPLIT_AT); };
        bool isStrSplitRest() 		const { return (kind == NODE_KIND::NT_STR_SPLIT_REST); };
        bool isStrNumSplits() 		const { return (kind == NODE_KIND::NT_STR_NUM_SPLITS); };
        bool isStrSplitAtRe() 		const { return (kind == NODE_KIND::NT_STR_SPLIT_AT_RE); };
        bool isStrSplitRestRe() 		const { return (kind == NODE_KIND::NT_STR_SPLIT_REST_RE); };
        bool isStrNumSplitsRe() 		const { return (kind == NODE_KIND::NT_STR_NUM_SPLITS_RE); };
        bool isStrOp() 				const { return (isStrLen() || isStrConcat() || isStrSubstr() || isStrPrefixof() || isStrSuffixof() || isStrIndexof() || isStrCharat() || isStrUpdate() || isStrReplace() || isStrReplaceAll() || isStrToLower() || isStrToUpper() || isStrRev() || isStrSplit() || isStrSplitAt() || isStrSplitRest() || isStrNumSplits() || isStrSplitAtRe() || isStrSplitRestRe() || isStrNumSplitsRe() || (isUFApplication() && sort->isStr())); };

        // check strings comparison
        bool isStrLt() 				const { return (kind == NODE_KIND::NT_STR_LT); };
        bool isStrLe() 				const { return (kind == NODE_KIND::NT_STR_LE); };
        bool isStrGt() 				const { return (kind == NODE_KIND::NT_STR_GT); };
        bool isStrGe() 				const { return (kind == NODE_KIND::NT_STR_GE); };
        bool isStrEq() 				const { return (isEq() && getChildrenSize() >= 2 && (getChild(0)->isVStr() || getChild(0)->isCStr() || getChild(0)->isStrOp())); };
        bool isStrComp() 			const { return (isStrLt() || isStrLe() || isStrGt() || isStrGe() || isStrEq()); };

        // check strings properties
        bool isStrInReg() 			const { return (kind == NODE_KIND::NT_STR_IN_REG); };
        bool isStrContains() 		const { return (kind == NODE_KIND::NT_STR_CONTAINS); };
        bool isStrIsDigit() 		const { return (kind == NODE_KIND::NT_STR_IS_DIGIT); };
        bool isStrProp() 				const { return (isStrInReg() || isStrContains() || isStrIsDigit()); };
        bool isStrAtom() 				const { return isStrComp() || isStrProp(); }

        // check strings conversion
        bool isStrFromInt() 		const { return (kind == NODE_KIND::NT_STR_FROM_INT); };
        bool isStrToInt() 			const { return (kind == NODE_KIND::NT_STR_TO_INT); };
        bool isStrToReg() 			const { return (kind == NODE_KIND::NT_STR_TO_REG); };
        bool isStrToCode() 			const { return (kind == NODE_KIND::NT_STR_TO_CODE); };
        bool isStrFromCode() 		const { return (kind == NODE_KIND::NT_STR_FROM_CODE); };
        bool isStrConv() 			const { return (isStrFromInt() || isStrToInt() || isStrToReg() || isStrToCode() || isStrFromCode()); };

        // reg
        bool isRegNone() 			const { return (kind == NODE_KIND::NT_REG_NONE); };
        bool isRegAll() 			const { return (kind == NODE_KIND::NT_REG_ALL); };
        bool isRegAllChar() 		const { return (kind == NODE_KIND::NT_REG_ALLCHAR); };
        bool isRegConcat() 		    const { return (kind == NODE_KIND::NT_REG_CONCAT); };
        bool isRegUnion() 			const { return (kind == NODE_KIND::NT_REG_UNION); };
        bool isRegInter() 			const { return (kind == NODE_KIND::NT_REG_INTER); };
        bool isRegDiff() 			const { return (kind == NODE_KIND::NT_REG_DIFF); };
        bool isRegStar() 			const { return (kind == NODE_KIND::NT_REG_STAR); }; 
        bool isRegPlus() 			const { return (kind == NODE_KIND::NT_REG_PLUS); };
        bool isRegOpt() 			const { return (kind == NODE_KIND::NT_REG_OPT); };
        bool isRegRange() 			const { return (kind == NODE_KIND::NT_REG_RANGE); };
        bool isRegRepeat() 		    const { return (kind == NODE_KIND::NT_REG_REPEAT); };
        bool isRegLoop() 			const { return (kind == NODE_KIND::NT_REG_LOOP); };
        bool isRegComplement() 		const { return (kind == NODE_KIND::NT_REG_COMPLEMENT); };

        bool isAtom()				const {
            return isArithAtom() || isBVAtom() || isFPAtom() || isStrAtom() ||
                   (isUFApplication() && sort->isBool());
        }
        // check let
        bool isLet()				const { return kind == NODE_KIND::NT_LET; };
        bool isLetChain()			const { return kind == NODE_KIND::NT_LET_CHAIN; };
        bool isLetBindVarList()		const { return kind == NODE_KIND::NT_LET_BIND_VAR_LIST; };

        // check ite
        bool isIte()				const { return arenaKind() == somtarena::Kind::Ite; };

        // check function
        bool isFuncDec()            const { return (kind == NODE_KIND::NT_FUNC_DEC); };
        bool isFuncDef()			const { return (kind == NODE_KIND::NT_FUNC_DEF); };
        bool isFuncRec()			const { return (kind == NODE_KIND::NT_FUNC_REC); };
        bool isFuncParam()			const { return (kind == NODE_KIND::NT_FUNC_PARAM); };
        bool isFuncApplication()          const { return (kind == NODE_KIND::NT_FUNC_APPLY); };
        bool isFuncRecApplication()       const { return (kind == NODE_KIND::NT_FUNC_REC_APPLY); };


        // count the use of the node
        size_t getUseCount() const { return _use_count; };
        void incUseCount() { _use_count++; };
        void decUseCount() { _use_count--; };

        // get pure variable name for let bind var
        std::string getPureName()   const {
            return name;
        }

        std::string toString()      const { return getPureName(); };

        /// Get the BV bit width. Returns 0 if not a BV sort.
        size_t getBitWidth() const {
            auto s = getSort();
            return (s && s->isBv()) ? s->getBitWidth() : 0;
        }

        /// Extract raw string value from a string constant (strips SMT-LIB quotes, unescapes "").
        std::string getStringLiteral() const {
            std::string s = getName();
            if(s.size() >= 2 && s.front() == '"' && s.back() == '"') {
                s = s.substr(1, s.size() - 2);
                // Handle SMT-LIB escape: "" → "
                std::string result;
                result.reserve(s.size());
                for(size_t i = 0; i < s.size(); ++i) {
                    if(i + 1 < s.size() && s[i] == '"' && s[i+1] == '"') {
                        result += '"';
                        ++i;
                    } else {
                        result += s[i];
                    }
                }
                return result;
            }
            return s;
        }

        // other functions
        /**
         * @brief Get the sort of the node
         * 
         * @return The sort of the node
         */
        std::shared_ptr<Sort> getSort() const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.a): serve the sort from the shared SOMTArena term-IR via the parser-side
            // read registry when SOMTP_DAGNODE_ARENA_READS is on. Verdict-neutral: the registry holds
            // the SAME interned shared_ptr<Sort> the `sort` field holds (registered by the builder
            // from the authoritative field), so on == off exactly. sortOf() matches the node's owning
            // arena (arenaPtr_), so a stale handle into a discarded arena falls back to the field.
            // Also falls back for handle-less nodes (quantifier/let scaffolding, arenaExprId_==0), an
            // unregistered ExprId, or the flag off. The arena BUILDER must NOT use this path — it uses
            // getSortRaw() so the arena is always built from the authoritative field.
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto s = ArenaReadRegistry::instance().sortOf(arenaPtr_, arenaExprId_)) return s;
            }
#endif
            return sort;
        };
#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P3.a): the authoritative field sort, bypassing the arena read registry. Used by
        // the arena builder (src/arena/build.cpp), which is the SOURCE that populates the registry and
        // must never read back from it (a node may still hold a stale handle from a discarded build).
        std::shared_ptr<Sort> getSortRaw() const { return sort; }
#endif
        /**
         * @brief Get the name of the node
         *
         * @return The name of the node
         */
        std::string getName() const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.e): serve the name from the shared SOMTArena term-IR via the parser-side read
            // registry when SOMTP_DAGNODE_ARENA_READS is on (mirrors P3.a getSort / P3.b getValue).
            // Verdict-neutral: registerName stores the SAME field string, keyed by BOTH arena AND owner
            // (like children_, NOT sort_), so nameFor returns null for the let-forward alias — a let
            // node forwards a child's ExprId but its OWN name (bound var) differs from the child's — and
            // the let node falls back to its own `name` field. Also falls back for handle-less nodes
            // (arenaExprId_==0), an unregistered ExprId, a stale handle (Arena* mismatch), or flag off.
            // A NULL nameFor means "absent" (NOT the empty string — operators legitimately have name"");
            // the arena BUILDER must NOT use this path — it uses getNameRaw() (authoritative field).
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto* n = ArenaReadRegistry::instance().nameFor(arenaPtr_, arenaExprId_, this)) return *n;
            }
#endif
            return name;
        };
#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P3.e): the authoritative field name, bypassing the arena read registry. Used by the
        // arena builder (src/arena/build.cpp), which is the SOURCE that populates the registry and must
        // never read back from it. Mirrors getSortRaw/getValueRaw/getChildRaw.
        std::string getNameRaw() const { return name; }
#endif

        /**
         * @brief Get the re-named name of the node
         * 
         * @return The re-named name of the node
         */
        std::string rename(const std::string& new_name) {
            std::cerr << "Warning: rename " << name << " to " << new_name << std::endl;
            name = new_name;
            return new_name;
        }

        /**
         * @brief Get the kind of the node
         * 
         * @return The kind of the node
         */
#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P0): arena-handle accessors. setArenaHandle() is called by the buildArena walk for
        // each core node; arenaExprId()/arenaPtr() are read by the P2 façade (0 == not built into an
        // arena, e.g. transient let/match scaffolding or a not-yet-built node).
        // II-2b-3 (foundation): `finalized` marks an OWN-handle set (core / quantifier / singleton). The
        // two let-FORWARD sites omit it (default false) — a let node aliases a child's ExprId, not its
        // own — so finalized_ ends up meaning "owns its arena node", which arenaKind()'s assert checks.
        void setArenaHandle(const somtarena::Arena* a, std::uint64_t id, bool finalized = false) {
            arenaPtr_ = a; arenaExprId_ = id; finalized_ = finalized;
            g_liveArena = a;
        }
        std::uint64_t arenaExprId() const { return arenaExprId_; }
        const somtarena::Arena* arenaPtr() const { return arenaPtr_; }
#endif

        NODE_KIND getKind()           const { return kind; };

#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (foundation, UNWIRED): the future arena-backed source for getKind()/is*. When
        // SOMTP_DAGNODE_ARENA_READS is on AND we are past the front-end phase AND this node owns a
        // finalized arena handle, it reads the somtarena::Kind straight from the shared arena; otherwise
        // it derives the Kind from the NODE_KIND field via mapKind (verdict-identical). The g_frontend-
        // Phase guard is the field-net that makes the arena read crash-proof: during parse+import let
        // scaffolding forwards a CHILD's ExprId (arenaPtr_->kind would return the child's Kind ->
        // misclassification -> the negateComp OOB SIGSEGV the direct-read attempt hit), and the NRA
        // discard path leaves stale handles. NOTHING calls this yet — it is installed so the later is*
        // migration is crash-proof and the finalized_ assert traps, in CI, any site that reaches the
        // arena via a forwarded (non-final) let handle.
        somtarena::Kind arenaKind() const {
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            // II-2b-3 (reader-side): deref the shared arena's Kind ONLY when this node owns a FINALIZED
            // handle (finalized_ — not a let-forward alias whose arena Kind is the forwarded child's) AND
            // that handle points into the CURRENTLY-LIVE arena (arenaPtr_ == g_liveArena). A stale handle
            // into a discarded arena (NRA discard path) or a non-final let alias falls through to the field
            // mapKind — verdict-identical, no deref of freed memory. O(1) pointer compare.
            if (useArena && arenaExprId_ != 0 && arenaPtr_) {
                static const bool unguarded = [](){ const char* e = std::getenv("XOLVER_ARENAKIND_UNGUARDED");
                                                    return e && *e && *e != '0'; }();
                if (unguarded) return arenaPtr_->kind(arenaExprId_);
                if (finalized_ && arenaPtr_ == g_liveArena) return arenaPtr_->kind(arenaExprId_);
            }
            bool mapped = false; return xarena_cov::mapKind(kind, mapped);
        }
#endif

        /**
         * @brief Get the value of the node
         * 
         * @return The value of the node
         */
        std::shared_ptr<Value> getValue() const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.b): serve the value from the shared SOMTArena term-IR via the parser-side
            // read registry when SOMTP_DAGNODE_ARENA_READS is on (mirrors getSort, P3.a). Verdict-
            // neutral: the registry holds the SAME interned shared_ptr<Value> the `value` field holds
            // — populated by the builder from getValueRaw() at handle-set time, and kept in sync by
            // setValue() on any post-handle mutation. valueOf() matches the node's owning arena
            // (arenaPtr_), so a stale handle into a discarded arena falls back to the field; likewise
            // for handle-less nodes (arenaExprId_==0), an unregistered/null-valued ExprId, or flag
            // off. The arena BUILDER must NOT use this path — it reads getValueRaw() (authoritative
            // field) so the arena is always built from the correct value.
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto v = ArenaReadRegistry::instance().valueOf(arenaPtr_, arenaExprId_)) return v;
            }
#endif
            return value;
        };
#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P3.b): the authoritative field value, bypassing the arena read registry. Used by
        // the arena builder (src/arena/map.cpp mapValue, src/arena/build.cpp), which is the SOURCE
        // that populates the registry and must never read back from it (a node may still hold a stale
        // handle from a discarded build).
        std::shared_ptr<Value> getValueRaw() const { return value; }
#endif

        /**
         * @brief Set the value of the node
         *
         * @param v The value to set
         */
        void setValue(std::shared_ptr<Value> v) {
            value = v;
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.b): keep the read registry in sync with this mutation when the node already
            // carries an arena handle (a POST-handle setValue, e.g. mkConstBv/mkConstFp enriching a
            // const node's value after createNode fired the inline hook). Without this, getValue()
            // would return the value the builder captured at handle-set time, not the mutated field
            // -> a verdict divergence vs the flag-off path. When setValue runs BEFORE the handle is
            // set (construction), arenaExprId_==0 so this is skipped and the handle-set populate
            // captures the final field value. Either ordering keeps registry == field.
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0 && arenaPtr_) {
                ArenaReadRegistry::instance().registerValue(arenaPtr_, arenaExprId_, value);
            }
#endif
        };

        // Typed setValue overloads delegate to the shared_ptr overload above so the P3.b registry
        // sync lives in exactly one place (behaviorally identical to the prior `value = newValue(v)`).
        void setValue(const Integer& v) { setValue(newValue(v)); };

        void setValue(const Real& v) { setValue(newValue(v)); };

        void setValue(const double& v) { setValue(newValue(v)); };

        void setValue(const int& v) { setValue(newValue(v)); };

        void setValue(const Interval& v) { setValue(newValue(v)); };
        
        /**
         * @brief Get the number of children of the node
         *
         * @return The number of children of the node
         */
        size_t getChildrenSize() const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.c): serve the child count from the shared SOMTArena term-IR via the parser-
            // side read registry when SOMTP_DAGNODE_ARENA_READS is on. Verdict-neutral: childrenOf
            // returns the child ExprId list captured at build time (Apply-remapped to DAGNode normal
            // form), whose size == the `children` field size. Falls back to the field for handle-less
            // nodes (arenaExprId_==0), quantifier/leaf nodes (not registered), the let-forward alias
            // (owner mismatch), a stale handle into a discarded arena (Arena* mismatch), or flag off.
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto* cids = ArenaReadRegistry::instance().childrenOf(arenaPtr_, arenaExprId_, this))
                    return cids->size();
            }
#endif
            return children.size();
        };

        /**
         * @brief Get the children of the node
         *
         * @return The children of the node
         */
        std::vector<std::shared_ptr<DAGNode>> getChildren() const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.c): materialize the children from the arena read registry — each child
            // ExprId -> its canonical DAGNode via nodeFor. Whole-node field fallback: if the size
            // differs from the field, or ANY child ExprId isn't registered (nodeFor null), return the
            // field vector ENTIRELY (never a partial / nullptr-bearing list).
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto* cids = ArenaReadRegistry::instance().childrenOf(arenaPtr_, arenaExprId_, this)) {
                    if (cids->size() == children.size()) {
                        std::vector<std::shared_ptr<DAGNode>> res;
                        res.reserve(cids->size());
                        bool ok = true;
                        for (std::uint64_t cid : *cids) {
                            auto n = ArenaReadRegistry::instance().nodeFor(arenaPtr_, cid);
                            if (!n) { ok = false; break; }
                            res.push_back(std::move(n));
                        }
                        if (ok) return res;
                    }
                }
            }
#endif
            return children;
        };

        /**
         * @brief Get the child of the node
         *
         * @param i The index of the child
         * @return The child of the node
         */
        std::shared_ptr<DAGNode> getChild(int i) const {
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P3.c): serve child i from the arena read registry (its child ExprId -> DAGNode
            // via nodeFor). Whole-node field fallback: if the arena child count differs from the field,
            // i is out of range, or the child ExprId isn't registered (nodeFor null), read the FIELD
            // for the whole node (never a partial / nullptr child).
            static const bool useArena = [](){ const char* e = std::getenv("SOMTP_DAGNODE_ARENA_READS");
                                               return e && *e && *e != '0'; }();
            if (useArena && arenaExprId_ != 0) {
                if (auto* cids = ArenaReadRegistry::instance().childrenOf(arenaPtr_, arenaExprId_, this)) {
                    if (cids->size() == children.size() &&
                        i >= 0 && static_cast<size_t>(i) < cids->size()) {
                        if (auto n = ArenaReadRegistry::instance().nodeFor(arenaPtr_, (*cids)[i]))
                            return n;
                    }
                }
            }
#endif
            return children[i];
        };
#ifdef SOMTPARSER_WITH_ARENA
        // II-2b-3 (P3.c): field-only child accessors, bypassing the arena read registry. Used by the
        // arena builder (src/arena/build.cpp), which is the SOURCE that populates the registry and
        // must never read back from it (a node may still hold a stale handle from a discarded build).
        // Mirrors getSortRaw/getValueRaw.
        size_t getChildrenSizeRaw() const { return children.size(); }
        const std::vector<std::shared_ptr<DAGNode>>& getChildrenRaw() const { return children; }
        std::shared_ptr<DAGNode> getChildRaw(int i) const { return children[i]; }
#endif
        // NOTE: function body is the first child

        /**
         * @brief Get the body of the function
         * 
         * @return The body of the function
         */
        std::shared_ptr<DAGNode> getFuncBody() 
                                    const { return children[0]; };

        /**
         * @brief Get the parameters of the function
         * 
         * @return The parameters of the function
         */
        std::vector<std::shared_ptr<DAGNode>> getFuncParams() const{
            std::vector<std::shared_ptr<DAGNode>> res;
            for(size_t i = 1;i<getChildrenSize();i++){
                res.emplace_back(getChild(i));
            }
            return res;
        }

        /**
         * @brief Get the number of parameters of the function
         * 
         * @return The number of parameters of the function
         */
        size_t getFuncParamsSize() const{
            return getChildrenSize() - 1;
        }

        // get quant body
        /**
         * @brief Get the body of the quantifier
         * 
         * @return The body of the quantifier
         */
        std::shared_ptr<DAGNode> getQuantBody() const { return children[0]; };

        /**
         * @brief Get the variables of the quantifier
         * 
         * @return The variables of the quantifier
         */
        std::vector<std::shared_ptr<DAGNode>> getQuantVars() const{
            std::vector<std::shared_ptr<DAGNode>> res;
            for(size_t i = 1;i<getChildrenSize();i++){
                res.emplace_back(getChild(i));
            }
            return res;
        }

        // Array operations helper functions
        /**
         * @brief Get the array operand of a store node
         * 
         * @note For store(array, index, value), returns array
         * @return The array operand
         */
        std::shared_ptr<DAGNode> getStoreArray() const {
            condAssert(isStore() && getChildrenSize() >= 3, "getStoreArray: node is not a store or has insufficient children");
            return getChild(0);
        }

        /**
         * @brief Get the index operand of a store node
         * 
         * @note For store(array, index, value), returns index
         * @return The index operand
         */
        std::shared_ptr<DAGNode> getStoreIndex() const {
            condAssert(isStore() && getChildrenSize() >= 3, "getStoreIndex: node is not a store or has insufficient children");
            return getChild(1);
        }

        /**
         * @brief Get the value operand of a store node
         * 
         * @note For store(array, index, value), returns value
         * @return The value operand
         */
        std::shared_ptr<DAGNode> getStoreValue() const {
            condAssert(isStore() && getChildrenSize() >= 3, "getStoreValue: node is not a store or has insufficient children");
            return getChild(2);
        }

        /**
         * @brief Get the array operand of a select node
         * 
         * @note For select(array, index), returns array
         * @return The array operand
         */
        std::shared_ptr<DAGNode> getSelectArray() const {
            condAssert(isSelect() && getChildrenSize() >= 2, "getSelectArray: node is not a select or has insufficient children");
            return getChild(0);
        }

        /**
         * @brief Get the index operand of a select node
         * 
         * @note For select(array, index), returns index
         * @return The index operand
         */
        std::shared_ptr<DAGNode> getSelectIndex() const {
            condAssert(isSelect() && getChildrenSize() >= 2, "getSelectIndex: node is not a select or has insufficient children");
            return getChild(1);
        }

        // is really equal to another node
        /**
         * @brief Check if the node is equivalent to another node
         * 
         * @param other The other node
         * @return True if the node is equivalent to the other node, false otherwise
         */
        bool isEquivalentTo(const std::shared_ptr<DAGNode>& other) const {
            return isEquivalentTo(*other);
        }

        /**
         * @brief Check if the node is equivalent to another node
         * 
         * @param other The other node
         * @return True if the node is equivalent to the other node, false otherwise
         */
        bool isEquivalentTo(const DAGNode& other) const {
            std::unordered_set<std::pair<const DAGNode*, const DAGNode*>, PairNodePtrHash, PairNodePtrEqual> visited;
            return isEquivalentTo(other, visited);
        }

        /**
         * @brief Equivalence check with THIS node's kind + sort + name threaded in.
         *
         * II-2b-3 (E3-dedup / step2): used by the hash-cons dedup path where THIS is the freshly-built
         * candidate (called as candidate->isEquivalentTo(*bucketNode, candidateKind, candidateSort,
         * candidateName)). The candidate has no arena handle yet, so its kind/sort/name are threaded
         * from createNode instead of read from the this->kind/sort/name fields. thisKind/thisSort/
         * thisName == this->kind/sort/name for the candidate, so the boolean result is byte-identical
         * to isEquivalentTo(other) — only the SOURCE of the candidate's kind/sort/name changes.
         * `other` (the pre-existing bucket node) still reads other's fields (field-drop is a later step).
         */
        bool isEquivalentTo(const DAGNode& other, NODE_KIND thisKind,
                const std::shared_ptr<Sort>& thisSort, const std::string& thisName) const {
            std::unordered_set<std::pair<const DAGNode*, const DAGNode*>, PairNodePtrHash, PairNodePtrEqual> visited;
            return isEquivalentTo(other, thisKind, thisSort, thisName, visited);
        }

        /**
         * @brief Get the hash code of the node
         *
         * @return The hash code of the node
         */
        std::size_t hashCode() const{
            // II-2b-3 (E3-dedup / step2): the normal path sources kind/sort/name from the fields; identical value.
            return hashCode(kind, sort, name);
        }

        /**
         * @brief Get the hash code of the node with its kind + sort + name THREADED in.
         *
         * II-2b-3 (E3-dedup / step2): used by the hash-cons dedup path (insertNodeToBucket) where the
         * freshly-built candidate has no arena handle yet, so its kind/sort/name cannot be read back
         * from the arena. threadedKind/threadedSort/threadedName == this->kind/sort/name for the
         * candidate, so the returned hash is byte-identical to hashCode() — only the SOURCE of h1
         * (sort mix), h2 (kind mix) and h3 (name mix) changes (the params, not the this-> fields).
         */
        std::size_t hashCode(NODE_KIND threadedKind, const std::shared_ptr<Sort>& threadedSort,
                const std::string& threadedName) const{
            if(hash_computed) {
                return cached_hash_code;
            }

            // high quality hash algorithm, reduce conflicts
            size_t h1 = std::hash<std::string>{}(threadedSort->toString());
            size_t h2 = static_cast<size_t>(threadedKind);
            size_t h3 = threadedName.empty() ? 0 : std::hash<std::string>{}(threadedName);
            size_t h4 = children.size();
            size_t h5 = children_hash.empty() ? 0 : std::hash<std::string>{}(children_hash);
            
            // use better hash combination algorithm (based on boost::hash_combine)
            size_t seed = 0;
            seed ^= h1 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h4 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= h5 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            
            seed ^= 0x9e3779b9 + (seed << 6) + (seed >> 2);
            
            cached_hash_code = seed;
            hash_computed = true;
            return cached_hash_code;
        }

        /**
         * @brief Update the function definition
         * 
         * @note This function is only used to update the function definition.
         * 
         * @param out_sort The output sort
         * @param body The body of the function
         * @param params The parameters of the function
         */
        void updateFuncDef(std::shared_ptr<Sort> out_sort, std::shared_ptr<DAGNode> body, const std::vector<std::shared_ptr<DAGNode>> &params, bool is_rec = false);

        /**
         * @brief Update the function application
         *
         * @note This function is only used to update the function application.
         * 
         * @param out_sort The output sort
         * @param body The body of the function
         * @param params The parameters of the function
         */
        void updateApplyFunc(std::shared_ptr<Sort> out_sort, std::shared_ptr<DAGNode> body, const std::vector<std::shared_ptr<DAGNode>> &params, bool is_rec = false);

    private:
        bool isEquivalentTo(const DAGNode& other, std::unordered_set<std::pair<const DAGNode*, const DAGNode*>, PairNodePtrHash, PairNodePtrEqual>& visited) const {
            TIME_FUNC();
            
            // fastest check: pointer same
            if (this == &other) {
                return true;
            }
            
            // fast structure check (avoid the expensive subsequent comparison)
            if (kind != other.kind || 
                children.size() != other.children.size() || 
                sort.get() != other.sort.get()) {
                return false;
            }
            
            // name check
            if (name != other.name) {
                return false;
            }
            
            // children_hash check (if both are calculated, this is the fastest deep comparison)
            if (!children_hash.empty() && !other.children_hash.empty() && 
                children_hash != other.children_hash) {
                return false;
            }
            
            auto p = std::make_pair(this, &other);
            if(visited.find(p) != visited.end()){
                return true;
            }
            visited.insert(p);
            
            // most expensive recursive comparison at the end
            for (size_t i = 0; i < children.size(); i++) {
                if (!children[i]->isEquivalentTo(*other.children[i], visited)) {
                    return false;
                }
            }
            return true;
        }

        // II-2b-3 (E3-dedup / step2): recursive equivalence where THIS node's (top-level) kind + sort +
        // name are threaded in via `thisKind`/`thisSort`/`thisName` instead of read from the this->
        // kind/sort/name fields. Mirrors the body above exactly, save for the threaded structural
        // checks (`thisKind != other.kind`, `thisSort.get() != other.sort.get()`, `thisName != other.name`).
        // Children recurse through the normal (field-sourced) 2-arg path — they are pre-existing nodes
        // whose fields are valid. For the candidate, thisKind/thisSort/thisName == this->kind/sort/name,
        // so the result is byte-identical to the 2-arg form.
        bool isEquivalentTo(const DAGNode& other, NODE_KIND thisKind,
                const std::shared_ptr<Sort>& thisSort, const std::string& thisName,
                std::unordered_set<std::pair<const DAGNode*, const DAGNode*>, PairNodePtrHash, PairNodePtrEqual>& visited) const {
            TIME_FUNC();

            // fastest check: pointer same
            if (this == &other) {
                return true;
            }

            // fast structure check (avoid the expensive subsequent comparison)
            if (thisKind != other.kind ||
                children.size() != other.children.size() ||
                thisSort.get() != other.sort.get()) {
                return false;
            }

            // name check
            if (thisName != other.name) {
                return false;
            }

            // children_hash check (if both are calculated, this is the fastest deep comparison)
            if (!children_hash.empty() && !other.children_hash.empty() &&
                children_hash != other.children_hash) {
                return false;
            }

            auto p = std::make_pair(this, &other);
            if(visited.find(p) != visited.end()){
                return true;
            }
            visited.insert(p);

            // most expensive recursive comparison at the end
            for (size_t i = 0; i < children.size(); i++) {
                if (!children[i]->isEquivalentTo(*other.children[i], visited)) {
                    return false;
                }
            }
            return true;
        }
    };

    struct NodeHash {
        size_t operator()(const std::shared_ptr<DAGNode>& node) const {
            return node->hashCode();  // directly use the cached hash code
        }
    };

    struct NodeEqual {
        bool operator()(const std::shared_ptr<DAGNode>& node1, const std::shared_ptr<DAGNode>& node2) const {
            // fast path: first compare hash code
            if(node1->hashCode() != node2->hashCode()) {
                return false;
            }
            // only check the expensive equivalence when the hash is the same
            return node1->isEquivalentTo(*node2);
        }
    };

    std::string dumpSMTLIB2(const std::shared_ptr<DAGNode>& node);
    std::string dumpFuncDef(const std::shared_ptr<DAGNode>& node);
    std::string dumpFuncRec(const std::shared_ptr<DAGNode>& node);
    std::string dumpFuncDec(const std::shared_ptr<DAGNode>& node);
    
    // smart pointer
    typedef std::shared_ptr<DAGNode> NodePtr;

    class NodeManager{
        private:
            std::vector<std::shared_ptr<DAGNode>> nodes;
            // use secondary hash: Kind -> Hash -> NodeIndex
            std::array<std::unordered_map<size_t, std::vector<std::pair<std::shared_ptr<DAGNode>, size_t>>>, NUM_KINDS> node_buckets;
            // Track the number of static constant nodes to preserve them during clear()
            size_t static_node_count = 0;
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P1): inline arena-builder hook. The arena bridge registers a closure that
            // builds the SOMTArena node for each NEW DAGNode (children already carry handles, since
            // construction is bottom-up) and caches it via node->setArenaHandle. Type-erased so the
            // IR module stays free of somtarena. Empty (the default) = inactive -> zero behavior change.
            // II-2b-3 (E3 step4a): the candidate's NODE_KIND is threaded in as the 2nd arg so the hook
            // need not read the handle-less candidate's kind field (arenaExprId_==0 at hook time — the
            // hook is what sets the handle). insertNodeToBucket forwards its candidateKind param.
            // II-2b-3 (endgame step3): the candidate's sort/name/value are threaded too (args 3/4/5 ==
            // its createNode sort/name/value) so the builder's field path reads none of the candidate's
            // sort/name/value FIELDS. insertNodeToBucket forwards candidateSort/candidateName/candidateValue.
            std::function<void(const std::shared_ptr<DAGNode>&, NODE_KIND,
                               const std::shared_ptr<Sort>&, const std::string&,
                               const std::shared_ptr<Value>&)> arenaBuilderHook_;
#endif
        public:
            NodeManager();
#ifdef SOMTPARSER_WITH_ARENA
            // II-2b-3 (P1): register/clear the inline arena-builder hook. The bridge calls this once an
            // arena is set up; an empty function disables inline building (back to the default path).
            void setArenaBuilderHook(std::function<void(const std::shared_ptr<DAGNode>&, NODE_KIND,
                                                        const std::shared_ptr<Sort>&, const std::string&,
                                                        const std::shared_ptr<Value>&)> h) { arenaBuilderHook_ = std::move(h); }
            bool hasArenaBuilderHook() const { return static_cast<bool>(arenaBuilderHook_); }
#endif
            ~NodeManager();
            std::shared_ptr<DAGNode> getNode(const size_t index) const;
            size_t getIndex(const std::shared_ptr<DAGNode>& node) const;
            size_t size() const;

            // createNode methods corresponding to DAGNode constructors
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, NODE_KIND kind, std::string name, std::vector<std::shared_ptr<DAGNode>> children);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, NODE_KIND kind, std::string name);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, NODE_KIND kind);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort);
            std::shared_ptr<DAGNode> createNode();
            std::shared_ptr<DAGNode> createNode(NODE_KIND kind, std::string name);
            std::shared_ptr<DAGNode> createNode(NODE_KIND kind);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, const Integer& v);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, const Real& v);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, const double& v);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, const int& v);
            std::shared_ptr<DAGNode> createNode(std::shared_ptr<Sort> sort, const bool& v);
            std::shared_ptr<DAGNode> createNode(const std::string& n);
            
            void clear();
            
            // Getter functions for static constant nodes
            static std::shared_ptr<DAGNode> getNull() { return NULL_NODE; }
            static std::shared_ptr<DAGNode> getUnknown() { return UNKNOWN_NODE; }
            static std::shared_ptr<DAGNode> getTrue() { return TRUE_NODE; }
            static std::shared_ptr<DAGNode> getFalse() { return FALSE_NODE; }
            static std::shared_ptr<DAGNode> getE() { return E_NODE; }
            static std::shared_ptr<DAGNode> getPi() { return PI_NODE; }
            static std::shared_ptr<DAGNode> getNaN() { return NAN_NODE; }
            static std::shared_ptr<DAGNode> getEpsilon() { return EPSILON_NODE; }
            static std::shared_ptr<DAGNode> getPosEpsilon() { return POS_EPSILON_NODE; }
            static std::shared_ptr<DAGNode> getNegEpsilon() { return NEG_EPSILON_NODE; }
            
            // Infinity getters
            static std::shared_ptr<DAGNode> getStrInf() { return STR_INF_NODE; }
            static std::shared_ptr<DAGNode> getStrPosInf() { return STR_POS_INF_NODE; }
            static std::shared_ptr<DAGNode> getStrNegInf() { return STR_NEG_INF_NODE; }
            static std::shared_ptr<DAGNode> getIntInf() { return INT_INF_NODE; }
            static std::shared_ptr<DAGNode> getIntPosInf() { return INT_POS_INF_NODE; }
            static std::shared_ptr<DAGNode> getIntNegInf() { return INT_NEG_INF_NODE; }
            static std::shared_ptr<DAGNode> getRealInf() { return REAL_INF_NODE; }
            static std::shared_ptr<DAGNode> getRealPosInf() { return REAL_POS_INF_NODE; }
            static std::shared_ptr<DAGNode> getRealNegInf() { return REAL_NEG_INF_NODE; }
        public:
            // static constant nodes (inline for guaranteed initialization order)
            inline static const std::shared_ptr<DAGNode> NULL_NODE = std::make_shared<DAGNode>("NULL");
            inline static const std::shared_ptr<DAGNode> UNKNOWN_NODE = std::make_shared<DAGNode>(SortManager::UNKNOWN_SORT, NODE_KIND::NT_UNKNOWN, "unknown");
            inline static const std::shared_ptr<DAGNode> ERROR_NODE = std::make_shared<DAGNode>(SortManager::NULL_SORT, NODE_KIND::NT_ERROR, "error");
            inline static const std::shared_ptr<DAGNode> TRUE_NODE = std::make_shared<DAGNode>("true");
            inline static const std::shared_ptr<DAGNode> FALSE_NODE = std::make_shared<DAGNode>("false");
            inline static const std::shared_ptr<DAGNode> E_NODE = std::make_shared<DAGNode>("e");
            inline static const std::shared_ptr<DAGNode> PI_NODE = std::make_shared<DAGNode>("pi");
            // inline static const std::shared_ptr<DAGNode> INF_NODE = std::make_shared<DAGNode>(SortManager::EXT_SORT, NODE_KIND::NT_INFINITY, "INF");
            // inline static const std::shared_ptr<DAGNode> POS_INF_NODE = std::make_shared<DAGNode>(SortManager::EXT_SORT, NODE_KIND::NT_POS_INFINITY, "+INF");
            // inline static const std::shared_ptr<DAGNode> NEG_INF_NODE = std::make_shared<DAGNode>(SortManager::EXT_SORT, NODE_KIND::NT_NEG_INFINITY, "-INF");
            inline static const std::shared_ptr<DAGNode> NAN_NODE = std::make_shared<DAGNode>("NaN");
            inline static const std::shared_ptr<DAGNode> EPSILON_NODE = std::make_shared<DAGNode>("EPSILON");
            inline static const std::shared_ptr<DAGNode> POS_EPSILON_NODE = std::make_shared<DAGNode>(SortManager::EXT_SORT, NODE_KIND::NT_POS_EPSILON, "+EPSILON");
            inline static const std::shared_ptr<DAGNode> NEG_EPSILON_NODE = std::make_shared<DAGNode>(SortManager::EXT_SORT, NODE_KIND::NT_NEG_EPSILON, "-EPSILON");
            
            // infinity
            inline static const std::shared_ptr<DAGNode> STR_INF_NODE = std::make_shared<DAGNode>(SortManager::STR_SORT, NODE_KIND::NT_INFINITY, "INF");
            inline static const std::shared_ptr<DAGNode> STR_POS_INF_NODE = std::make_shared<DAGNode>(SortManager::STR_SORT, NODE_KIND::NT_POS_INFINITY, "+INF");
            inline static const std::shared_ptr<DAGNode> STR_NEG_INF_NODE = std::make_shared<DAGNode>(SortManager::STR_SORT, NODE_KIND::NT_NEG_INFINITY, "-INF");
            inline static const std::shared_ptr<DAGNode> INT_INF_NODE = std::make_shared<DAGNode>(SortManager::INT_SORT, NODE_KIND::NT_INFINITY, "INF");
            inline static const std::shared_ptr<DAGNode> INT_POS_INF_NODE = std::make_shared<DAGNode>(SortManager::INT_SORT, NODE_KIND::NT_POS_INFINITY, "+INF");
            inline static const std::shared_ptr<DAGNode> INT_NEG_INF_NODE = std::make_shared<DAGNode>(SortManager::INT_SORT, NODE_KIND::NT_NEG_INFINITY, "-INF");
            inline static const std::shared_ptr<DAGNode> REAL_INF_NODE = std::make_shared<DAGNode>(SortManager::REAL_SORT, NODE_KIND::NT_INFINITY, "INF");
            inline static const std::shared_ptr<DAGNode> REAL_POS_INF_NODE = std::make_shared<DAGNode>(SortManager::REAL_SORT, NODE_KIND::NT_POS_INFINITY, "+INF");
            inline static const std::shared_ptr<DAGNode> REAL_NEG_INF_NODE = std::make_shared<DAGNode>(SortManager::REAL_SORT, NODE_KIND::NT_NEG_INFINITY, "-INF");
        private:
            void initializeStaticNodes();
            // II-2b-3 (E3-dedup / step2): candidateKind + candidateSort + candidateName are threaded from
            // createNode so the hash-cons dedup never reads the freshly-built candidate's kind/sort/name
            // fields (it has no arena handle yet).
            // II-2b-3 (endgame step3): candidateValue is threaded too — the dedup does NOT hash value, but
            // the arena-builder hook needs it (Const payload) so buildCoreNode's field path stays value-
            // field-free; insertNodeToBucket only forwards it to the hook.
            std::shared_ptr<DAGNode> insertNodeToBucket(const std::shared_ptr<DAGNode>& node, NODE_KIND candidateKind,
                const std::shared_ptr<Sort>& candidateSort, const std::string& candidateName,
                const std::shared_ptr<Value>& candidateValue);
    };

}
#endif