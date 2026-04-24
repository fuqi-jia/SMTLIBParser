/* -*- Header -*-
 *
 * The Model Enumeration
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

#ifndef _MODEL_H
#define _MODEL_H

#include "somtparser/ir/dag.h"

namespace SOMTParser{
    class Model{
        public:
            Model();
            Model(const Model &other);
            ~Model();

            /**
             * @brief Add a variable to the model
             * 
             * @param node Variable to add
             */
            void add(const std::shared_ptr<DAGNode> &node, const std::shared_ptr<DAGNode> &value);

            /**
             * @brief Add a variable to the model
             * 
             * @param name Variable name
             */
            void add(const std::string &name, const std::shared_ptr<DAGNode> &value);

            /**
             * @brief Add a variable to the model
             * 
             * @param node Variable to add
             */
            void addVar(const std::shared_ptr<DAGNode> &node);

            /**
             * @brief Get a variable from the model
             * 
             * @param node Variable to get
             * @return Variable
             */
            std::shared_ptr<DAGNode> get(const std::shared_ptr<DAGNode> &node);

            /**
             * @brief Get a variable from the model
             * 
             * @param name Variable name
             * @return Variable
             */
            std::shared_ptr<DAGNode> get(const std::string &name);

            /**
             * @brief Check if the model is full
             * 
             * @return True if the model is full, false otherwise
             */
            bool isFull() const;

            /**
             * @brief Check if the model is empty
             * 
             * @return True if the model is empty, false otherwise
             */
            bool isEmpty() const;

            /**
             * @brief Clear the model
             */
            void clear();

            /**
             * @brief Clear the values of the model
             */
            void clearValues();

            /**
             * @brief Get the size of the model
             * 
             * @return The size of the model
             */
            size_t size() const;

            /**
             * @brief Get the variables of the model
             * 
             * @return The variables of the model
             */
            std::vector<std::shared_ptr<DAGNode>> getVars() const;

            /**
             * @brief Get the values of the model
             * 
             * @return The values of the model
             */
            std::vector<std::shared_ptr<DAGNode>> getValues() const;

            /**
             * @brief Get the pairs of the model
             * 
             * @return The pairs of the model
             */
            std::vector<std::pair<std::string, std::shared_ptr<DAGNode>>> getPairs() const;

            /**
             * @brief Get the string representation of the model
             * 
             * @return The string representation of the model
             */
            std::string toString();

            // ── UF function tables ────────────────────────────────────────────

            /// Store a UF result: func_name( arg_key ) → result_node
            void setUF(const std::string& func, const std::string& arg_key,
                       const std::shared_ptr<DAGNode>& result);

            /// Look up a UF result by function name and argument key.
            /// Returns NodeManager::UNKNOWN_NODE if not found.
            std::shared_ptr<DAGNode> getUF(const std::string& func,
                                           const std::string& arg_key) const;

            /// Returns true if any UF entries exist for the given function name.
            bool hasUF(const std::string& func) const;

            // ── Array storage ─────────────────────────────────────────────────

            /// Store an array element: array_name[ idx_key ] → val_node
            void setArrayStore(const std::string& arr, const std::string& idx_key,
                               const std::shared_ptr<DAGNode>& val);

            /// Set the "else" (default) value for an array (from const-array).
            void setArrayDefault(const std::string& arr,
                                 const std::shared_ptr<DAGNode>& default_val);

            /// Look up an array element. Returns the default value when the
            /// index key is not in the store; returns UNKNOWN_NODE if the array
            /// is not known at all.
            std::shared_ptr<DAGNode> getArraySelect(const std::string& arr,
                                                    const std::string& idx_key) const;

        private:
            std::unordered_map<std::string, size_t> model_name_index;
            std::vector<std::shared_ptr<DAGNode>> model_vars;
            std::vector<std::shared_ptr<DAGNode>> model_values;

            // UF: func_name → { arg_key → result_node }
            std::unordered_map<std::string,
                std::unordered_map<std::string, std::shared_ptr<DAGNode>>> uf_tables_;

            // Arrays: array_name → { index_key → value_node, default }
            struct ArrayValue {
                std::unordered_map<std::string, std::shared_ptr<DAGNode>> stores;
                std::shared_ptr<DAGNode> default_value;
            };
            std::unordered_map<std::string, ArrayValue> array_values_;
    };

    // smart pointer
    typedef std::shared_ptr<Model> ModelPtr;
    ModelPtr newModel();
}

#endif

