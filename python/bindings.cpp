/**
 * Python bindings for SOMTParser using pybind11
 * 
 * This file provides Pythonic bindings for the SOMTParser C++ library,
 * exposing Parser, Node, Sort, Model, and Objective classes with
 * Python protocols support (__iter__, __getitem__, __contains__, etc.)
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include <somtparser/parser.h>

namespace py = pybind11;
using namespace SOMTParser;

// Type aliases for convenience
using NodePtr = std::shared_ptr<DAGNode>;
using SortPtr = std::shared_ptr<Sort>;
using ModelPtr = std::shared_ptr<Model>;

PYBIND11_MODULE(_somtparser, m) {
    m.doc() = "SOMTParser: SMT/OMT Parser Library - Python Bindings";
    
    // ===== Sort =====
    py::class_<Sort, std::shared_ptr<Sort>>(m, "Sort", "SMT-LIB2 sort (type) representation")
        .def_property_readonly("kind", [](const Sort& s) { 
            return s.toString(); 
        }, "Sort kind as string")
        .def_property_readonly("is_bool", &Sort::isBool, "Check if Bool sort")
        .def_property_readonly("is_int", &Sort::isInt, "Check if Int sort")
        .def_property_readonly("is_real", &Sort::isReal, "Check if Real sort")
        .def_property_readonly("is_int_or_real", &Sort::isIntOrReal, 
            "Check if IntOrReal sort (numeric literal type)")
        .def_property_readonly("is_bv", &Sort::isBv, "Check if BitVec sort")
        .def_property_readonly("is_fp", &Sort::isFp, "Check if FloatingPoint sort")
        .def_property_readonly("is_string", &Sort::isStr, "Check if String sort")
        .def_property_readonly("is_array", &Sort::isArray, "Check if Array sort")
        .def_property_readonly("bv_width", &Sort::getBitWidth, "BitVec width (0 if not BV)")
        .def("__repr__", &Sort::toString)
        .def("__eq__", &Sort::operator==);
    
    // ===== Node (DAGNode) =====
    py::class_<DAGNode, std::shared_ptr<DAGNode>>(m, "Node", "SMT-LIB2 expression node")
        // Properties (read-only)
        .def_property_readonly("kind", [](const DAGNode& n) { 
            return kindToString(n.getKind()); 
        }, "Node kind as string (e.g., 'and', 'add', 'var')")
        .def_property_readonly("sort", &DAGNode::getSort, "Sort of this node")
        .def_property_readonly("name", &DAGNode::getName, "Name (for variables/constants)")
        .def_property_readonly("num_children", [](const DAGNode& n) {
            return n.getChildrenSize();
        }, "Number of children")
        
        // Python sequence protocol
        .def("__len__", [](const DAGNode& n) {
            return n.getChildrenSize();
        })
        .def("__getitem__", [](const DAGNode& n, int i) {
            int size = static_cast<int>(n.getChildrenSize());
            if (i < 0) i += size;
            if (i < 0 || i >= size) {
                throw py::index_error("Node index out of range");
            }
            return n.getChild(i);
        }, py::arg("index"), "Get child by index (supports negative indexing)")
        .def("__iter__", [](const DAGNode& n) {
            // Return children as a Python list to avoid iterator lifetime issues
            py::list result;
            for (const auto& child : n.getChildren()) {
                result.append(child);
            }
            return py::iter(result);
        }, "Iterate over children")
        
        // String representation
        .def("__repr__", &DAGNode::toString)
        .def("__hash__", [](const DAGNode& n) {
            return static_cast<size_t>(n.hashCode());
        })
        .def("__eq__", [](const DAGNode& a, const DAGNode& b) {
            return a.isEquivalentTo(b);
        })
        
        // Type checks (commonly used)
        .def_property_readonly("is_const", &DAGNode::isConst, "Is a constant")
        .def_property_readonly("is_var", &DAGNode::isVar, "Is a variable")
        .def_property_readonly("is_leaf", &DAGNode::isLeaf, "Is a leaf node")
        
        // Boolean checks
        .def_property_readonly("is_true", &DAGNode::isTrue)
        .def_property_readonly("is_false", &DAGNode::isFalse)
        .def_property_readonly("is_and", &DAGNode::isAnd)
        .def_property_readonly("is_or", &DAGNode::isOr)
        .def_property_readonly("is_not", &DAGNode::isNot)
        .def_property_readonly("is_implies", &DAGNode::isImplies)
        .def_property_readonly("is_xor", &DAGNode::isXor)
        .def_property_readonly("is_ite", &DAGNode::isIte)
        
        // Comparison checks
        .def_property_readonly("is_eq", &DAGNode::isEq)
        .def_property_readonly("is_distinct", &DAGNode::isDistinct)
        .def_property_readonly("is_lt", &DAGNode::isLt)
        .def_property_readonly("is_le", &DAGNode::isLe)
        .def_property_readonly("is_gt", &DAGNode::isGt)
        .def_property_readonly("is_ge", &DAGNode::isGe)
        
        // Arithmetic checks
        .def_property_readonly("is_add", &DAGNode::isAdd)
        .def_property_readonly("is_sub", &DAGNode::isSub)
        .def_property_readonly("is_mul", &DAGNode::isMul)
        .def_property_readonly("is_neg", &DAGNode::isNeg)
        
        // Methods
        .def("to_smt2", [](const std::shared_ptr<DAGNode>& n) { 
            return dumpSMTLIB2(n); 
        }, "Convert to SMT-LIB2 string")
        .def("children", &DAGNode::getChildren, "Get list of children");
    
    // ===== Model =====
    py::class_<Model, std::shared_ptr<Model>>(m, "Model", "SMT model mapping variables to values")
        .def(py::init<>())
        
        // Dict-like protocol
        .def("__getitem__", [](Model& m, const std::string& name) {
            auto v = m.get(name);
            if (!v) {
                throw py::key_error(name);
            }
            return v;
        }, py::arg("name"), "Get value by variable name")
        .def("__contains__", [](Model& m, const std::string& name) {
            return m.get(name) != nullptr;
        }, py::arg("name"), "Check if variable is in model")
        .def("__len__", &Model::size, "Number of variable assignments")
        
        // Methods
        .def("get", [](Model& m, const std::string& name, py::object default_val) {
            auto v = m.get(name);
            if (!v) {
                return default_val;
            }
            return py::cast(v);
        }, py::arg("name"), py::arg("default") = py::none(), "Get value with default")
        .def("keys", [](Model& m) {
            auto pairs = m.getPairs();
            std::vector<std::string> keys;
            keys.reserve(pairs.size());
            for (const auto& p : pairs) {
                keys.push_back(p.first);
            }
            return keys;
        }, "Get all variable names")
        .def("values", [](Model& m) {
            return m.getValues();
        }, "Get all values")
        .def("items", &Model::getPairs, "Get all (name, value) pairs")
        
        .def("__repr__", &Model::toString);
    
    // ===== OptKind Enum =====
    py::enum_<OPT_KIND>(m, "OptKind", "Optimization objective kind")
        .value("MINIMIZE", OPT_KIND::OPT_MINIMIZE)
        .value("MAXIMIZE", OPT_KIND::OPT_MAXIMIZE)
        .value("MAXSAT", OPT_KIND::OPT_MAXSAT)
        .value("MINSAT", OPT_KIND::OPT_MINSAT)
        .value("LEX", OPT_KIND::OPT_LEX_OPTIMIZE)
        .value("PARETO", OPT_KIND::OPT_PARETO_OPTIMIZE)
        .value("BOX", OPT_KIND::OPT_BOX_OPTIMIZE)
        .export_values();
    
    // ===== Objective =====
    py::class_<MetaObjective, std::shared_ptr<MetaObjective>>(m, "Objective", "Optimization objective")
        .def_property_readonly("kind", &MetaObjective::getObjectiveKind, "Objective kind")
        .def_property_readonly("is_minimize", &MetaObjective::isMinimize)
        .def_property_readonly("is_maximize", &MetaObjective::isMaximize)
        .def_property_readonly("is_maxsat", &MetaObjective::isMaxSAT)
        .def_property_readonly("is_single", &MetaObjective::isSingleObjective)
        .def_property_readonly("is_multi", &MetaObjective::isMultiObjective)
        .def_property_readonly("term", [](const MetaObjective& obj) {
            try {
                return obj.getObjectiveTerm();
            } catch (...) {
                return std::shared_ptr<DAGNode>(nullptr);
            }
        }, "Objective term (for single objectives)")
        .def_property_readonly("group_id", &MetaObjective::getGroupID, "Group ID");
    
    // ===== Parser =====
    py::class_<Parser, std::shared_ptr<Parser>>(m, "Parser", "SMT-LIB2/OMT parser and expression builder")
        .def(py::init<>())
        .def(py::init<const std::string&>(), py::arg("filename"), "Create parser and parse file")
        
        // Parsing methods (return self for chaining)
        .def("parse", [](Parser& p, const std::string& path) -> Parser& {
            p.parse(path);
            return p;
        }, py::return_value_policy::reference, py::arg("path"), "Parse SMT-LIB2 file")
        .def("parse_string", [](Parser& p, const std::string& text) -> Parser& {
            p.parseStr(text);
            return p;
        }, py::return_value_policy::reference, py::arg("text"), "Parse SMT-LIB2 string")
        
        // Results as properties
        .def_property_readonly("assertions", &Parser::getAssertions, "List of assertions")
        .def_property_readonly("assumptions", &Parser::getAssumptions, "List of assumption groups")
        .def_property_readonly("soft_assertions", &Parser::getSoftAssertions, "List of soft assertions")
        .def_property_readonly("objectives", &Parser::getObjectives, "List of optimization objectives")
        .def_property_readonly("variables", &Parser::getVariables, "List of declared variables")
        .def_property_readonly("functions", &Parser::getFunctions, "List of declared functions")
        
        // Options
        .def("set_option", py::overload_cast<const std::string&, const std::string&>(
            &Parser::setOption), py::arg("key"), py::arg("value"), "Set parser option")
        .def("set_option", py::overload_cast<const std::string&, const bool&>(
            &Parser::setOption), py::arg("key"), py::arg("value"))
        .def("set_option", py::overload_cast<const std::string&, const int&>(
            &Parser::setOption), py::arg("key"), py::arg("value"))
        
        // Evaluation
        .def("evaluate", [](Parser& p, NodePtr expr, ModelPtr model) {
            return p.evaluate(expr, model);
        }, py::arg("expr"), py::arg("model"), "Evaluate expression under model")
        
        // Model access
        .def("get_model", &Parser::getModel, "Get current model (after check-sat)")
        
        // Expression building - Variables
        .def("var", py::overload_cast<const std::string&, const std::string&>(
            &Parser::declareVar), py::arg("name"), py::arg("sort"), 
            "Declare a variable with sort string (e.g., 'Int', 'Bool')")
        .def("var_int", &Parser::mkVarInt, py::arg("name"), "Declare Int variable")
        .def("var_real", &Parser::mkVarReal, py::arg("name"), "Declare Real variable")
        .def("var_bool", &Parser::mkVarBool, py::arg("name"), "Declare Bool variable")
        .def("var_bv", &Parser::mkVarBv, py::arg("name"), py::arg("width"), 
            "Declare BitVec variable")
        
        // Expression building - Constants
        .def("const_int", py::overload_cast<const int&>(&Parser::mkConstInt), 
            py::arg("value"), "Create Int constant from int")
        .def("const_int", py::overload_cast<const std::string&>(&Parser::mkConstInt),
            py::arg("value"), "Create Int constant from string")
        .def("const_real", py::overload_cast<const double&>(&Parser::mkConstReal),
            py::arg("value"), "Create Real constant from float")
        .def("const_real", py::overload_cast<const std::string&>(&Parser::mkConstReal),
            py::arg("value"), "Create Real constant from string")
        .def("true_", &Parser::mkTrue, "Create true constant")
        .def("false_", &Parser::mkFalse, "Create false constant")
        
        // Expression building - Boolean
        .def("and_", py::overload_cast<NodePtr, NodePtr>(&Parser::mkAnd), 
            py::arg("a"), py::arg("b"), "Create AND expression")
        .def("and_", py::overload_cast<const std::vector<NodePtr>&>(&Parser::mkAnd),
            py::arg("args"), "Create AND expression from list")
        .def("or_", py::overload_cast<NodePtr, NodePtr>(&Parser::mkOr),
            py::arg("a"), py::arg("b"), "Create OR expression")
        .def("or_", py::overload_cast<const std::vector<NodePtr>&>(&Parser::mkOr),
            py::arg("args"), "Create OR expression from list")
        .def("not_", &Parser::mkNot, py::arg("a"), "Create NOT expression")
        .def("implies", py::overload_cast<NodePtr, NodePtr>(&Parser::mkImplies), 
            py::arg("a"), py::arg("b"), "Create IMPLIES expression")
        .def("xor_", py::overload_cast<NodePtr, NodePtr>(&Parser::mkXor), 
            py::arg("a"), py::arg("b"), "Create XOR expression")
        .def("ite", py::overload_cast<NodePtr, NodePtr, NodePtr>(&Parser::mkIte), 
            py::arg("cond"), py::arg("then_"), py::arg("else_"),
            "Create ITE (if-then-else) expression")
        
        // Expression building - Comparison
        .def("eq", py::overload_cast<NodePtr, NodePtr>(&Parser::mkEq), 
            py::arg("a"), py::arg("b"), "Create equality")
        .def("distinct", py::overload_cast<NodePtr, NodePtr>(&Parser::mkDistinct), 
            py::arg("a"), py::arg("b"), "Create distinctness")
        
        // Expression building - Arithmetic  
        .def("add", py::overload_cast<NodePtr, NodePtr>(&Parser::mkAdd), 
            py::arg("a"), py::arg("b"), "Create addition")
        .def("sub", py::overload_cast<NodePtr, NodePtr>(&Parser::mkSub), 
            py::arg("a"), py::arg("b"), "Create subtraction")
        .def("mul", py::overload_cast<NodePtr, NodePtr>(&Parser::mkMul), 
            py::arg("a"), py::arg("b"), "Create multiplication")
        .def("neg", &Parser::mkNeg, py::arg("a"), "Create negation")
        .def("div", py::overload_cast<NodePtr, NodePtr>(&Parser::mkDiv), 
            py::arg("a"), py::arg("b"), "Create division")
        .def("mod", &Parser::mkMod, py::arg("a"), py::arg("b"), "Create modulo")
        
        // Expression building - Comparison (arithmetic)
        .def("lt", py::overload_cast<NodePtr, NodePtr>(&Parser::mkLt), 
            py::arg("a"), py::arg("b"), "Create less-than")
        .def("le", py::overload_cast<NodePtr, NodePtr>(&Parser::mkLe),
            py::arg("a"), py::arg("b"), "Create less-or-equal")
        .def("gt", py::overload_cast<NodePtr, NodePtr>(&Parser::mkGt),
            py::arg("a"), py::arg("b"), "Create greater-than")
        .def("ge", py::overload_cast<NodePtr, NodePtr>(&Parser::mkGe),
            py::arg("a"), py::arg("b"), "Create greater-or-equal")
        
        // Generic expression from string
        .def("expr", &Parser::mkExpr, py::arg("smt2_expr"), 
            "Create expression from SMT-LIB2 string")
        
        // Assertion (note: C++ method is 'assert', Python uses 'assert_' since assert is a keyword)
        .def("assert_", py::overload_cast<const std::string&>(&Parser::assert), 
            py::arg("constraint"), "Assert constraint string")
        .def("assert_", py::overload_cast<NodePtr>(&Parser::assert),
            py::arg("node"), "Assert constraint node")
        
        // Utility
        .def("to_string", [](Parser& p, NodePtr expr) {
            return p.toString(expr);
        }, py::arg("expr"), "Convert expression to string")
        .def("dump_smt2", py::overload_cast<>(&Parser::dumpSMT2), 
            "Dump all content as SMT-LIB2 string")
        .def("dump_smt2", py::overload_cast<const std::string&>(&Parser::dumpSMT2),
            py::arg("filename"), "Dump all content to SMT-LIB2 file");
    
    // ===== Module-level convenience functions =====
    m.def("parse", [](const std::string& text) {
        auto p = std::make_shared<Parser>();
        p->parseStr(text);
        return p;
    }, py::arg("text"), "Parse SMT-LIB2 text and return a Parser");
    
    m.def("parse_file", [](const std::string& path) {
        auto p = std::make_shared<Parser>();
        p->parse(path);
        return p;
    }, py::arg("path"), "Parse SMT-LIB2 file and return a Parser");
    
    // Version info
    m.attr("__version__") = "0.1.0";
}
