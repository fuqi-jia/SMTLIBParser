/**
 * Test that the umbrella header <somtparser/parser.h> provides the full API:
 * Parser, Rewriter, Context, Node, etc. One include, no other project headers.
 */
#include "somtparser/parser.h"
#include <cassert>
#include <iostream>

int main() {
    std::cout << "======= Umbrella Header Test =======\n";

    // Parser API (from frontend/parser.h)
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    std::shared_ptr<SOMTParser::DAGNode> node = parser->mkExpr("(and true false)");
    assert(node && node->isFalse());
    std::cout << "Parser: (and true false) -> " << parser->toString(node) << " OK\n";

    // Rewriter API (from passes/rewriter.h)
    SOMTParser::ParserPtr p2 = SOMTParser::newParser();
    p2->parseStr("(declare-const x Bool)");
    bool ok = p2->parseStr("(assert (not (not x)))");
    assert(ok);
    std::shared_ptr<SOMTParser::DAGNode> n = p2->getAssertions().back();
    SOMTParser::Rewriter r(p2->getNodeManager());
    SOMTParser::Node simplified = r.rewrite(SOMTParser::Node(n));
    assert(simplified);
    std::cout << "Rewriter: (not (not x)) -> " << p2->toString(simplified) << " OK\n";

    // Context / ParserContext (from context + frontend)
    SOMTParser::Context& ctx = parser->context();
    (void)ctx.getNodeManager();
    (void)static_cast<SOMTParser::ParserContext&>(parser->context()).getAssertions();
    std::cout << "Context/ParserContext OK\n";

    std::cout << "======= Umbrella test passed =======\n";
    return 0;
}
