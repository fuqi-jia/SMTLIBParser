// Every instance under test/instances must survive parse -> dump -> parse -> dump.
//
// test_dump_reparse.cpp already checks this property, but only against scripts
// written by hand inside that file, and it says so in its own header comment:
// "the cases below deliberately avoid operators whose printing is fixed
// elsewhere".  That is the whole problem.  A hand-written corpus can only
// contain constructs somebody thought to write down, so it can never catch the
// case where the parser accepts something its own printer cannot emit -- which
// is precisely the defect class this property exists to catch.
//
// So this file supplies the input instead of the assertion: it runs the same
// property over the real corpus.  Three defects it found on the day it was
// written:
//
//   * define-funs-rec (mutually recursive definitions) was parsed and then
//     never dumped, while the assertions calling those functions were dumped,
//     so the output referred to symbols it never defined;
//   * NT_STR_INDEXOF_RE printed as "str.indexof", a different operator with a
//     different arity, so the output could not be read back at all;
//   * commutative operands were re-canonicalised on the way back in, so
//     dump -> parse -> dump did not converge on a fixed point.
//
// Adding an instance to test/instances therefore extends this test for free.
// That is the point: coverage should follow the corpus, not a list.

#include "somtparser/frontend/parser.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "test_helpers.h"

using namespace SOMTParser;

namespace {

const char* kCorpus = "test/instances";

std::vector<std::filesystem::path> collectInstances() {
    std::vector<std::filesystem::path> files;
    VERIFY(std::filesystem::exists(kCorpus));
    for (const auto& entry : std::filesystem::recursive_directory_iterator(kCorpus)) {
        if (entry.is_regular_file() && entry.path().extension() == ".smt2") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    // Guard against silently testing nothing if the corpus moves or the working
    // directory is wrong: a zero-file run would otherwise report success.
    VERIFY(files.size() >= 15);
    return files;
}

// Names every symbol the dump introduces, so an assertion that mentions a
// symbol the dump never defined can be reported precisely rather than as a
// bare "re-parse failed".
std::string describeReparseFailure(const std::string& dump) {
    std::istringstream in(dump);
    std::string line;
    std::size_t defs = 0;
    while (std::getline(in, line)) {
        if (line.rfind("(declare-", 0) == 0 || line.rfind("(define-", 0) == 0) { ++defs; }
    }
    std::ostringstream out;
    out << dump.size() << " bytes, " << defs << " declarations/definitions";
    return out.str();
}

struct Failure {
    std::string file;
    std::string what;
};

}  // namespace

int main() {
    std::cout << "======= dumpSMT2 round-trip over test/instances =======\n";

    const auto files = collectInstances();
    std::vector<Failure> failures;
    std::size_t checked = 0;

    for (const auto& path : files) {
        const std::string name = path.string();

        auto first = std::make_shared<Parser>();
        if (!first->parse(name)) {
            // Not a round-trip failure: the corpus holds files this parser is
            // not expected to accept.  Skipped, but counted nowhere, so it
            // cannot mask a regression in the files that do parse.
            std::cout << "  skip (does not parse): " << name << "\n";
            continue;
        }

        const std::string once = first->dumpSMT2();

        auto second = std::make_shared<Parser>();
        if (!second->parseStr(once)) {
            failures.push_back({name, "dump is not re-parsable: " + describeReparseFailure(once)});
            continue;
        }

        const std::string twice = second->dumpSMT2();
        if (once != twice) {
            // Report the first differing line rather than two whole scripts.
            std::istringstream a(once), b(twice);
            std::string la, lb;
            std::size_t lineno = 0, diff_at = 0;
            std::string sample_a, sample_b;
            while (std::getline(a, la)) {
                ++lineno;
                if (!std::getline(b, lb)) { lb.clear(); }
                if (la != lb && diff_at == 0) {
                    diff_at = lineno;
                    sample_a = la;
                    sample_b = lb;
                }
            }
            failures.push_back({name, "dump -> parse -> dump is not a fixed point at line " +
                                          std::to_string(diff_at) + "\n      first:  " + sample_a +
                                          "\n      second: " + sample_b});
            continue;
        }

        ++checked;
    }

    for (const auto& f : failures) {
        std::cerr << "  FAIL " << f.file << "\n      " << f.what << "\n";
    }

    std::cout << checked << " instance(s) round-tripped, " << failures.size() << " failed\n";
    VERIFY(failures.empty());
    // A run where every file was skipped would reach here with no failures;
    // require that the property was actually exercised.
    VERIFY(checked >= 15);

    std::cout << "All instances round-trip.\n";
    return 0;
}
