/* -*- Source -*-
 *
 * The Sort Class
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

#include "somtparser/ir/sort.h"

namespace SOMTParser{

    // Static constant sorts are now defined inline in the header file

    // SortManager implementation
    SortManager::SortManager() {
        initializeStaticSorts();
    }

    SortManager::~SortManager() {
        clear();
    }

    void SortManager::initializeStaticSorts() {
        // Add static sorts to the manager
        sorts.push_back(NULL_SORT);
        sorts.push_back(UNKNOWN_SORT);
        sorts.push_back(BOOL_SORT);
        sorts.push_back(INT_SORT);
        sorts.push_back(REAL_SORT);
        sorts.push_back(ALGEBRAIC_SORT);
        sorts.push_back(TRANSCENDENTAL_SORT);
        sorts.push_back(STR_SORT);
        sorts.push_back(REG_SORT);
        sorts.push_back(EXT_SORT);
        sorts.push_back(NAT_SORT);
        sorts.push_back(RAND_SORT);
        sorts.push_back(INTOREAL_SORT);
        sorts.push_back(FLOAT64_SORT);
        sorts.push_back(FLOAT32_SORT);
        sorts.push_back(FLOAT16_SORT);
        sorts.push_back(ROUNDING_MODE_SORT);
        
        // Add them to the hash buckets for deduplication
        for (size_t i = 0; i < sorts.size(); ++i) {
            size_t hash_val = sorts[i]->hash();
            sort_buckets[hash_val].push_back({sorts[i], i});
        }
    }

    std::shared_ptr<Sort> SortManager::getSort(const size_t index) const {
        if (index >= sorts.size()) {
            return nullptr;
        }
        return sorts[index];
    }

    size_t SortManager::getIndex(const std::shared_ptr<Sort>& sort) const {
        for (size_t i = 0; i < sorts.size(); ++i) {
            if (sorts[i] == sort) {
                return i;
            }
        }
        return SIZE_MAX; // Not found
    }

    size_t SortManager::size() const {
        return sorts.size();
    }

    void SortManager::clear() {
        sorts.clear();
        sort_buckets.clear();
    }

    std::shared_ptr<Sort> SortManager::insertSortToBucket(const std::shared_ptr<Sort>& sort) {
        size_t hash_val = sort->hash();
        
        // Check if an equivalent sort already exists
        auto bucket_it = sort_buckets.find(hash_val);
        if (bucket_it != sort_buckets.end()) {
            for (const auto& pair : bucket_it->second) {
                if (*pair.first == *sort) {
                    return pair.first; // Return existing sort
                }
            }
        }
        
        // Add new sort
        size_t new_index = sorts.size();
        sorts.push_back(sort);
        sort_buckets[hash_val].push_back({sort, new_index});
        
        return sort;
    }

    // createSort methods
    std::shared_ptr<Sort> SortManager::createSort(SORT_KIND kind, std::string name, size_t arity, std::vector<std::shared_ptr<Sort>> children) {
        auto sort = std::make_shared<Sort>(kind, name, arity, children);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSort(SORT_KIND kind, std::string name, size_t arity) {
        auto sort = std::make_shared<Sort>(kind, name, arity);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSort(SORT_KIND kind, std::string name) {
        auto sort = std::make_shared<Sort>(kind, name);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSort(SORT_KIND kind) {
        auto sort = std::make_shared<Sort>(kind);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSort(std::string name) {
        auto sort = std::make_shared<Sort>(name);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSort() {
        auto sort = std::make_shared<Sort>();
        return insertSortToBucket(sort);
    }

    // Specific sort creation methods
    std::shared_ptr<Sort> SortManager::createBVSort(size_t width) {
        auto width_sort = std::make_shared<Sort>(SORT_KIND::SK_NULL, "Width", width);
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_BV, "BV", 0, std::vector<std::shared_ptr<Sort>>{width_sort});
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createFPSort(size_t exp, size_t sig) {
        auto exp_sort = std::make_shared<Sort>(SORT_KIND::SK_NULL, "Exp", exp);
        auto sig_sort = std::make_shared<Sort>(SORT_KIND::SK_NULL, "Sig", sig);
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_FP, "FP", 0, std::vector<std::shared_ptr<Sort>>{exp_sort, sig_sort});
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createArraySort(std::shared_ptr<Sort> index, std::shared_ptr<Sort> elem) {
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_ARRAY, "Array", 0, std::vector<std::shared_ptr<Sort>>{index, elem});
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createDatatypeSort(const std::string& name,
            const std::vector<Sort::DtConstructor>& constructors) {
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_DATATYPE, name, 0);
        sort->dt_constructors = std::make_shared<std::vector<Sort::DtConstructor>>(constructors);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSortDec(const std::string& name, size_t arity) {
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_DEC, name, arity);
        return insertSortToBucket(sort);
    }

    std::shared_ptr<Sort> SortManager::createSortDef(const std::string& name, const std::vector<std::shared_ptr<Sort>> &params, std::shared_ptr<Sort> out_sort) {
        std::vector<std::shared_ptr<Sort>> children;
        children.insert(children.end(), params.begin(), params.end());
        children.push_back(out_sort);
        auto sort = std::make_shared<Sort>(SORT_KIND::SK_DEF, name, children.size() - 1, children);
        return insertSortToBucket(sort);
    }


    size_t SortManager::getBitWidth(const std::shared_ptr<Sort> &sort) {
        condAssert(sort != nullptr, "Sort is null");
        return sort->getBitWidth();
    }
    
    size_t SortManager::getExponentWidth(const std::shared_ptr<Sort> &sort) {
        condAssert(sort != nullptr, "Sort is null");
        return sort->getExponentWidth();
    }
    
    size_t SortManager::getSignificandWidth(const std::shared_ptr<Sort> &sort) {
        condAssert(sort != nullptr, "Sort is null");
        return sort->getSignificandWidth();
    }
    
    std::shared_ptr<Sort> SortManager::getIndexSort(const std::shared_ptr<Sort> &sort) {
        return sort->getIndexSort();
    }
    
    std::shared_ptr<Sort> SortManager::getElemSort(const std::shared_ptr<Sort> &sort) {
        return sort->getElemSort();
    }
    
    std::shared_ptr<Sort> SortManager::getParamSort(const std::shared_ptr<Sort> &sort, size_t index) {
        return sort->getParamSort(index);
    }
    
    std::vector<std::shared_ptr<Sort>> SortManager::getParams(const std::shared_ptr<Sort> &sort) {
        return sort->getParams();
    }
    
    std::shared_ptr<Sort> SortManager::getOutSort(const std::shared_ptr<Sort> &sort) {
        return sort->getOutSort();
    }
}