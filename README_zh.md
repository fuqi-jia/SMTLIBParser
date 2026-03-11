# SOMTParser

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**SOMTParser** 是一个与求解器无关的 C++17 前端库，面向 SMT-LIB 与 OMT（Optimization Modulo Theories）。它将输入解析为**带类型的 DAG 中间表示（IR）**，并在该 IR 上提供解析、类型检查、遍历、重写以及公式级处理等模块化能力，从而把可复用的前端逻辑与后端推理分离，便于构建新求解器或基于 SMT 的工具。

## 设计概览

- **前端**：命令/项解析、类型检查、OMT 相关输入，以及通过 API 构建表达式并接入同一 IR 管道。
- **IR 核心**：统一的带类型 DAG 表示，通过 `NodeManager` 做结构共享与规范化（canonicalization），相同子项只存一份。
- **Passes**：基于 IR 的遍历（每节点至多访问一次）与重写（自底向上、支持不动点），以及按节点种类分发的扩展机制。
- **工具**：公式转换（NNF/CNF/DNF，含 CDCL(T) 风格的布尔抽象）、模型解析与在模型下的求值（支持部分模型）。

## 主要特性

- **SMT-LIB2 支持**：符合 SMT-LIB2 规范，支持多逻辑与多理论
- **多理论**：布尔、整数/实数算术、位向量、IEEE-754 浮点、字符串与正则、数组等
- **带类型 DAG IR**：结构共享与规范化，内存更省，便于后续分析与变换
- **OMT 支持**：解析 `assert-soft`、`maximize`/`minimize`、`define-objective`、`lex-optimize`/`pareto-optimize`/`box-optimize`、`maxsat`/`minsat` 等中间语法，由 `ObjectiveManager` 统一管理
- **程序化构造**：通过 API 构造项与公式，与解析结果共用同一 IR
- **公式转换**：NNF、CNF、DNF；CNF 支持理论原子到布尔变量的抽象及双向映射，便于与 CDCL(T) 类推理对接
- **模型接口**：解析求解器输出的 model、在（完整或部分）模型下对公式/项求值
- **重写与遍历**：可扩展的重写规则、按 kind 的 dispatcher、DAG 上的一次访问遍历

## 系统要求

- 支持 C++17 的编译器
- CMake 3.10+
- GMP（GNU 多精度算术库）
- MPFR（GNU 多精度浮点库）

### 安装依赖

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install -y \
  build-essential \
  g++ \
  cmake \
  libgmp-dev \
  libmpfr-dev
```

#### Fedora/RHEL/CentOS
```bash
# Fedora
sudo dnf install -y \
  gcc-c++ \
  cmake \
  gmp-devel \
  mpfr-devel

# RHEL/CentOS
sudo yum install -y \
  gcc-c++ \
  cmake \
  gmp-devel \
  mpfr-devel
```

#### macOS
使用 [Homebrew](https://brew.sh/)：
```bash
brew install \
  cmake \
  gmp \
  mpfr
```

#### Windows

##### 使用 MSYS2
1. 安装 [MSYS2](https://www.msys2.org/)
2. 打开 MSYS2 MinGW 64-bit 终端并执行：
```bash
pacman -Syu
pacman -S \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-gmp \
  mingw-w64-x86_64-mpfr
```

##### 使用 vcpkg
1. 安装 [vcpkg](https://github.com/microsoft/vcpkg)
2. 安装依赖：
```bash
vcpkg install gmp:x64-windows mpfr:x64-windows
```

##### 使用 WSL（Windows 子系统 for Linux）
1. 安装并配置 [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
2. 在 WSL 内按 Ubuntu/Debian 说明安装依赖

## 安装

### 标准 CMake 构建

```bash
# 克隆仓库
git clone https://github.com/fuqi-jia/SOMTParser.git
cd SOMTParser

# 创建并进入构建目录
mkdir build && cd build

# 配置
cmake ..

# 编译（使用所有可用核心）
make -j$(nproc)

# 安装（可能需要管理员权限）
sudo make install
```

### 作为 Git 子模块集成

在 Git 项目中可将 SOMTParser 添加为子模块：

```bash
# 添加为子模块
git submodule add https://github.com/fuqi-jia/SOMTParser.git SOMTParser

# 初始化子模块
git submodule update --init --recursive

