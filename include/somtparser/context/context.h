/* -*- Header -*-
 *
 * Context base for IR / Dispatcher (service layer).
 *
 * Context: base holding NodeManager, SortManager, Options only.
 * No SymbolManager/ObjectiveManager — see ParserContext in parser_context.h for parser/frontend.
 * NullContext: empty context for dispatch(Node) default; managers remain nullptr.
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

#ifndef CONTEXT_HEADER
#define CONTEXT_HEADER

#include "somtparser/ir/dag.h"
#include "somtparser/core/options.h"
#include "somtparser/ir/sort.h"

#include <memory>

namespace SOMTParser {

/**
 * Base context: holds NodeManager, SortManager, Options only.
 * Used by passes (e.g. Rewriter), dispatcher; does not depend on symbol/objective.
 */
class Context {
protected:
    std::shared_ptr<NodeManager>   node_manager_;
    std::shared_ptr<SortManager>   sort_manager_;
    std::shared_ptr<GlobalOptions> options_;

public:
    virtual ~Context() = default;

    void setNodeManager(std::shared_ptr<NodeManager> nm) { node_manager_ = std::move(nm); }
    void setSortManager(std::shared_ptr<SortManager> sm) { sort_manager_ = std::move(sm); }
    void setOptions(std::shared_ptr<GlobalOptions> opt) { options_ = std::move(opt); }

    std::shared_ptr<NodeManager> getNodeManager() { return node_manager_; }
    std::shared_ptr<NodeManager> getNodeManager() const { return node_manager_; }
    std::shared_ptr<SortManager> getSortManager() { return sort_manager_; }
    std::shared_ptr<SortManager> getSortManager() const { return sort_manager_; }
    std::shared_ptr<GlobalOptions> getOptions() { return options_; }
    std::shared_ptr<GlobalOptions> getOptions() const { return options_; }
};

/** Empty context for dispatch(Node) default; no get* (base has none). */
class NullContext : public Context {
};

} // namespace SOMTParser

#endif
