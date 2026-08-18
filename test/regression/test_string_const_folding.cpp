// String operators applied to STRING CONSTANTS.
//
// Constant folding in simp_oper only fires when an argument IS a constant, so
// a test that exercises an operator over a declared variable never reaches it.
// Four defects lived in that gap, and all four are about a string constant
// having two spellings:
//
//   * `str.to_int` and `str.to_code` RAISED AN ERROR for an argument the
//     standard gives a value. Both are total in SMT-LIB -- a string that is not
//     a run of digits maps to -1, a string that is not one character maps to -1
//     -- so `(str.to_int "abc")` is a well-formed term, and erroring turned a
//     legal script into a parse failure.
//
//   * `str.to_int`, `str.to_code`, `str.is_digit` and `str.rev` read
//     `toString()`, which renders a string constant WITH its quotes. So the
//     is-it-digits test saw `"7"` rather than `7`, and `"a"` was three
//     characters long: no literal ever reached the folding branch, including
//     `(str.to_int "7")`. `getStringLiteral()` is the accessor for the value,
//     and the NT_STR_LEN case beside them was already using it.
//
//   * `str.is_digit` was sharing `str.to_int`'s test, so a run of digits
//     satisfied it and `(str.is_digit "77")` folded to true.
//
//   * `mkConstStr` stored a quoted argument with its quotes and an unquoted one
//     without. Nodes are hash-consed by name, so a constant built through the
//     API and the identical literal read from a script were never the same node
//     and `(= (str.from_int 7) "7")` folded to FALSE. Both PRINT as `"7"`,
//     because printing adds the quotes when they are missing, so it was
//     invisible in the output and showed up only as an equality that would not
//     hold.
//
// The assertions are on the VALUE rather than on acceptance: each expression is
// folded and compared against what the standard says it equals, so a folder
// that returns the wrong number fails rather than one that merely returns.

#include "somtparser/frontend/parser.h"

#include <iostream>
#include <string>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

/** Parse `(assert <e>)` and report whether it folded to `true`. A fully folded
 *  true is the only outcome that means "the value is what the standard says". */
bool foldsTrue(const std::string& e) {
    ParserPtr p = newParser();
    if (!p->parseStr("(assert " + e + ")\n(check-sat)\n")) { return false; }
    const std::string out = p->dumpSMT2();
    return out.find("(assert true)") != std::string::npos;
}

} // namespace

int main() {
    std::cout << "======= string operators over constants =======\n";

    // ---- str.to_int: total, and -1 is a value rather than an error. ---------
    VERIFY(foldsTrue("(= (str.to_int \"7\") 7)"));
    // Leading zeroes are digits, so "0007" is 7 -- not -1, which is what a
    // reader that rejected the spelling would give.
    VERIFY(foldsTrue("(= (str.to_int \"0007\") 7)"));
    VERIFY(foldsTrue("(= (str.to_int \"abc\") (- 1))"));
    VERIFY(foldsTrue("(= (str.to_int \"\") (- 1))"));
    // A mixed string is not a run of digits, so it is -1 and not a prefix.
    VERIFY(foldsTrue("(= (str.to_int \"12a\") (- 1))"));
    VERIFY(foldsTrue("(= (str.to_int \"-3\") (- 1))"));

    // ---- str.to_code: the code point, or -1. -------------------------------
    VERIFY(foldsTrue("(= (str.to_code \"a\") 97)"));
    VERIFY(foldsTrue("(= (str.to_code \"A\") 65)"));
    VERIFY(foldsTrue("(= (str.to_code \"ab\") (- 1))"));
    VERIFY(foldsTrue("(= (str.to_code \"\") (- 1))"));

    // ---- str.is_digit: ONE digit character. --------------------------------
    VERIFY(foldsTrue("(str.is_digit \"7\")"));
    VERIFY(foldsTrue("(not (str.is_digit \"77\"))"));
    VERIFY(foldsTrue("(not (str.is_digit \"a\"))"));
    VERIFY(foldsTrue("(not (str.is_digit \"\"))"));

    // ---- One canonical spelling for a constant. -----------------------------
    //
    // The two sides of each equality reach mkConstStr by different routes: one
    // from an operator that built the value, one from the parser reading a
    // literal. They must be the same node.
    VERIFY(foldsTrue("(= (str.from_int 7) \"7\")"));
    VERIFY(foldsTrue("(= (str.rev \"abc\") \"cba\")"));
    VERIFY(foldsTrue("(= (str.++ \"ab\" \"c\") \"abc\")"));
    VERIFY(foldsTrue("(= (str.len \"abc\") 3)"));

    std::cout << "All string constant-folding tests passed." << std::endl;
    return 0;
}