# 需要时更新到最新版本
git submodule update --remote --merge
```

### 构建与运行测试

```bash
mkdir -p build && cd build
cmake .. -DBUILD_TESTS=ON
make -j$(nproc)
cd test
for test in test_*; do ./$test; done
```

也可在项目根目录使用提供的测试脚本：

```bash
./test/run_tests.sh
```

### 构建配置选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `BUILD_SHARED_LIBS` | 构建动态库 (.so/.dll) | OFF |
| `BUILD_BOTH_LIBS` | 同时构建静态库与动态库 | ON |
| `BUILD_TESTS` | 构建测试可执行文件 | OFF |
| `ENABLE_DEBUG_SYMBOLS` | 启用调试符号 | OFF |

### 编译使用 SOMTParser 的应用程序

链接库及依赖示例：

```bash
g++ -std=c++17 -o application main.cpp -lsomtparser -lgmp -lmpfr
```

CMake 项目可参考 [README.md](README.md) 中的 CMake 示例。

## 配置选项

通过 `GlobalOptions` 可配置解析行为、求值精度等。常用选项包括：`logic`、`precision`、`keep_let`、`expand_functions` 等。详见 [README.md](README.md) 中的 Available Options 表格。

## API 概览

核心类型：`Parser`/`ParserPtr`、`DAGNode`/`NodePtr`、`Sort`/`SortPtr`、`Objective`、`Model`/`ModelPtr` 等。工厂方法：`newParser()`、`newModel()`。主要 API 包括变量/常量构造、表达式构造、算术/位向量/字符串/数组/浮点运算、公式求值、格式转换（toCNF/toDNF/toNNF）、模型操作等。完整列表见 [README.md](README.md) 的 API Reference 部分。

## 使用示例

所有公开 API（parser、passes、visitor、rewriter、op-dispatcher 等）通过**单一 umbrella header** 提供。只需包含一次，无需再包含各组件头文件：

```cpp
#include "somtparser/parser.h"
using namespace SOMTParser;
// Parser parser; ...
```

以下示例均假定已包含上述头文件（并可选用 `using namespace SOMTParser`）。

### 基本解析与表达式构造

参见 [README.md](README.md) 中 “Basic Parsing and Expression Building” 与 “Expression Building” 的代码示例。

### 公式分析、格式转换、模型求值、Let 展开、OMT

参见 [README.md](README.md) 中 “Formula Analysis”“Formula Format Conversions”“Model Evaluation”“Let Expression Expansion”“Optimization Modulo Theories (OMT)” 等小节。

### 与论文一致的示例（代码与英文 README 相同）

**CNF 与布尔抽象**：在构建 CNF 后，可将理论原子与新增布尔变量建立双向映射（用于 CDCL(T) 类推理），例如用 `getCNFBoolVar` 取原子的抽象变量、用 `getCNFAtom` 从 CNF 布尔变量还原原子。

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    SOMTParser::Parser parser;
    parser.parseStr("(declare-const x Int)");
    parser.parseStr("(declare-const y Int)");

    SOMTParser::Node phi = parser.mkExpr("(and (> x 0) (< y 3))");
    SOMTParser::Node nnf = parser.toNNF(phi);
    SOMTParser::Node cnf = parser.toCNF({phi});

    SOMTParser::Node b = parser.getCNFBoolVar(parser.mkExpr("(> x 0)"));
    SOMTParser::Node a = parser.getCNFAtom(b);

    std::cout << "Atom from CNF bool var: " << parser.toString(a) << std::endl;
    return 0;
}
```

**解析模型字符串并求值**：用 `parseModel` 解析求解器输出的 model 字符串，再用 `evaluate` 在得到的模型下对公式求值。

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    SOMTParser::Parser parser;
    parser.parseStr("(declare-const x Int)");
    parser.parseStr("(declare-const y Int)");
    SOMTParser::Node phi = parser.mkExpr("(and (> x 0) (> y 0))");

    std::string modelStr = R"(
(model
  (define-fun x () Int 1)
  (define-fun y () Int 2)
))";

    auto M = parser.parseModel(modelStr);
    SOMTParser::Node psi = parser.evaluate(phi, M);
    std::cout << "Evaluated: " << parser.toString(psi) << std::endl;
    return 0;
}
```

**DAG 遍历（每节点至多访问一次）**：IR 为 DAG 时同一子项会多处出现；遍历接口保证每个不同节点只被访问一次。例如 `(and (or a b) (or a b))` 中共享子项 `(or a b)` 只访问一次，共 4 个节点。

```cpp
#include "somtparser/parser.h"
#include <iostream>
#include <map>

using namespace SOMTParser;

