// SymbolManager::popLetScope() reads and pops let_scope_checkpoints_.  Its
// precondition used to be stated with assert(), which the default Release
// build (-DNDEBUG) removes, leaving .back()/.pop_back() on a possibly empty
// vector -- undefined behaviour in exactly the configuration that ships.
//
// It now uses condAssert, which throws unconditionally.  These tests pin that
// an unbalanced pop is diagnosed in every build type, and that the balanced
// path is unaffected.

#include <iostream>
#include <stdexcept>
#include <string>

#include "somtparser/frontend/parser.h"
#include "somtparser/frontend/symbol_manager.h"
#include "test_helpers.h"

using namespace SOMTParser;

namespace {

// The invariant the guard protects is maintained by base_parser: each
// pushLetScope() is undone through the per-frame scope_pushed flag, including
// on the error exits. Those exits are the interesting ones -- a let that fails
// mid-binding must still leave the symbol table balanced -- so drive them
// through the parser rather than only unit-testing SymbolManager.
void parseErrorLeavesParserUsable(const char* label, const std::string& script) {
    ParserPtr p = newParser();
    try {
        p->parseStr(script);
    } catch (const std::exception&) {
        // Reporting the malformed input is fine; leaking a let scope is not.
    }
    // A well-formed let afterwards must still bind, print and unbind normally.
    auto node = p->mkExpr("(let ((y 1)) y)");
    VERIFY(node && !node->isErr());
    VERIFY(p->toString(node) == "(let ((y 1)) y)");
    // And the binding must not have escaped its scope.
    VERIFY(!p->getSymbolManager()->hasLet("y"));
    std::cout << "  ok: " << label << "\n";
}

}  // namespace

int main() {
    std::cout << "======= let scope guard tests =======\n";

    // Balanced use: a binding registered inside a scope disappears with it.
    {
        SymbolManager sm;
        sm.pushLetScope();
        sm.registerLet("x", nullptr);
        VERIFY(sm.hasLet("x"));
        sm.popLetScope();
        VERIFY(!sm.hasLet("x"));
        std::cout << "  ok: balanced push/pop restores the outer scope\n";
    }

    // Nested scopes: the inner pop must only undo the inner bindings.
    {
        SymbolManager sm;
        sm.pushLetScope();
        sm.registerLet("outer", nullptr);
        sm.pushLetScope();
        sm.registerLet("inner", nullptr);
        VERIFY(sm.hasLet("outer") && sm.hasLet("inner"));
        sm.popLetScope();
        VERIFY(sm.hasLet("outer") && !sm.hasLet("inner"));
        sm.popLetScope();
        VERIFY(!sm.hasLet("outer"));
        std::cout << "  ok: nested pop only undoes the inner scope\n";
    }

    // Unbalanced pop: a diagnosable error, not undefined behaviour.  The
    // library is compiled with -DNDEBUG in Release, so this only holds because
    // the check is a condAssert rather than an assert.
    {
        SymbolManager sm;
        bool threw = false;
        try {
            sm.popLetScope();
        } catch (const std::exception& e) {
            threw = true;
            const std::string msg = e.what();
            VERIFY(msg.find("popLetScope") != std::string::npos);
        }
        VERIFY(threw);
        std::cout << "  ok: pop without a matching push throws\n";
    }

    // One pop too many after a balanced pair must also be caught.
    {
        SymbolManager sm;
        sm.pushLetScope();
        sm.popLetScope();
        bool threw = false;
        try {
            sm.popLetScope();
        } catch (const std::exception&) {
            threw = true;
        }
        VERIFY(threw);
        std::cout << "  ok: extra pop after a balanced pair throws\n";
    }

    // The parser-level error exits that pop the scopes they pushed.
    parseErrorLeavesParserUsable(
        "duplicate binding in one let leaves the scope balanced",
        "(declare-fun a () Int)\n(assert (= a (let ((x 1) (x 2)) x)))\n");
    parseErrorLeavesParserUsable(
        "failure inside a binding value leaves the scope balanced",
        "(declare-fun a () Int)\n(assert (= a (let ((x (undefined_fn 1))) x)))\n");
    parseErrorLeavesParserUsable(
        "failure inside a nested let leaves both scopes balanced",
        "(declare-fun a () Int)\n"
        "(assert (= a (let ((x 1)) (let ((z (undefined_fn x))) z))))\n");

    // The success path still nests correctly after all of the above.
    {
        ParserPtr p = newParser();
        p->parseStr("(declare-fun a () Int)\n(assert (= a (let ((x 1)) (let ((y x)) y))))\n");
        VERIFY(p->getAssertions().size() == 1);
        VERIFY(!p->getSymbolManager()->hasLet("x") && !p->getSymbolManager()->hasLet("y"));
        std::cout << "  ok: nested let parses and unbinds both scopes\n";
    }

    std::cout << "All let scope guard tests passed.\n";
    return 0;
}
