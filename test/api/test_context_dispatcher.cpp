/**
 * Test Context + OpDispatcher: get* delegation, context(), dispatch(Node, Context&), dispatch(Node),
 * and a recursive tree visitor (max-depth) using fluent + sugar API.
 */
#include "somtparser/frontend/parser.h"
#include "somtparser/passes/op_dispatcher.h"
#include <algorithm>
#include <iostream>
#include "test_helpers.h"

using namespace SOMTParser;

// --- Simple handlers for basic dispatch tests ---
static int handler_gt(Node n, Context&) {
    (void)n;
    return 1;
}

static int handler_fallback(Node, Context&) {
    return -1;
}

// --- Recursive visitor: max depth of expression tree ---
// Context that carries the dispatcher so handlers can recurse.
struct DepthVisitorContext : Context {
    OpDispatcher<int, Context>* disp = nullptr;
};

static int depth_fallback(Node n, Context& ctx) {
    DepthVisitorContext& vc = static_cast<DepthVisitorContext&>(ctx);
    int d = 0;
    for (Node c : children(n)) {
        if (c)
            d = std::max(d, vc.disp->dispatch(c, ctx));
    }
    return 1 + d;
}

int main() {
    std::cout << "======= Context & OpDispatcher Test =======" << std::endl;

    Parser parser;
    parser.mkVarInt("x");
    bool ok = parser.parseStr("(assert (> x 0))");
    VERIFY(ok);

    // get* delegate to ParserContext; need ParserContext& for get* via context()
    auto from_get = parser.getAssertions();
    auto from_ctx = static_cast<ParserContext&>(parser.context()).getAssertions();
    VERIFY(from_get.size() == from_ctx.size() && from_get.size() == 1);
    std::cout << "getAssertions() == static_cast<ParserContext&>(context()).getAssertions() OK" << std::endl;

    Node root = from_get[0];
    VERIFY(kind(root) == NODE_KIND::NT_GT);

    // OpDispatcher with Context& (parser.context()) — existing API
    OpDispatcher<int, Context> disp;
    disp.on(NODE_KIND::NT_GT, handler_gt);
    disp.otherwise(handler_fallback);

    int r1 = disp.dispatch(root, parser.context());
    VERIFY(r1 == 1);
    std::cout << "dispatch(node, parser.context()) OK" << std::endl;

    // Convenience: dispatch(Node) uses NullContext
    int r2 = disp.dispatch(root);
    VERIFY(r2 == 1);
    std::cout << "dispatch(node) with NullContext OK" << std::endl;

    // Fluent + sugar API
    OpDispatcher<int, Context> disp2;
    disp2.onGT(handler_gt).otherwise(handler_fallback);
    int r3 = disp2.dispatch(root, parser.context());
    int r4 = disp2.dispatch(root);
    VERIFY(r3 == 1 && r4 == 1);
    std::cout << "fluent + sugar (onGT/otherwise) and dispatch(node)/dispatch(node, context()) OK" << std::endl;

    // --- Recursive max-depth visitor (fluent + sugar, context carries dispatcher) ---
    parser.parseStr("(assert (and (> x 0) (< x 10)))");
    Node root2 = parser.getAssertions().back();
    VERIFY(kind(root2) == NODE_KIND::NT_AND);

    OpDispatcher<int, Context> depth_disp;
    depth_disp
        .onAND(depth_fallback)
        .onOR(depth_fallback)
        .onGT(depth_fallback)
        .onLT(depth_fallback)
        .onGE(depth_fallback)
        .onLE(depth_fallback)
        .onADD(depth_fallback)
        .onNOT(depth_fallback)
        .otherwise(depth_fallback);

    DepthVisitorContext depth_ctx;
    depth_ctx.disp = &depth_disp;

    int depth = depth_disp.dispatch(root2, depth_ctx);
    // Tree: AND(GT(x,0), LT(x,10)) -> AND depth 1 + max(GT depth, LT depth).
    // GT/LT each: 1 + max(VAR, CONST) = 1 + 1 = 2. So depth = 1 + 2 = 3.
    VERIFY(depth == 3);
    std::cout << "recursive max-depth visitor: tree (and (> x 0) (< x 10)) depth=" << depth << " OK" << std::endl;

    // Single node depth
    int depth_single = depth_disp.dispatch(root, depth_ctx);
    VERIFY(depth_single == 2);  // GT(x, 0) -> 1 + max(1,1) = 2
    std::cout << "single (> x 0) depth=" << depth_single << " OK" << std::endl;

    std::cout << "======= All Context & Dispatcher tests passed =======" << std::endl;
    return 0;
}
