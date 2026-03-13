# SOMTParser

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**SOMTParser** is a solver-independent C++17 front-end library for SMT-LIB and OMT (Optimization Modulo Theories). It parses input into a **typed DAG-based intermediate representation (IR)** and provides modular front-end functionality—parsing, type checking, traversals, rewriting, and formula-level processing—around this IR, separating reusable front-end logic from backend reasoning and reducing the effort to build new solvers and SMT-based tools.

## Design Overview

- **Frontend**: Command and term parsing, type checking, OMT-oriented input, and API-level expression construction feeding the same IR pipeline.
- **IR core**: A unified typed DAG representation with structure sharing and canonicalization via `NodeManager`; equal subterms are stored once.
- **Passes**: IR-based traversal (each node visited at most once) and rewriting (bottom-up, with fixpoint mode), plus a kind-based dispatch mechanism for extensions.
- **Utilities**: Formula conversion (NNF, CNF, DNF, including CDCL(T)-style Boolean abstraction), model parsing, and evaluation under full or partial models.

## Key Features

- **SMT-LIB2 support**: Compliant with the SMT-LIB2 specification; multiple logics and theories.
- **Multi-theory**: Booleans, integer/real arithmetic, bitvectors, IEEE-754 floating point, strings and regular expressions, arrays.
- **Typed DAG IR**: Structure sharing and canonicalization for a more compact representation and easier analysis/transformation.
- **OMT support**: Parses intermediate syntax such as `assert-soft`, `maximize`/`minimize`, `define-objective`, `lex-optimize`/`pareto-optimize`/`box-optimize`, `maxsat`/`minsat`, managed by `ObjectiveManager`.
- **Programmatic construction**: Build terms and formulas via API, sharing the same IR as parsed input.
- **Formula conversion**: NNF, CNF, DNF; CNF supports theory-atom-to-Boolean abstraction and bidirectional mapping for CDCL(T)-style reasoning.
- **Model interface**: Parse solver-produced model output and evaluate formulas/terms under full or partial models.
- **Rewriting and traversal**: Extensible rewrite rules, kind-based dispatcher, visit-once traversal over the DAG.

## System Requirements

- C++17 compatible compiler
- CMake 3.10+
- GMP (GNU Multiple Precision Arithmetic Library)
- MPFR (GNU Multiple Precision Floating-Point Reliable Library)

