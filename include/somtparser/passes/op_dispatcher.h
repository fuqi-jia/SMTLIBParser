/* -*- Header -*-
 *
 * OpDispatcher: V1 hot-path kind-based dispatch (array + function pointer)
 * Core: dispatch(Node, Context&). Optional dispatch(Node) only for base Context.
 * Registration is fluent. Sugar APIs exist to avoid exposing NODE_KIND in user code.
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

#ifndef OP_DISPATCHER_HEADER
#define OP_DISPATCHER_HEADER

#include "somtparser/ir/node.h"
#include "somtparser/core/kind.h"
#include "somtparser/context/context.h"

#include <array>
#include <cassert>
#include <cstddef>
#include <type_traits>

namespace SOMTParser {

inline constexpr std::size_t to_index(NODE_KIND k) noexcept {
    return static_cast<std::size_t>(k);
}

/**
 * Kind-based O(1) dispatcher: array index + one branch, no map/virtual.
 *
 * - Core: dispatch(Node, Context&). Handlers take (Node, Context&).
 * - Optional: dispatch(Node) only when Context is SOMTParser::Context; uses NullContext.
 * - Registration is fluent: on(...) and otherwise(...) return *this.
 * - Sugar APIs (onAND, onADD, onGT, ... one per oper_key_map kind) avoid exposing NODE_KIND.
 */
template <class Result, class Context>
class OpDispatcher {
public:
    using Handler = Result (*)(Node, Context&);

    constexpr OpDispatcher() noexcept : table_{}, has_{}, fallback_{nullptr} {}

    /** Register handler for kind k; returns *this for fluent chaining. */
    constexpr OpDispatcher& on(NODE_KIND k, Handler h) noexcept {
        const auto i = to_index(k);
        if (i < NUM_KINDS) {
            table_[i] = h;
            has_[i] = (h != nullptr);
        }
        return *this;
    }

    /** Register fallback handler; returns *this for fluent chaining. */
    constexpr OpDispatcher& otherwise(Handler h) noexcept {
        fallback_ = h;
        return *this;
    }

    constexpr bool has(NODE_KIND k) const noexcept {
        const auto i = to_index(k);
        return i < NUM_KINDS && has_[i];
    }

    /**
     * Core: dispatch with context. O(1): array index + one branch.
     * If no handler and no fallback: debug assert; release: return Result{} or return (void).
     */
    inline Result dispatch(Node n, Context& ctx) const {
        const NODE_KIND k = kind(n);
        const auto i = to_index(k);

        if (i < NUM_KINDS && has_[i]) {
            return table_[i](n, ctx);
        }

        if (fallback_) {
            return fallback_(n, ctx);
        }

        assert(false && "OpDispatcher: unhandled NODE_KIND");
        if constexpr (!std::is_void_v<Result>) {
            return Result{};
        } else {
            return;
        }
    }

    /**
     * Convenience: dispatch without context. Uses NullContext (bound as Context&).
     * Enabled only when Context is SOMTParser::Context.
     */
    template <typename C = Context>
    std::enable_if_t<std::is_same_v<C, ::SOMTParser::Context>, Result> dispatch(Node n) const {
        NullContext null_ctx;
        return dispatch(n, null_ctx);
    }

