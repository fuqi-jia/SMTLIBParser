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

#include "somtparser/frontend/symbol_manager.h"
#include "test_helpers.h"

using namespace SOMTParser;

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

    std::cout << "All let scope guard tests passed.\n";
    return 0;
}