### Installing Dependencies

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
Using [Homebrew](https://brew.sh/):
```bash
brew install \
  cmake \
  gmp \
  mpfr
```

#### Windows

##### Using MSYS2
1. Install [MSYS2](https://www.msys2.org/)
2. Open MSYS2 MinGW 64-bit terminal and run:
```bash
pacman -Syu
pacman -S \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-gmp \
  mingw-w64-x86_64-mpfr
```

##### Using vcpkg
1. Install [vcpkg](https://github.com/microsoft/vcpkg)
2. Install dependencies:
```bash
vcpkg install gmp:x64-windows mpfr:x64-windows
```

##### Using WSL (Windows Subsystem for Linux)
1. Install and set up [WSL](https://learn.microsoft.com/en-us/windows/wsl/install)
2. Follow the Ubuntu/Debian instructions to install dependencies within WSL

## Installation

### Standard CMake Build Process

```bash
# Clone the repository
git clone https://github.com/fuqi-jia/SOMTParser.git
cd SOMTParser

# Create and enter build directory
mkdir build && cd build

# Configure the build
cmake ..

# Compile the library (utilizing all available cores)
make -j$(nproc)

# Install the library (may require administrative privileges)
sudo make install
```

### Integration as Git Submodule

For projects using Git, SOMTParser can be included as a submodule:

```bash
# Add the repository as a submodule
git submodule add https://github.com/fuqi-jia/SOMTParser.git SOMTParser

# Initialize the submodule
git submodule update --init --recursive

# Update the submodule to the latest version when needed
git submodule update --remote --merge
```

### Python Installation

SOMTParser also provides Python bindings. Install with pip:

```bash
# Requires Python 3.9+ and system dependencies (GMP, MPFR)
pip install .

# Or install with test dependencies
pip install ".[test]"
```

#### Quick Start (Python)

```python
import somtparser as sp

# Parse SMT-LIB2 text
p = sp.parse("(set-logic QF_LIA)(declare-const x Int)(assert (> x 0))")

# Access assertions
for node in p.assertions:
    print(node.kind, node.sort)

# Build expressions programmatically
x = p.var_int("x")
y = p.var_int("y")
sum_expr = p.add(x, y)
constraint = p.eq(sum_expr, p.const_int("10"))

# Iterate over children
for child in sum_expr:
    print(child.name)
```

### Build TEST

To build and run the tests:

```bash
# Create and enter build directory
mkdir -p build && cd build

# Configure with tests enabled
cmake .. -DBUILD_TESTS=ON

# Build the project and tests
make -j$(nproc)

# Run all tests
cd test
for test in test_*; do ./$test; done

# Alternatively, run individual tests
./test_parser
./test_string_handling
```

You can also use the provided test script from the project root:

```bash
./test/run_tests.sh
```

### Build Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_SHARED_LIBS` | Build shared libraries (.so/.dll) | OFF |
| `BUILD_BOTH_LIBS` | Build both static (.a/.lib) and shared libraries | ON |
| `BUILD_TESTS` | Build test executables | OFF |
| `ENABLE_DEBUG_SYMBOLS` | Enable debug symbols in the build for debugging purposes | OFF |

To customize the build configuration:

```bash
cmake -DBUILD_SHARED_LIBS=ON -DBUILD_BOTH_LIBS=OFF ..
```

### Compiling Client Applications

When building applications that use SOMTParser, link against the library and its dependencies:

```bash
g++ -std=c++17 -o application main.cpp -lsomtparser -lgmp -lmpfr
```

For CMake projects, you can use:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(your_application)

set(CMAKE_CXX_STANDARD 17)

# Find required dependencies
find_package(PkgConfig REQUIRED)
pkg_check_modules(GMP REQUIRED gmp)
pkg_check_modules(MPFR REQUIRED mpfr)

# Method 1: Using as Git submodule (recommended)
add_subdirectory(SOMTParser)

# Method 2: If SOMTParser is installed system-wide (alternative)
# find_library(SOMTPARSER_LIB somtparser REQUIRED)
# find_path(SOMTPARSER_INCLUDE_DIR parser.h PATH_SUFFIXES somtparser)

# Create your executable
add_executable(your_application main.cpp)

# Link libraries and set include directories
target_link_libraries(your_application 
    somtparser               # SOMTParser target from submodule
    ${GMP_LIBRARIES} 
    ${MPFR_LIBRARIES}
)

target_include_directories(your_application PRIVATE 
    ${GMP_INCLUDE_DIRS} 
    ${MPFR_INCLUDE_DIRS}
)

# Note: SOMTParser headers are automatically included when using add_subdirectory
```

## Configuration Options

SOMTParser provides various configuration options through the `GlobalOptions` class to control parsing behavior, evaluation settings, and other aspects.

### Quick Start

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();

    // Set options via direct methods
    parser->getOptions()->setLogic("QF_LIA");
    parser->getOptions()->setKeepLet(false);
    parser->getOptions()->setEvaluatePrecision(256);

    // Or via string interface
    parser->getOptions()->setOption("keep_let", "false");
    parser->getOptions()->setOption("precision", "256");

    // Print detailed configuration report
    std::cout << parser->optionToString() << std::endl;
    return 0;
}
```

### Available Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| **logic** | string | `UNKNOWN_LOGIC` | SMT-LIB2 logic (e.g., `QF_LIA`, `QF_BV`, `ALL`) |
| **precision** | uint | `128` | MPFR floating-point precision in bits |
| **float_evaluate** | bool | `true` | Use floating-point (true) or exact rational (false) arithmetic |
| **keep_division** | bool | `true` | Preserve division if not exact (e.g., `(/ 5 2)` stays as-is) |
| **keep_let** | bool | `true` | Preserve let-bindings instead of expanding inline |
| **expand_functions** | bool | `false` | When true, inline function calls with definitions; when false (default), preserve as function applications |
| **Command Flags** | bool | `false` | Tracks encountered SMT-LIB2 commands: `check_sat`, `get_model`, `get_assertions`, `get_proof`, `get_unsat_core`, `get_objectives`, etc. |

<!-- 
**Planned Feature:**
| **expand_recursive_functions** | bool | `false` | Expand `define-fun-rec` like regular functions |
This feature is planned for future implementation. It will use a 'this' placeholder mechanism to handle 
recursive self-references during function body parsing and expansion. Currently, recursive functions 
are preserved as-is in their original form.
-->

### Setting Options

```cpp
#include "somtparser/parser.h"

int main() {
    auto parser = SOMTParser::newParser();

    // Method 1: Direct setters (recommended)
    parser->getOptions()->setLogic("QF_LIA");
    parser->getOptions()->setEvaluatePrecision(256);
    parser->getOptions()->setKeepLet(false);
    parser->getOptions()->setExpandFunctions(false);  // Preserve function applications

    // Method 2: String interface (useful for SMT-LIB2 compatibility)
    parser->setOption("precision", "256");
    parser->setOption("keep_let", "false");
    parser->setOption("expand_functions", "false");

    return 0;
}
```

### Configuration Report

The `toString()` method generates a comprehensive report with all settings, defaults, and descriptions:

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();
    std::cout << parser->optionToString() << std::endl;
    return 0;
}
```

## API Reference

### Core Components and Smart Pointer Types

| Component | Smart Pointer Type | Description |
|-----------|-------------------|-----------|
| `Parser` | `ParserPtr` | Main class for parsing SMT-LIB2 files and managing expressions |
| `DAGNode` | `NodePtr` | Represents expressions as nodes in a "directed acyclic graph" |
| `Sort` | `SortPtr` | Encapsulates SMT-LIB2 type system |
| `Objective` | `ObjectivePtr` | Represents optimization objectives for OMT problems |
| `SingleObjective` | `SingleObjectivePtr` | Represents a single optimization objective |
| `MetaObjective` | `MetaObjectivePtr` | Represents a meta-optimization objective |
| `Model` | `ModelPtr` | Represents a model with variable assignments |

### Factory Methods

| Method | Description |
|--------|-----------|
| `newParser()` | Creates a new parser instance |
| `newParser(const std::string& filename)` | Creates a new parser and parses the specified file |
| `newModel()` | Creates a new model instance |

### Primary API Functions

| Category | Function Examples |
|----------|------------------|
| **Variable Creation** | `mkVar`, `mkVarInt`, `mkVarReal`, `mkVarBool`, `mkTempVar`, ... |
| **Constant Creation** | `mkConstInt`, `mkConstReal`, `mkTrue`, `mkFalse`, `mkConstStr`, ... |
| **Expression Building** | `mkOper`, `mkEq`, `mkDistinct`, `mkNot`, `mkAnd`, `mkOr`, `mkIte`, ... |
| **Arithmetic Operations** | `mkAdd`, `mkSub`, `mkMul`, `mkDiv`, `mkPow`, `mkAbs`, `mkMod`, ... |
| **Comparison Operations** | `mkLt`, `mkLe`, `mkGt`, `mkGe`, ... |
| **Bitvector Operations** | `mkBvAnd`, `mkBvOr`, `mkBvXor`, `mkBvAdd`, `mkBvShl`, `mkBvLshr`, ... |
| **String Operations** | `mkStrLen`, `mkStrConcat`, `mkStrSubstr`, `mkStrIndexof`, ... |
| **Regular Expression Operations** | `mkStrToReg`, `mkRegUnion`, `mkRegStar`, `mkRegInter`, ... |
| **Array Operations** | `mkSelect`, `mkStore`, ... |
| **Floating Point Operations** | `mkFpAdd`, `mkFpMul`, `mkFpDiv`, `mkFpEq`, `mkFpLt`, ... |
| **Formula Evaluation** | `evaluate`, `setEvaluatePrecision`, `setEvaluateUseFloating`, ... |
| **Format Conversion** | `toCNF`, `toDNF`, `toNNF`, `toTseitinCNF`, ... |
| **Formula Analysis** | `collectAtoms`, `collectVars`, `expandLet`, `replaceAtoms`, ... |
| **Model Operations** | `add`, `get`, `isEmpty`, `toString`, ... |
| **Debugging & Output** | `toString`, `getAssertions`, ... |

## Usage Examples

All public APIs (parser, passes, visitor, rewriter, op-dispatcher, etc.) are exposed through a single **umbrella header**. Include it once; no need to include individual component headers.

```cpp
#include "somtparser/parser.h"
using namespace SOMTParser;
// Parser parser; ...
```

The examples below assume this include (and optionally `using namespace SOMTParser`).

### Basic Parsing and Expression Building

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    // Initialize the parser
    SOMTParser::Parser parser;

    // Parse an SMT-LIB2 file
    if (!parser.parse("formula.smt2")) {
        std::cerr << "Error parsing file" << std::endl;
        return 1;
    }

    // Retrieve the parsed assertions
    auto assertions = parser.getAssertions();

    // Output the assertions
    for(auto constraint: assertions){
        std::cout << parser.toString(constraint) << std::endl;
    }

    return 0;
}
```

### Expression Building

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();
    
    // Create variables and expressions
    auto x = parser->mkVarInt("x");
    auto y = parser->mkVarInt("y");
    auto sum = parser->mkAdd(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{x, y});
    auto condition = parser->mkGt(sum, parser->mkConstInt(10));
    
    std::cout << "Sum expression: " << parser->toString(sum) << std::endl;
    std::cout << "Condition: " << parser->toString(condition) << std::endl;
    
    return 0;
}
```





## Some Useful Features

### Formula Analysis

Analysis and manipulation of formulas:

```cpp
#include "somtparser/parser.h"
#include <iostream>
#include <unordered_set>

int main() {
    auto parser = SOMTParser::newParser();
    
    // Create variables and build complex formula
    auto x = parser->mkVarInt("x");
    auto y = parser->mkVarInt("y");
    auto z = parser->mkVarBool("z");
    
    std::vector<std::shared_ptr<SOMTParser::DAGNode>> parts = {
        parser->mkGt(parser->mkAdd(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{x, y}), parser->mkConstInt(0)),
        parser->mkLt(x, parser->mkConstInt(10)),
        z
    };
    auto formula = parser->mkAnd(parts);
    
    // Collect atoms and variables
    std::unordered_set<std::shared_ptr<SOMTParser::DAGNode>> atoms, vars;
    parser->collectAtoms(formula, atoms);
    parser->collectVars(formula, vars);
    
    std::cout << "Formula: " << parser->toString(formula) << std::endl;
    // Output: (and (< x 10) (> (+ y x) 0) z)
    
    std::cout << "Found " << atoms.size() << " atoms and " << vars.size() << " variables" << std::endl;
    // Output: Found 2 atoms and 3 variables
    
    return 0;
}
```

### Formula Format Conversions

Convert formulas between different normal forms:

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();
    auto a = parser->mkVarBool("a");
    auto b = parser->mkVarBool("b");
    auto c = parser->mkVarBool("c");

    // Build complex formula
    auto formula = parser->mkAnd(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{
        parser->mkImplies(a, b),
        parser->mkOr(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{b, c})
    });

    // Convert to different normal forms
    auto nnf = parser->toNNF(formula);
    auto cnf = parser->toCNF(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{formula});
    auto dnf = parser->toDNF(formula);

    std::cout << "Original: " << parser->toString(formula) << std::endl;
    std::cout << "NNF: " << parser->toString(nnf) << std::endl;
    std::cout << "CNF: " << parser->toString(cnf) << std::endl;
    std::cout << "DNF: " << parser->toString(dnf) << std::endl;
    return 0;
}
```

### CNF and Boolean abstraction

The CNF conversion can associate theory atoms with fresh Boolean variables and maintain the mapping in both directions (useful for CDCL(T)-style reasoning). After building CNF, you can get the abstraction variable for an atom and recover the atom from a CNF Boolean variable:

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    SOMTParser::Parser parser;
    parser.parseStr("(declare-const x Int)");
    parser.parseStr("(declare-const y Int)");

    SOMTParser::Node phi = parser.mkExpr("(and (> x 0) (< y 3))");
    SOMTParser::Node nnf = parser.toNNF(phi);
    SOMTParser::Node cnf = parser.toCNF({phi});  // vector of assertions

    SOMTParser::Node b = parser.getCNFBoolVar(parser.mkExpr("(> x 0)"));
    SOMTParser::Node a = parser.getCNFAtom(b);   // recovers (> x 0)

    std::cout << "Atom from CNF bool var: " << parser.toString(a) << std::endl;
    return 0;
}
```

### Model Evaluation

Evaluate logical formulas with mathematical functions and variable assignments:

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();
    auto model = SOMTParser::newModel();

    // Create a formula: (sin(x) > 0) ∧ (y > 1) ∧ (z ⟹ (x + y > 3))
    auto x = parser->mkVarReal("x");
    auto y = parser->mkVarReal("y");
    auto z = parser->mkVarBool("z");

    auto sin_x = parser->mkSin(x);
    auto cond1 = parser->mkGt(sin_x, parser->mkConstReal(std::string("0")));
    auto cond2 = parser->mkGt(y, parser->mkConstReal(std::string("1")));
    auto sum_xy = parser->mkAdd(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{x, y});
    auto cond3 = parser->mkImplies(z, parser->mkGt(sum_xy, parser->mkConstReal(std::string("3"))));
    auto formula = parser->mkAnd(std::vector<std::shared_ptr<SOMTParser::DAGNode>>{cond1, cond2, cond3});

    // Assign values and evaluate
    model->add(x, parser->mkConstReal(std::string("1.5")));
    model->add(y, parser->mkConstReal(std::string("2.0")));
    model->add(z, parser->mkTrue());

    auto result = parser->evaluate(formula, model);
    std::cout << "Formula: " << parser->toString(formula) << std::endl;
    std::cout << "Result: " << parser->toString(result) << std::endl;
    return 0;
}
```

You can also parse solver-produced model output and then evaluate a formula under that model:

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

### Let Expression Expansion

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();
    auto x = parser->mkVarInt("x");
    auto y = parser->mkVarInt("y");
    auto let_expr = parser->mkExpr("(let ((temp (+ x 1))) (> temp y))");
    auto expanded = parser->expandLet(let_expr);

    std::cout << "Let expression: " << parser->toString(let_expr) << std::endl;
    std::cout << "Expanded: " << parser->toString(expanded) << std::endl;
    return 0;
}
```

### Optimization Modulo Theories (OMT)

```cpp
#include "somtparser/parser.h"
#include <iostream>