    // --- Sugar APIs: one per kind in oper_key_map; hide NODE_KIND; return *this for fluent ---
    // Macro: OP_ON(K) -> on##K(Handler) forwards to on(NODE_KIND::NT_##K, h).
#define OP_ON(K) \
    constexpr OpDispatcher& on##K(Handler h) noexcept { return on(NODE_KIND::NT_##K, h); }
    // Boolean
    OP_ON(AND) OP_ON(OR) OP_ON(NOT) OP_ON(IMPLIES) OP_ON(XOR)
    // Core
    OP_ON(EQ) OP_ON(DISTINCT) OP_ON(ITE)
    // Arithmetic
    OP_ON(ADD) OP_ON(SUB) OP_ON(MUL) OP_ON(IAND) OP_ON(POW2) OP_ON(POW)
    OP_ON(DIV_INT) OP_ON(DIV_REAL) OP_ON(MOD) OP_ON(ABS) OP_ON(SQRT) OP_ON(SAFESQRT)
    OP_ON(CEIL) OP_ON(FLOOR) OP_ON(ROUND)
    OP_ON(EXP) OP_ON(LN) OP_ON(LG) OP_ON(LB) OP_ON(LOG)
    OP_ON(SIN) OP_ON(COS) OP_ON(TAN) OP_ON(COT) OP_ON(SEC) OP_ON(CSC)
    OP_ON(ASIN) OP_ON(ACOS) OP_ON(ATAN) OP_ON(ACOT) OP_ON(ASEC) OP_ON(ACSC)
    OP_ON(ATAN2) OP_ON(SINH) OP_ON(COSH) OP_ON(TANH) OP_ON(SECH) OP_ON(CSCH) OP_ON(COTH)
    OP_ON(ASINH) OP_ON(ACOSH) OP_ON(ATANH) OP_ON(ASECH) OP_ON(ACSCH) OP_ON(ACOTH)
    OP_ON(LE) OP_ON(LT) OP_ON(GE) OP_ON(GT)
    OP_ON(TO_REAL) OP_ON(TO_INT)
    OP_ON(IS_INT) OP_ON(IS_DIVISIBLE) OP_ON(IS_PRIME) OP_ON(IS_EVEN) OP_ON(IS_ODD)
    OP_ON(GCD) OP_ON(LCM) OP_ON(FACT)
    // Bitvector
    OP_ON(BV_NOT) OP_ON(BV_NEG) OP_ON(BV_AND) OP_ON(BV_OR) OP_ON(BV_XOR)
    OP_ON(BV_NAND) OP_ON(BV_NOR) OP_ON(BV_XNOR) OP_ON(BV_COMP)
    OP_ON(BV_ADD) OP_ON(BV_SUB) OP_ON(BV_MUL) OP_ON(BV_UDIV) OP_ON(BV_UREM)
    OP_ON(BV_SDIV) OP_ON(BV_SREM) OP_ON(BV_UMOD) OP_ON(BV_SMOD)
    OP_ON(BV_SHL) OP_ON(BV_LSHR) OP_ON(BV_ASHR)
    OP_ON(BV_ULT) OP_ON(BV_ULE) OP_ON(BV_UGT) OP_ON(BV_UGE)
    OP_ON(BV_SLT) OP_ON(BV_SLE) OP_ON(BV_SGT) OP_ON(BV_SGE)
    OP_ON(BV_CONCAT) OP_ON(BV_TO_NAT) OP_ON(NAT_TO_BV) OP_ON(BV_TO_INT) OP_ON(INT_TO_BV)
    OP_ON(BV_EXTRACT) OP_ON(BV_REPEAT) OP_ON(BV_ZERO_EXT) OP_ON(BV_SIGN_EXT)
    OP_ON(BV_ROTATE_LEFT) OP_ON(BV_ROTATE_RIGHT)
    // Floating point
    OP_ON(FP_ABS) OP_ON(FP_NEG) OP_ON(FP_ADD) OP_ON(FP_SUB) OP_ON(FP_MUL) OP_ON(FP_DIV)
    OP_ON(FP_FMA) OP_ON(FP_SQRT) OP_ON(FP_REM) OP_ON(FP_ROUND_TO_INTEGRAL)
    OP_ON(FP_MIN) OP_ON(FP_MAX) OP_ON(FP_LE) OP_ON(FP_LT) OP_ON(FP_GE) OP_ON(FP_GT) OP_ON(FP_EQ)
    OP_ON(FP_TO_UBV) OP_ON(FP_TO_SBV) OP_ON(FP_TO_REAL) OP_ON(FP_TO_FP) OP_ON(FP_TO_FP_UNSIGNED)
    OP_ON(FP_IS_NORMAL) OP_ON(FP_IS_SUBNORMAL) OP_ON(FP_IS_ZERO) OP_ON(FP_IS_INF) OP_ON(FP_IS_NAN)
    OP_ON(FP_IS_NEG) OP_ON(FP_IS_POS)
    // Array
    OP_ON(SELECT) OP_ON(STORE)
    // String
    OP_ON(STR_LEN) OP_ON(STR_CONCAT) OP_ON(STR_SUBSTR) OP_ON(STR_PREFIXOF) OP_ON(STR_SUFFIXOF)
    OP_ON(STR_INDEXOF) OP_ON(STR_CHARAT) OP_ON(STR_UPDATE) OP_ON(STR_REPLACE) OP_ON(STR_REPLACE_ALL)
    OP_ON(STR_REPLACE_REG) OP_ON(STR_REPLACE_REG_ALL) OP_ON(STR_INDEXOF_REG)
    OP_ON(STR_TO_LOWER) OP_ON(STR_TO_UPPER) OP_ON(STR_REV)
    OP_ON(STR_SPLIT) OP_ON(STR_SPLIT_AT) OP_ON(STR_SPLIT_REST) OP_ON(STR_NUM_SPLITS)
    OP_ON(STR_SPLIT_AT_RE) OP_ON(STR_SPLIT_REST_RE) OP_ON(STR_NUM_SPLITS_RE)
    OP_ON(STR_LT) OP_ON(STR_LE) OP_ON(STR_GT) OP_ON(STR_GE)
    OP_ON(STR_IN_REG) OP_ON(STR_CONTAINS) OP_ON(STR_IS_DIGIT)
    OP_ON(STR_FROM_INT) OP_ON(STR_TO_INT) OP_ON(STR_TO_REG) OP_ON(STR_TO_CODE) OP_ON(STR_FROM_CODE)
    // Regular expression
    OP_ON(REG_CONCAT) OP_ON(REG_UNION) OP_ON(REG_INTER) OP_ON(REG_DIFF)
    OP_ON(REG_STAR) OP_ON(REG_PLUS) OP_ON(REG_OPT) OP_ON(REG_RANGE) OP_ON(REG_REPEAT)
    OP_ON(REG_LOOP) OP_ON(REG_COMPLEMENT)
    // Root
    OP_ON(ROOT_OBJ) OP_ON(ROOT_OF_WITH_INTERVAL)
#undef OP_ON

private:
    static constexpr std::size_t N = NUM_KINDS;
    std::array<Handler, N> table_;
    std::array<bool, N> has_;
    Handler fallback_;
};

} // namespace SOMTParser

#endif
