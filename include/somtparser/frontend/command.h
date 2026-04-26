/* -*- Header -*-
 *
 * Command and Script classes for sequential / incremental SMT-LIB parsing.
 *
 * Author: Fuqi Jia <jiafq@ios.ac.cn>
 *
 * Copyright (C) 2025 Fuqi Jia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef COMMAND_HEADER
#define COMMAND_HEADER

#include "somtparser/ir/dag.h"
#include "somtparser/ir/sort.h"

#include <memory>
#include <string>
#include <vector>

namespace SOMTParser {

// Forward declaration from parser.h
enum class CMD_TYPE;

/**
 * @brief Unified struct capturing the effect of a parsed SMT-LIB command.
 *
 * Populated as a side-effect of parsing. Not all fields are valid for all command types.
 */
struct Command {
    CMD_TYPE type;
    size_t line_number = 0;

    // Fields populated depending on type:
    std::shared_ptr<DAGNode> expr;                           // assert, maximize, minimize, etc.
    std::string name;                                        // declare-const, declare-fun, define-sort, etc.
    std::shared_ptr<Sort> sort;                              // declare-const, declare-fun
    std::vector<std::shared_ptr<DAGNode>> params;            // define-fun params
    std::string group_id;                                    // assert :id, assert-soft
    std::shared_ptr<DAGNode> weight;                         // assert-soft weight
    size_t push_pop_level = 1;                               // push, pop
    std::string logic;                                       // set-logic

    Command(CMD_TYPE t = CMD_TYPE(0)) : type(t) {} // initialized with default CT_UNKNOWN=0

    bool isAssert() const;
    bool isPush() const;
    bool isPop() const;
    bool isReset() const;
    bool isResetAssertions() const;
};

/**
 * @brief Container for an ordered sequence of parsed commands.
 */
class Script {
    std::vector<Command> commands_;

public:
    void addCommand(const Command& cmd);
    void addCommand(Command&& cmd);
    const std::vector<Command>& commands() const;
    size_t size() const;
    const Command& operator[](size_t i) const;
    void clear();
};

} // namespace SOMTParser

#endif