class KindCounter : public NodeVisitor {
public:
    void visit(Node n) override { counts[kind(n)]++; }
    std::map<NODE_KIND, size_t> counts;
};

int main() {
    Parser parser;
    parser.parseStr("(declare-const a Bool)");
    parser.parseStr("(declare-const b Bool)");
    Node phi = parser.mkExpr("(and (or a b) (or a b))");

    KindCounter c;
    c.walk(phi);
    std::cout << "Total nodes (visit-once): " << c.counts.size() << " kinds" << std::endl;
    return 0;
}
```

**重写**：重写器自底向上变换公式；安装默认规则并做不动点重写后，`(and true (not (not p)))` 会简化为 `p`。

```cpp
#include "somtparser/parser.h"
#include <iostream>

using namespace SOMTParser;

int main() {
    Parser parser;
    parser.parseStr("(declare-const p Bool)");
    parser.parseStr("(assert (and true (not (not p))))");

    Rewriter rw(parser.getNodeManager());
    installDefaultRewriteRules(rw);

    Node result = rw.rewrite(parser.getAssertions().back());
    std::cout << "Rewritten to: " << parser.toString(result) << std::endl;
    return 0;
}
```

**按 kind 分发（项深度）**：用 `OpDispatcher` 按节点 kind 注册处理函数，`otherwise` 提供默认递归行为；例如可维护一个携带 dispatcher 的 Context，递归计算项深度。

```cpp
#include "somtparser/parser.h"
#include <iostream>

using namespace SOMTParser;

struct DepthContext : Context {
    OpDispatcher<int, Context>* disp = nullptr;
};

static int depthDefault(Node n, Context& ctx) {
    auto& dc = static_cast<DepthContext&>(ctx);
    int d = 0;
    for (Node c : children(n))
        if (c) d = std::max(d, dc.disp->dispatch(c, ctx));
    return 1 + d;
}

int main() {
    Parser parser;
    parser.parseStr("(declare-const x Int)");
    parser.parseStr("(assert (and (> x 0) (< x 10)))");
    Node root = parser.getAssertions().back();

    OpDispatcher<int, Context> disp;
    disp.onAND(depthDefault).onGT(depthDefault).onLT(depthDefault).otherwise(depthDefault);
    DepthContext dctx;
    dctx.disp = &disp;
    int depth = disp.dispatch(root, dctx);
    std::cout << "Term depth: " << depth << std::endl;
    return 0;
}
```

### 性能与实现要点

- **DAG 与规范化**：通过 `NodeManager` 在构造时做 interning，相同结构的子项自动共享，IR 更紧凑
- **部分模型求值**：支持在部分模型下对公式/项求值，未完全确定的子项可被化简
- **Tseitin CNF**：便于与 SAT/CDCL(T) 类推理对接的公式转换
- **高精度算术**：可配置的 MPFR 精度
- **重写**：支持单轮与不动点模式，重写结果经 `NodeManager` 重建以保持规范化

## API 文档

使用 Doxygen 生成详细 API 文档：

```bash
doxygen Doxyfile
```

然后在浏览器中打开 `docs/html/index.html`。

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE)。

## 贡献

欢迎贡献，详见 [CONTRIBUTORS.md](CONTRIBUTORS.md)。

## 评估与适用场景

本库定位为**仅前端**：不包含求解器，评估也仅针对解析与 IR 构建的鲁棒性与开销。在 SMT-COMP 2025 的 QF_AX、QF_BV、QF_FP、QF_LIA、QF_LRA、QF_NIA、QF_NRA、QF_S 等基准族上，与 Z3、cvc5、smt-switch、pySMT、ANTLR4、jSMTLIB 等前端对比显示：SOMTParser 在各逻辑族上成功率高、失败率为 0%，且构造的 AST 节点数通常更少（结构共享与规范化带来更紧凑的 IR）。详见论文中的实验部分。

## 参考文献

若在学术工作中使用本库，可引用：

- **SOMTParser: A Solver-Independent Front-End for SMT and OMT Tools**  
  Fuqi Jia, Rui Han, Kunhang Lv, Sicheng Tan, Feifei Ma, Jian Zhang.

## 开发状态

**积极维护中** — 项目持续维护与开发，会定期增加新特性与优化。

## 联系

技术问题或支持可联系：

**贾富琦 (Fuqi Jia)**  
Email: jiafq@ios.ac.cn  
中国科学院软件研究所

---

*完整安装步骤、配置说明、API 列表及代码示例请参阅英文版 [README.md](README.md)。*
