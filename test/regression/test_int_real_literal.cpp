// A numeric literal's sort is IntOrReal, and it must be exempt from the
// int/real type check.
//
// canExempt lists the sorts that may mix under the int/real exemption and
// listed only Int and Real. A literal carries neither: `2` has sort IntOrReal.
// So `(/ x 2)` on an Int variable was rejected as "Type mismatch in div" while
// `(/ x y)` on two declared Reals passed -- the exemption held everywhere
// except where a literal met an operator that wants Reals, which is most places
// anyone would write one.
//
// getSort(), twenty lines below canExempt in the same file, already asked
// isIntOrReal() for exactly this reason. The two disagreed.
//
// The failure was also reported badly, and that is worth its own case below:
// err_all THROWS ParseErrorException rather than returning an error node, so a
// well-formed term rejected here does not come back as `isErr()` -- it unwinds
// out of the builder. A caller that expected a node got an exception instead.

#include "somtparser/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

/** True when building @p f raises the parser's type error. err_all throws, so
 *  a rejection cannot be observed as a returned node. */
template <typename F>
bool rejects(F&& f) {
    try {
        auto n = f();
        return n == nullptr || n->isErr() || n->isUnknown();
    } catch (const std::exception&) {
        return true;
    }
}

} // namespace

int main() {
    std::cout << "======= numeric literals and the int/real exemption =======\n";

    // ---- A literal really does carry IntOrReal. ----------------------------
    {
        // The premise. If this ever changes, the rest of the file is testing
        // something other than what it claims to.
        ParserPtr p = newParser();
        VERIFY(p->mkConstInt(2)->getSort()->isIntOrReal());
        VERIFY(p->mkVarInt("x")->getSort()->isInt());
    }

    // ---- Real division accepts an integer literal. -------------------------
    {
        // The original failure, in its smallest form: an Int variable divided
        // by an integer literal.
        ParserPtr p = newParser();
        const auto q = p->mkDivReal(p->mkVarInt("x"), p->mkConstInt(2));
        VERIFY(q != nullptr && !q->isErr() && !q->isUnknown());
        VERIFY(q->getSort()->isReal());
    }
    {
        // A Real variable and an integer literal, mixing the other way.
        ParserPtr p = newParser();
        const auto q = p->mkDivReal(p->mkVarReal("r"), p->mkConstInt(2));
        VERIFY(q != nullptr && !q->isErr() && !q->isUnknown());
        VERIFY(q->getSort()->isReal());
    }

    // ---- Integer division took the same route. -----------------------------
    {
        ParserPtr p = newParser();
        const auto q = p->mkDivInt(p->mkVarInt("x"), p->mkConstInt(2));
        VERIFY(q != nullptr && !q->isErr() && !q->isUnknown());
        VERIFY(q->getSort()->isInt());
    }

    // ---- Through the parser, which is how a user meets it. -----------------
    {
        ParserPtr p = newParser();
        VERIFY(p->parseStr("(declare-fun x () Int)\n(declare-fun r () Real)\n"
                           "(assert (= r (/ x 2)))\n"));
        VERIFY(has(p->dumpSMT2(), "(/ "));
    }

    // ---- The exemption is not a licence to mix anything. -------------------
    {
        // A Bool operand is still a type error. Without this, "fixing" the
        // check by deleting it passes every assertion above.
        ParserPtr p = newParser();
        VERIFY(rejects([&] { return p->mkDivReal(p->mkVarBool("b"), p->mkConstInt(2)); }));
    }

    std::cout << "All int/real literal tests passed." << std::endl;
    return 0;
}
