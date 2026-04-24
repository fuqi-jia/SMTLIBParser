/* -*- Header -*-
 *
 * OpDispatcher-based evaluation dispatch table.
 * Replaces the 157-branch if-else chain in Parser::evaluate().
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

#ifndef EVAL_DISPATCH_HEADER
#define EVAL_DISPATCH_HEADER

#include "somtparser/passes/op_dispatcher.h"
#include "somtparser/model/model.h"

namespace SOMTParser {

class Parser;  // forward declaration

/** Context passed through OpDispatcher to every evaluate handler. */
struct EvalContext : public Context {
    Parser*                         parser;
    std::shared_ptr<Model>          model;
    std::shared_ptr<DAGNode>*       result;   // output pointer

    EvalContext(Parser* p, std::shared_ptr<Model> m, std::shared_ptr<DAGNode>* r)
        : parser(p), model(std::move(m)), result(r) {}
};

/** Build the singleton dispatch table. Thread-safe after init. */
const OpDispatcher<bool, EvalContext>& getEvalDispatcher();

} // namespace SOMTParser

#endif
