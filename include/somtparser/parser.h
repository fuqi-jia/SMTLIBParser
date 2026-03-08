/* -*- Header -*-
 * Umbrella header: include this for the full SOMTParser API.
 * Order: core -> ir -> context -> passes -> frontend -> model.
 * Each header is self-contained; this order is for clarity only.
 */
#pragma once

#include "somtparser/core/asserting.h"
#include "somtparser/core/kind.h"
#include "somtparser/core/options.h"
#include "somtparser/core/timing.h"
#include "somtparser/core/util.h"
#include "somtparser/ir/sort.h"
#include "somtparser/ir/number.h"
#include "somtparser/ir/value.h"
#include "somtparser/ir/interval.h"
#include "somtparser/ir/dag.h"
#include "somtparser/ir/node.h"
#include "somtparser/context/context.h"
#include "somtparser/passes/op_dispatcher.h"
#include "somtparser/passes/visitor.h"
#include "somtparser/passes/rewriter.h"
#include "somtparser/frontend/parser_context.h"
#include "somtparser/frontend/symbol_manager.h"
#include "somtparser/frontend/objective.h"
#include "somtparser/frontend/op_utils.h"
#include "somtparser/frontend/parser.h"
#include "somtparser/model/model.h"
