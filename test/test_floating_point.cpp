#include <iostream>
#include <string>
#include <vector>
#include "somtparser/frontend/parser.h"
#include <cassert>

// 测试基本的浮点数常量和表示
void test_fp_constants(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(_ +zero 8 24)",        // IEEE 754单精度+0.0
        "(_ -zero 8 24)",        // IEEE 754单精度-0.0
        "(_ +oo 8 24)",          // IEEE 754单精度+∞
        "(_ -oo 8 24)",          // IEEE 754单精度-∞
        "(_ NaN 8 24)",          // IEEE 754单精度NaN
        "(fp #b0 #b01111111 #b00000000000000000000000)", // IEEE 754单精度1.0的位表示
        "(fp #b1 #b10000010 #b01100000000000000000000)"  // IEEE 754单精度-6.5的位表示
    };
    
    std::cout << "=== 测试浮点数常量 ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "表达式: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  结果: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// 测试浮点数算术运算
void test_fp_arithmetic(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(fp.add RNE ((_ to_fp 8 24) RNE 3.14) ((_ to_fp 8 24) RNE 2.71))",    // 加法，向最近取整
        "(fp.sub RNE ((_ to_fp 8 24) RNE 10.5) ((_ to_fp 8 24) RNE 4.2))",     // 减法，向最近取整
        "(fp.mul RNE ((_ to_fp 8 24) RNE 2.5) ((_ to_fp 8 24) RNE 4.0))",      // 乘法，向最近取整
        "(fp.div RNE ((_ to_fp 8 24) RNE 15.0) ((_ to_fp 8 24) RNE 3.0))",     // 除法，向最近取整
        "(fp.fma RNE ((_ to_fp 8 24) RNE 2.0) ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 1.0))", // 乘加融合
        "(fp.sqrt RNE ((_ to_fp 8 24) RNE 16.0))",                              // 平方根
        "(fp.rem ((_ to_fp 8 24) RNE 17.5) ((_ to_fp 8 24) RNE 5.2))",         // 取余
        "(fp.roundToIntegral RNE ((_ to_fp 8 24) RNE 3.7))",                    // 向整数舍入
        "(fp.min ((_ to_fp 8 24) RNE 4.2) ((_ to_fp 8 24) RNE 4.3))",          // 最小值
        "(fp.max ((_ to_fp 8 24) RNE 4.2) ((_ to_fp 8 24) RNE 4.3))"           // 最大值
    };
    
    std::cout << "=== 测试浮点数算术运算 ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "表达式: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  结果: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// 测试浮点数比较操作
void test_fp_comparisons(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        "(fp.eq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",           // 相等
        "(fp.lt ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 4.0))",           // 小于
        "(fp.gt ((_ to_fp 8 24) RNE 5.0) ((_ to_fp 8 24) RNE 4.0))",           // 大于
        "(fp.leq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",          // 小于等于
        "(fp.geq ((_ to_fp 8 24) RNE 3.0) ((_ to_fp 8 24) RNE 3.0))",          // 大于等于
        "(fp.isNormal ((_ to_fp 8 24) RNE 1.0))",                               // 是否为规格化数
        "(fp.isSubnormal ((_ to_fp 11 53) RNE 0.0001))",                      // 是否为次规格化数
        "(fp.isZero ((_ to_fp 8 24) RNE 0.0))",                                 // 是否为零
        "(fp.isInfinite (_ +oo 8 24))",                                       // 是否为无穷大
        "(fp.isNaN (_ NaN 8 24))",                                            // 是否为NaN
        "(fp.isNegative ((_ to_fp 8 24) RNE -1.0))",                            // 是否为负数
        "(fp.isPositive ((_ to_fp 8 24) RNE 1.0))"                              // 是否为正数
    };
    
    std::cout << "=== 测试浮点数比较操作 ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "表达式: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  结果: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

// 测试浮点数转换操作
void test_fp_conversions(SOMTParser::ParserPtr& parser) {
    std::vector<std::string> expressions = {
        // 从实数到浮点数
        "((_ to_fp 8 24) RNE 3.14159)",
        // 从整数到浮点数
        "((_ to_fp 8 24) RNE 42)",
        // 从二进制字符串到浮点数
        "((_ to_fp 8 24) #b01000001001000000000000000000000)",
        // 从浮点数到实数
        "(fp.to_real ((_ to_fp 8 24) RNE 3.14))",
        // 从浮点数到有符号位向量（向零舍入）
        "((_ fp.to_sbv 32) RTZ ((_ to_fp 8 24) RNE 3.14))",
        // 从浮点数到无符号位向量（向零舍入）
        "((_ fp.to_ubv 32) RTZ ((_ to_fp 8 24) RNE 3.14))",
        // 从实数到不同精度浮点数
        "((_ to_fp 11 53) RNE 3.14)",  // 实数到双精度
        "((_ to_fp 5 11) RNE 3.14)",   // 实数到半精度
        // 不同精度浮点数之间的转换
        "((_ to_fp 11 53) RNE ((_ to_fp 8 24) RNE 3.14))",  // 单精度到双精度
        "((_ to_fp 5 11) RNE ((_ to_fp 8 24) RNE 3.14))"    // 单精度到半精度
    };

    std::cout << "=== 测试浮点数转换操作 ===" << std::endl;
    for (const auto& expr : expressions) {
        std::cout << "表达式: " << expr << std::endl;
        std::shared_ptr<SOMTParser::DAGNode> result = parser->mkExpr(expr);
        assert(result && !result->isErr());
        std::cout << "  结果: " << parser->toString(result) << std::endl;
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "======= 浮点数理论测试 =======" << std::endl;
    
    SOMTParser::ParserPtr parser = SOMTParser::newParser();
    
    test_fp_constants(parser);
    test_fp_arithmetic(parser);
    test_fp_comparisons(parser);
    test_fp_conversions(parser);
    
    return 0;
} 