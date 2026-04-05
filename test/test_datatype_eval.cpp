#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include <cassert>

// NDEBUG-safe assertion for side-effectful expressions
#define VERIFY(expr) do { if(!(expr)) { std::cerr << "VERIFY failed: " #expr << " at " << __FILE__ << ":" << __LINE__ << "\n"; std::abort(); } } while(0)

static std::string dirname_of(const std::string& path) {
    auto p = path.find_last_of("/\\");
    return p == std::string::npos ? std::string(".") : path.substr(0, p);
}

int main() {
    using namespace SOMTParser;

    std::string instance = dirname_of(__FILE__) + "/instances/datatypes.smt2";

    ParserPtr parser = newParser();
    bool parsed = parser->parse(instance);
    if (!parsed) {
        parser = newParser();
        VERIFY(parser->parseStr("(set-logic ALL)"));
        VERIFY(parser->parseStr(
            "(declare-datatypes ((Either 0)) (((left (lv Int)) (right (rv Int)))))"));
        std::cerr << "note: parseStr fallback (could not read " << instance << ")\n";
    } else {
        std::cout << "parsed " << instance << "\n";
    }

    ModelPtr model = newModel();

    {
        auto e = parser->mkExpr("(left 7)");
        VERIFY(e && !e->isErr());
        VERIFY(e->isConstructorApp());
        VERIFY(e->getName() == "left");
        auto ev = parser->evaluate(e, model);
        VERIFY(ev && ev->isConstructorApp());
        VERIFY(ev->getName() == "left");
        VERIFY(ev->getChildrenSize() == 1);
        VERIFY(parser->toString(ev->getChild(0)) == "7");
    }

    {
        auto e = parser->mkExpr("(lv (left 7))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        VERIFY(ev && parser->toString(ev) == "7");
    }

    {
        auto e = parser->mkExpr("(is-left (left 7))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        VERIFY(ev && ev->isTrue());
    }

    {
        auto e = parser->mkExpr("(is-right (left 7))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        VERIFY(ev && ev->isFalse());
    }

    {
        auto e = parser->mkExpr("(lv (right 3))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        VERIFY(ev && !parser->toString(ev).empty());
        VERIFY(parser->toString(ev) != "3");
    }

    // ─── DT structural equality ─────────────────────────────────────────
    {
        auto e = parser->mkExpr("(= (left 7) (left 7))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (= (left 7) (left 7)) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isTrue());
    }
    {
        auto e = parser->mkExpr("(= (left 7) (left 8))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (= (left 7) (left 8)) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isFalse());
    }
    {
        auto e = parser->mkExpr("(= (left 7) (right 7))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (= (left 7) (right 7)) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isFalse());
    }
    {
        auto e = parser->mkExpr("(distinct (left 1) (left 2))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (distinct (left 1) (left 2)) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isTrue());
    }
    {
        auto e = parser->mkExpr("(distinct (left 1) (left 1))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (distinct (left 1) (left 1)) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isFalse());
    }

    std::cout << "test_datatype_eval: basic DT assertions passed\n";

    // ─── Match expression tests ─────────────────────────────────────────
    {
        auto e = parser->mkExpr("(match (left 42) ((left x) x) ((right y) y))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (match (left 42) ...) => " << parser->toString(ev) << "\n";
        VERIFY(ev && parser->toString(ev) == "42");
    }
    {
        auto e = parser->mkExpr("(match (right 99) ((left x) x) ((right y) y))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (match (right 99) ...) => " << parser->toString(ev) << "\n";
        VERIFY(ev && parser->toString(ev) == "99");
    }
    {
        // match with arithmetic in body
        auto e = parser->mkExpr("(match (left 5) ((left x) (+ x 10)) ((right y) (- y 1)))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (match (left 5) with +10) => " << parser->toString(ev) << "\n";
        VERIFY(ev && parser->toString(ev) == "15");
    }
    {
        // match right branch with arithmetic
        auto e = parser->mkExpr("(match (right 5) ((left x) (+ x 10)) ((right y) (- y 1)))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (match (right 5) with -1) => " << parser->toString(ev) << "\n";
        VERIFY(ev && parser->toString(ev) == "4");
    }
    {
        // nested match: match result of a selector
        auto e = parser->mkExpr("(match (left 7) ((left x) (= x 7)) ((right y) false))");
        VERIFY(e && !e->isErr());
        auto ev = parser->evaluate(e, model);
        std::cout << "  (match (left 7) eq check) => " << parser->toString(ev) << "\n";
        VERIFY(ev && ev->isTrue());
    }

    std::cout << "test_datatype_eval: match expression tests passed\n";

    // ─── Recursive DT tests ─────────────────────────────────────────────
    {
        // Declare a recursive list datatype
        ParserPtr p2 = newParser();
        VERIFY(p2->parseStr("(set-logic ALL)"));
        VERIFY(p2->parseStr(
            "(declare-datatypes ((IntList 0)) (((cons (head Int) (tail IntList)) (nil))))"));
        ModelPtr m2 = newModel();

        // nil
        {
            auto e = p2->mkExpr("nil");
            VERIFY(e && !e->isErr());
            VERIFY(e->isConstructorApp());
            VERIFY(e->getName() == "nil");
        }
        // (cons 1 nil)
        {
            auto e = p2->mkExpr("(cons 1 nil)");
            VERIFY(e && !e->isErr());
            VERIFY(e->isConstructorApp());
            VERIFY(e->getName() == "cons");
            auto ev = p2->evaluate(e, m2);
            VERIFY(ev && ev->isConstructorApp());
        }
        // (head (cons 1 nil))
        {
            auto e = p2->mkExpr("(head (cons 1 nil))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            VERIFY(ev && p2->toString(ev) == "1");
        }
        // (is-cons (cons 1 nil))
        {
            auto e = p2->mkExpr("(is-cons (cons 1 nil))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            VERIFY(ev && ev->isTrue());
        }
        // (is-nil (cons 1 nil))
        {
            auto e = p2->mkExpr("(is-nil (cons 1 nil))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            VERIFY(ev && ev->isFalse());
        }
        // (is-nil nil)
        {
            auto e = p2->mkExpr("(is-nil nil)");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            VERIFY(ev && ev->isTrue());
        }
        // Recursive equality: (= (cons 1 nil) (cons 1 nil))
        {
            auto e = p2->mkExpr("(= (cons 1 nil) (cons 1 nil))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            std::cout << "  (= (cons 1 nil) (cons 1 nil)) => " << p2->toString(ev) << "\n";
            VERIFY(ev && ev->isTrue());
        }
        // Nested cons: (head (tail (cons 1 (cons 2 nil))))
        {
            auto e = p2->mkExpr("(head (tail (cons 1 (cons 2 nil))))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            std::cout << "  (head (tail (cons 1 (cons 2 nil)))) => " << p2->toString(ev) << "\n";
            VERIFY(ev && p2->toString(ev) == "2");
        }
        // match on recursive DT
        {
            auto e = p2->mkExpr("(match (cons 42 nil) ((cons h t) h) (nil 0))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            std::cout << "  (match (cons 42 nil) ...) => " << p2->toString(ev) << "\n";
            VERIFY(ev && p2->toString(ev) == "42");
        }
        // match nil case
        {
            auto e = p2->mkExpr("(match nil ((cons h t) h) (nil 0))");
            VERIFY(e && !e->isErr());
            auto ev = p2->evaluate(e, m2);
            std::cout << "  (match nil ...) => " << p2->toString(ev) << "\n";
            VERIFY(ev && p2->toString(ev) == "0");
        }

        std::cout << "test_datatype_eval: recursive DT tests passed\n";
    }

    std::cout << "test_datatype_eval: all assertions passed\n";
    return 0;
}
