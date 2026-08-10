// A `:named` annotation is stripped while parsing and kept aside for unsat
// cores.  Three things were wrong with that:
//
//   * dumpSMT2 never emitted it, so a dumped script could no longer answer
//     (get-unsat-core) with the names the input used;
//   * the standard form (assert (! e :named n)) never registered the name with
//     the push/pop scope, so the name outlived the (pop ...) that removed the
//     assertion it belonged to;
//   * naming the same assertion twice silently dropped one binding -- nodes are
//     hash-consed, so `(! A :named X)` and `(! A :named Y)` name one node.
//
// The last one has no lossless answer: the parser now warns and keeps the last
// name.  These tests pin the warning down as much as the result.

#include <iostream>
#include <sstream>
#include <string>

#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

using namespace SOMTParser;

namespace {

void fail(const std::string& what, const std::string& detail) {
    std::cerr << "FAILED " << what << "\n" << detail;
    std::abort();
}

void expectContains(const std::string& what, const std::string& hay, const std::string& needle) {
    if (hay.find(needle) == std::string::npos)
        fail(what, "expected to find\n  " + needle + "\nin\n" + hay);
}

void expectAbsent(const std::string& what, const std::string& hay, const std::string& needle) {
    if (hay.find(needle) != std::string::npos)
        fail(what, "did not expect\n  " + needle + "\nin\n" + hay);
}

// Parse with std::cout captured, so the warnings are testable.
struct Parsed {
    ParserPtr parser;
    std::string warnings;
};

Parsed parseCapturing(const std::string& script) {
    std::ostringstream captured;
    std::streambuf* saved = std::cout.rdbuf(captured.rdbuf());
    ParserPtr p = newParser();
    try {
        p->parseStr(script);
    } catch (...) {
        std::cout.rdbuf(saved);
        throw;
    }
    std::cout.rdbuf(saved);
    return {p, captured.str()};
}

// Dump, re-parse the dump, dump again; require a fixed point.  Returns it.
std::string dumpFixedPoint(const std::string& label, const std::string& script) {
    ParserPtr p = newParser();
    p->parseStr(script);
    const std::string once = p->dumpSMT2();

    ParserPtr q = newParser();
    q->parseStr(once);
    const std::string twice = q->dumpSMT2();

    if (once != twice)
        fail(label, "dump is not a fixed point\n--- first ---\n" + once + "--- second ---\n" + twice);
    std::cout << "  ok: " << label << "\n";
    return once;
}

const std::string DECLS = "(set-logic ALL)\n(declare-fun A () Bool)\n(declare-fun B () Bool)\n";

// ── The name has to come back out. ────────────────────────────────────────
void test_named_survives_dump() {
    std::cout << "-- :named round trip --\n";
    const std::string dump = dumpFixedPoint(
        "standard (! e :named n)",
        DECLS + "(assert (! A :named AA))\n(assert (! (not B) :named BB))\n(assert (= A B))\n");

    expectContains("name is emitted", dump, ":named AA");
    expectContains("name is emitted", dump, ":named BB");
    // The unnamed assertion stays unannotated.
    expectContains("unnamed assertion stays plain", dump, "(assert (= A B))");

    // And the names still resolve to the same assertions after a round trip.
    ParserPtr q = newParser();
    q->parseStr(dump);
    auto named = q->getNamedAssertions();
    VERIFY(named.size() == 2);
    VERIFY(named.count("AA") == 1 && named.count("BB") == 1);
    VERIFY(q->toString(named["AA"]) == "A");
    std::cout << "  ok: names still resolve after a round trip\n";

    // The non-standard trailing form the parser also accepts.
    const std::string trailing =
        dumpFixedPoint("trailing (assert e :named n)", DECLS + "(assert A :named AA)\n");
    expectContains("trailing form keeps its name", trailing, ":named AA");
}

// ── One assertion, two names: warn, keep the last. ────────────────────────
void test_assertion_named_twice() {
    std::cout << "-- one assertion named twice --\n";
    // Hash-consing makes both asserts the same node.
    Parsed r = parseCapturing(DECLS + "(assert (! A :named AA))\n(assert (! A :named BB))\n");
    expectContains("a dropped name is reported", r.warnings, "already named");
    expectContains("the warning names the survivor", r.warnings, "BB");
    expectContains("the warning names the casualty", r.warnings, "AA");

    auto named = r.parser->getNamedAssertions();
    VERIFY(named.size() == 1);
    VERIFY(named.count("BB") == 1 && named.count("AA") == 0);
    std::cout << "  ok: warned, and only the last name survives\n";

    const std::string dump = r.parser->dumpSMT2();
    expectContains("survivor is dumped", dump, ":named BB");
    expectAbsent("dropped name is not dumped", dump, ":named AA");

    // Re-naming with the *same* name is not a displacement and must not warn.
    Parsed same = parseCapturing(DECLS + "(assert (! A :named AA))\n(assert (! A :named AA))\n");
    expectAbsent("re-stating the same name is silent", same.warnings, "warning:");
    VERIFY(same.parser->getNamedAssertions().size() == 1);
    std::cout << "  ok: re-stating the same name is not a displacement\n";
}

// ── One name, two assertions: warn, keep the last. ────────────────────────
void test_name_reused() {
    std::cout << "-- one name for two assertions --\n";
    Parsed r = parseCapturing(DECLS + "(assert (! A :named N))\n(assert (! B :named N))\n");
    expectContains("a stolen name is reported", r.warnings, "already used for another assertion");

    auto named = r.parser->getNamedAssertions();
    VERIFY(named.size() == 1);
    VERIFY(r.parser->toString(named["N"]) == "B");
    std::cout << "  ok: warned, and the name refers to the last assertion\n";

    // The assertion that lost the name must dump plain, not with a stale one.
    const std::string dump = r.parser->dumpSMT2();
    expectContains("survivor keeps the name", dump, "(assert (! B :named N))");
    expectContains("the other dumps plain", dump, "(assert A)");
}

// ── The same node asserted twice carries the name once. ───────────────────
void test_repeated_assertion_names_once() {
    std::cout << "-- a named node asserted twice --\n";
    // Emitting ":named AA" on both would be a duplicate name on re-parse.
    const std::string dump =
        dumpFixedPoint("named node asserted twice", DECLS + "(assert (! A :named AA))\n(assert A)\n");
    size_t first = dump.find(":named AA");
    VERIFY(first != std::string::npos);
    VERIFY(dump.find(":named AA", first + 1) == std::string::npos);
    std::cout << "  ok: the name is emitted exactly once\n";
}

// ── Names live and die with their scope. ──────────────────────────────────
void test_scope() {
    std::cout << "-- push/pop --\n";
    ParserPtr p = newParser();
    p->parseStr(DECLS + "(push 1)\n(assert (! A :named AA))\n(pop 1)\n");
    VERIFY(p->getAssertions().empty());
    // The name used to outlive the assertion it belonged to.
    VERIFY(p->getNamedAssertions().empty());
    std::cout << "  ok: a popped assertion takes its name with it\n";

    // A name displaced inside a scope comes back when the scope goes away.
    ParserPtr q = newParser();
    q->parseStr(DECLS + "(assert (! A :named AA))\n(push 1)\n(assert (! A :named BB))\n(pop 1)\n");
    auto named = q->getNamedAssertions();
    if (!(named.size() == 1 && named.count("AA") == 1)) {
        std::string got;
        for (const auto& kv : named) got += "  " + kv.first + "\n";
        fail("displaced name is not restored on pop", "expected only AA, got:\n" + got);
    }
    std::cout << "  ok: a name displaced inside a scope is restored on pop\n";

    // Same for a name stolen by another assertion inside the scope.
    ParserPtr s = newParser();
    s->parseStr(DECLS + "(assert (! A :named N))\n(push 1)\n(assert (! B :named N))\n(pop 1)\n");
    auto restored = s->getNamedAssertions();
    VERIFY(restored.size() == 1 && restored.count("N") == 1);
    VERIFY(s->toString(restored["N"]) == "A");
    std::cout << "  ok: a stolen name goes back to its owner on pop\n";
}

}  // namespace

int main() {
    std::cout << "======= named assertion tests =======\n";
    test_named_survives_dump();
    test_assertion_named_twice();
    test_name_reused();
    test_repeated_assertion_names_once();
    test_scope();
    std::cout << "All named assertion tests passed.\n";
    return 0;
}
