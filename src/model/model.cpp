/* -*- Source -*-
 *
 * An SMT/OMT Parser (Model part)
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

#include "somtparser/model/model.h"

namespace SOMTParser{
    Model::Model(){

    }

    Model::Model(const Model &other){
        model_vars = other.model_vars;
        model_values = other.model_values;
        model_name_index = other.model_name_index;
        uf_tables_ = other.uf_tables_;
        array_values_ = other.array_values_;
    }

    Model::~Model(){

    }

    void Model::add(const std::shared_ptr<DAGNode> &node, const std::shared_ptr<DAGNode> &value){
        if(model_name_index.find(node->getName()) != model_name_index.end()){
            model_values[model_name_index[node->getName()]] = value;
        }
        else{
            model_name_index[node->getName()] = model_vars.size();
            model_vars.emplace_back(node);
            model_values.emplace_back(value);
        }
    }

    void Model::add(const std::string &name, const std::shared_ptr<DAGNode> &value){
        // Upsert: create the entry when the name is unknown. The previous
        // precondition (name already registered) turned into an out-of-bounds
        // write in release builds where condAssert is a no-op.
        auto it = model_name_index.find(name);
        if(it == model_name_index.end()){
            model_name_index[name] = model_vars.size();
            model_vars.emplace_back(std::make_shared<DAGNode>(
                value ? value->getSort() : SortManager::UNKNOWN_SORT,
                NODE_KIND::NT_VAR, name));
            model_values.emplace_back(value);
            return;
        }
        model_values[it->second] = value;
    }

    std::shared_ptr<DAGNode> Model::get(const std::shared_ptr<DAGNode> &node){
        if(model_name_index.find(node->getName()) != model_name_index.end()){
            return model_values[model_name_index[node->getName()]];
        }
        return NodeManager::UNKNOWN_NODE;
    }

    std::shared_ptr<DAGNode> Model::get(const std::string &name){
        if(model_name_index.find(name) != model_name_index.end()){
            return model_values[model_name_index[name]];
        }
        return NodeManager::UNKNOWN_NODE;
    }

    void Model::addVar(const std::shared_ptr<DAGNode> &node){
        if(model_name_index.find(node->getName()) != model_name_index.end()){
            return;
        }
        model_name_index[node->getName()] = model_vars.size();
        model_vars.emplace_back(node);
        model_values.emplace_back(NodeManager::UNKNOWN_NODE);
    }

    bool Model::isFull() const{
        for(size_t i = 0; i < model_vars.size(); i++){
            if(model_values[i]->isUnknown()){
                return false;
            }
        }
        return true;
    }

    bool Model::isEmpty() const{
        if(model_vars.size() == 0){
            return true;
        }
        for(size_t i = 0; i < model_vars.size(); i++){
            if(!model_values[i]->isUnknown()){
                return false;
            }
        }
        return true;
    }

    void Model::clear(){
        model_vars.clear();
        model_values.clear();
        model_name_index.clear();
    }

    void Model::clearValues(){
        for(size_t i = 0; i < model_vars.size(); i++){
            model_values[i] = NodeManager::UNKNOWN_NODE;
        }
    }

    size_t Model::size() const{
        return model_vars.size();
    }

    std::vector<std::shared_ptr<DAGNode>> Model::getVars() const{
        return model_vars;
    }

    std::vector<std::shared_ptr<DAGNode>> Model::getValues() const{
        return model_values;
    }

    std::vector<std::pair<std::string, std::shared_ptr<DAGNode>>> Model::getPairs() const{
        std::vector<std::pair<std::string, std::shared_ptr<DAGNode>>> pairs;
        for(size_t i = 0; i < model_vars.size(); i++){
            pairs.emplace_back(std::make_pair(model_vars[i]->getName(), model_values[i]));
        }
        return pairs;
    }

    std::string Model::toString(){
        std::stringstream ss;
        for(size_t i = 0; i < model_vars.size(); i++){
            ss << "(define-fun " << model_vars[i]->getName() << " () " << model_vars[i]->getSort()->toString() << " " << dumpSMTLIB2(model_values[i]) << ")" << std::endl;
        }
        return ss.str();
    }

    // ── UF tables ────────────────────────────────────────────────────────────

    void Model::setUF(const std::string& func, const std::string& arg_key,
                      const std::shared_ptr<DAGNode>& result) {
        uf_tables_[func][arg_key] = result;
    }

    std::shared_ptr<DAGNode> Model::getUF(const std::string& func,
                                          const std::string& arg_key) const {
        auto it = uf_tables_.find(func);
        if(it == uf_tables_.end()) return NodeManager::UNKNOWN_NODE;
        auto jt = it->second.find(arg_key);
        if(jt == it->second.end()) return NodeManager::UNKNOWN_NODE;
        return jt->second;
    }

    bool Model::hasUF(const std::string& func) const {
        return uf_tables_.count(func) > 0;
    }

    // ── Array storage ────────────────────────────────────────────────────────

    void Model::setArrayStore(const std::string& arr, const std::string& idx_key,
                              const std::shared_ptr<DAGNode>& val) {
        array_values_[arr].stores[idx_key] = val;
    }

    void Model::setArrayDefault(const std::string& arr,
                                const std::shared_ptr<DAGNode>& default_val) {
        array_values_[arr].default_value = default_val;
    }

    std::shared_ptr<DAGNode> Model::getArraySelect(const std::string& arr,
                                                   const std::string& idx_key) const {
        auto it = array_values_.find(arr);
        if(it == array_values_.end()) return NodeManager::UNKNOWN_NODE;
        auto jt = it->second.stores.find(idx_key);
        if(jt != it->second.stores.end()) return jt->second;
        if(it->second.default_value) return it->second.default_value;
        return NodeManager::UNKNOWN_NODE;
    }

    ModelPtr newModel(){
        return std::make_shared<Model>();
    }
}
