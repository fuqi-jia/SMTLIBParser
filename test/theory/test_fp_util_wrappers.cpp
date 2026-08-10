#include <cmath>
#include <cfenv>
#include <iostream>
#include <string>

#include "somtparser/frontend/parser.h"
#include "somtparser/core/util.h"
#include "somtparser/ir/value.h"
#include "test_helpers.h"

int main() {
    using namespace SOMTParser;

    ParserPtr parser = newParser();

    {
        auto nanN = parser->mkExpr("(_ NaN 8 24)");
        VERIFY(nanN && !nanN->isErr());
        VERIFY(fpNodeIsNaN(nanN));
        VERIFY(!fpNodeIsInf(nanN));
        VERIFY(!fpNodeIsZero(nanN));
    }

    {
        auto pinf = parser->mkExpr("(_ +oo 8 24)");
        VERIFY(pinf && fpNodeIsInf(pinf) && !fpNodeIsNeg(pinf));
        auto ninf = parser->mkExpr("(_ -oo 8 24)");
        VERIFY(ninf && fpNodeIsInf(ninf) && fpNodeIsNeg(ninf));
    }

    {
        auto pz = parser->mkExpr("(_ +zero 8 24)");
        VERIFY(pz && fpNodeIsZero(pz) && !fpNodeIsNeg(pz));
        auto nz = parser->mkExpr("(_ -zero 8 24)");
        VERIFY(nz && fpNodeIsZero(nz) && fpNodeIsNeg(nz));
    }

    {
        auto one = parser->mkExpr("((_ to_fp 8 24) RNE 1.0)");
        VERIFY(one && fpNodeIsNormal(one) && !fpNodeIsSubnormal(one));
        VERIFY(!fpNodeIsNeg(one));
        auto neg = parser->mkExpr("((_ to_fp 8 24) RNE -2.5)");
        VERIFY(neg && fpNodeIsNeg(neg));
    }

    {
        auto sub = parser->mkExpr("(fp #b0 #b00000000 #b00000000000000000000001)");
        VERIFY(sub && fpNodeIsSubnormal(sub));
        VERIFY(sub->getValue() != nullptr);
        VERIFY(sub->getValue()->getType() == ValueType::FP);
    }

    {
        auto rm = parser->mkExpr("RNE");
        VERIFY(rm);
        VERIFY(getFPRoundingMode(rm) == FE_TONEAREST);
    }

    {
        float x = 1.375f;
        std::string smt = float32ToSMTFP(x);
        auto n = parser->mkExpr(smt);
        VERIFY(n && !n->isErr());
        auto back = fpNodeToFloat32(n);
        VERIFY(back.has_value());
        VERIFY(std::fabs(static_cast<double>(*back - x)) < 1e-6);
    }

    {
        double y = -3.125;
        std::string smt = float64ToSMTFP(y);
        auto n = parser->mkExpr(smt);
        VERIFY(n && !n->isErr());
        auto back = fpNodeToFloat64(n);
        VERIFY(back.has_value());
        VERIFY(std::fabs(*back - y) < 1e-12);
    }

    {
        ModelPtr model = newModel();
        auto phi = parser->mkExpr("(fp.isNaN (_ NaN 8 24))");
        VERIFY(phi && !phi->isErr());
        auto ev = parser->evaluate(phi, model);
        VERIFY(ev && ev->isTrue());
        VERIFY(parser->toString(ev) == "true");
    }

    std::cout << "test_fp_util_wrappers: all assertions passed\n";
    return 0;
}
