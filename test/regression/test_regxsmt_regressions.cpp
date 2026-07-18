#include "somtparser/frontend/parser.h"
#include "test_helpers.h"

#include <functional>

using namespace SOMTParser;

int main() {
    {
        Parser parser;
        parser.setCommandLogging(true);
        VERIFY(parser.getResultType() == RESULT_TYPE::RT_UNKNOWN);
        const std::string source =
            "(set-logic ALL)\n"
            "(assert (= (str.len \"Hello\") 5))\n"
            "(assert (= (str.len \"é\") 1))\n"
            "(assert (= (str.len \"\\u{e9}\") 1))\n"
            "(assert (= (str.replace_all \"abc\" \"\" \"x\") \"abc\"))\n"
            "(check-sat)\n";
        VERIFY(parser.parseStr(source, "regression.smt2"));
        VERIFY(parser.getSourceName() == "regression.smt2");
        VERIFY(parser.getSourceText() == source);
        VERIFY(parser.getScript().size() == 6);
        for (size_t i = 0; i < parser.getScript().size(); ++i) {
            const auto& command = parser.getScript()[i];
            VERIFY(command.index == i);
            VERIFY(command.range.end.offset >= command.range.begin.offset);
            VERIFY(source.substr(command.range.begin.offset,
                                 command.range.end.offset - command.range.begin.offset) ==
                   command.original);
        }
        VERIFY(parser.getScript()[1].expr != nullptr);
        VERIFY(std::string(parser.getScript()[1].kindName()) == "assert");
        VERIFY(parser.getScript()[5].isCheckSat());
        VERIFY(std::string(parser.getScript()[5].kindName()) == "check-sat");
        for (const auto& assertion : parser.getAssertions()) {
            VERIFY(assertion->isTrue());
        }
    }

    {
        Parser parser;
        parser.getOptions()->preserveOperators({
            NODE_KIND::NT_STR_REPLACE,
            NODE_KIND::NT_STR_REPLACE_ALL,
        });
        auto replacement = parser.mkExpr(
            "(str.replace_all \"a,b,a\" \"a\" \"\")");
        VERIFY(replacement != nullptr);
        VERIFY(replacement->isStrReplaceAll());
        VERIFY(replacement->getChildrenSize() == 3);

        VERIFY(parser.parseStr(
            "(set-logic QF_SLIA)\n"
            "(declare-const x String)\n"
            "(assert (= x (str.replace_all \"a,b,a\" \"a\" \"\")))\n"
            "(check-sat)\n"));
        bool found_replacement = false;
        std::function<void(const std::shared_ptr<DAGNode>&)> visit =
            [&](const std::shared_ptr<DAGNode>& node) {
                if (!node || found_replacement) return;
                if (node->isStrReplaceAll()) found_replacement = true;
                for (const auto& child : node->getChildren()) visit(child);
            };
        visit(parser.getAssertions().front());
        VERIFY(found_replacement);
    }

    {
        Parser parser;
        VERIFY(parser.parseStr(
            "(set-logic QF_UF)\n"
            "(declare-const p Bool)\n"
            "(check-sat-assuming (p))\n"));
        VERIFY(parser.getAssumptions().size() == 1);
        VERIFY(parser.getAssumptions().front().size() == 1);
        VERIFY(parser.getAssumptions().front().front()->toString() == "p");
    }

    {
        Parser parser;
        VERIFY(parser.parseStr("(assert false)\n(check-sat)\n"));
        VERIFY(parser.checkSat() == RESULT_TYPE::RT_UNSAT);
        VERIFY(parser.getResultType() == RESULT_TYPE::RT_UNSAT);
        VERIFY(parser.reset());
        VERIFY(parser.getResultType() == RESULT_TYPE::RT_UNKNOWN);
    }

    return 0;
}