int main() {
    auto parser = SOMTParser::newParser();

    parser->parseStr("(declare-const x Int)");
    parser->parseStr("(declare-const y Int)");
    parser->parseStr("(assert (> x 0))");
    parser->parseStr("(assert (> y 0))");
    parser->parseStr("(assert-soft (< x 100) :weight 1.0)");
    parser->parseStr("(assert-soft (< y 50) :weight 2.0)");

    auto soft_assertions = parser->getSoftAssertions();
    auto soft_weights = parser->getSoftWeights();

    std::cout << "Found " << soft_assertions.size() << " soft assertions" << std::endl;
    for (size_t i = 0; i < soft_assertions.size(); ++i) {
        std::cout << "Soft assertion " << i << " (weight " << soft_weights[i] << "): "
                  << parser->toString(soft_assertions[i]) << std::endl;
    }
    return 0;
}
```

### DAG traversal (visit-once)

Because the IR is a DAG, the same subterm may appear in many places. The traversal API visits each distinct node at most once. For example, for `(and (or a b) (or a b))` the shared subterm `(or a b)` is visited once, so the walk sees exactly 4 nodes:

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

### Rewriting

The rewriter performs bottom-up transformations; you can install default rules and run fixpoint rewriting so that a formula like `(and true (not (not p)))` simplifies to `p`:

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

### Kind-based dispatch (term depth)

Analysis or transformation logic can be organized by node kind using `OpDispatcher`: register handlers per kind and use `otherwise` for the default recursive behavior. Below, a small context holds the dispatcher so handlers can recurse to compute term depth:

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

### Key Performance and Implementation Notes

- **DAG and canonicalization**: `NodeManager` interns nodes at construction time; structurally equal subterms are shared for a more compact IR.
- **Partial model evaluation**: Evaluate formulas/terms under partial models; under-specified subterms can be simplified.
- **Tseitin CNF**: Formula conversion suited for integration with SAT/CDCL(T)-style reasoning.
- **High-precision arithmetic**: Configurable MPFR precision.
- **Rewriting**: Single-round and fixpoint modes; rewritten results are rebuilt via `NodeManager` to preserve canonicalization.

## API Documentation

You can generate detailed API documentation using Doxygen:

### Installing Doxygen

#### Ubuntu/Debian
```bash
sudo apt-get install doxygen
```

#### CentOS/RHEL
```bash
sudo yum install doxygen
```

#### macOS
```bash
brew install doxygen
```

### Generating Documentation

To generate the documentation, run:

```bash
cd path/to/SOMTParser
doxygen Doxyfile
```

### Viewing Documentation

After generating the documentation, you can view it in your browser:

```bash
# Linux
xdg-open docs/html/index.html

# macOS
open docs/html/index.html

# Windows
start docs/html/index.html
```

Or simply navigate to `SOMTParser/docs/html/index.html` in your file browser and open it with any web browser.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

We welcome contributions from the community! Please see our [CONTRIBUTORS.md](CONTRIBUTORS.md) file for detailed information on how to contribute to this project.

## Development Status

**Active Development** — The project is under continuous maintenance and development; new features and optimizations are added regularly.

## Contact

For technical inquiries or support, please contact:

**Fuqi Jia**  
Email: jiafq@ios.ac.cn  
Institute of Software, Chinese Academy of Sciences

