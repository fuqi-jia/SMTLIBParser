/* -*- Header -*-
 *
 * The Timing Header
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

 
#ifndef TIMING_HEADER
#define TIMING_HEADER

#include <iostream>
#include <chrono>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>


#ifdef ENABLE_TIMING
namespace timing_detail {
    struct Stat { uint64_t ns = 0; size_t cnt = 0; };

    inline std::mutex &mtx() {
        static std::mutex m; return m;
    }
    inline std::map<std::string, Stat> &data() {
        static std::map<std::string, Stat> d; return d;
    }
    inline void add(const std::string &key, uint64_t ns) {
        std::lock_guard<std::mutex> lk(mtx());
        auto &s = data()[key];
        s.ns  += ns;
        s.cnt += 1;
    }

    struct Reporter {
        Reporter() {
            // make sure data() is constructed before Reporter, avoid destructor order problem
            (void)timing_detail::data();
        }
        ~Reporter();
    };
}

inline timing_detail::Reporter::~Reporter() {
    std::lock_guard<std::mutex> lk(timing_detail::mtx());
    if (timing_detail::data().empty()) return;
    std::cerr << "\n========= Timing Result =========\n";
    for (const auto &p : timing_detail::data()) {
        double ms = p.second.ns / 1e6;
        std::cerr << p.first << ": " << ms << " ms in " << p.second.cnt << " calls\n";
    }
    std::cerr << "=================================\n";
}

inline timing_detail::Reporter __global_timing_reporter;

// thread-local timer stack, for calculating exclusive time
inline thread_local std::vector<class ScopedTimer*> __timer_stack;
// record current recursion depth of each function
inline thread_local std::unordered_map<std::string, size_t> __rec_depth;

class ScopedTimer {
    std::string name_;
    std::string report_name_;        // may have [R] tag
    std::chrono::high_resolution_clock::time_point start_;
    uint64_t child_ns_ = 0;          // sum of child timer time
    ScopedTimer* parent_ = nullptr;  // pointer to parent timer
    bool record_ = true;             // whether to write to global statistics
public:
    explicit ScopedTimer(const std::string &n, bool top_only = false) : name_(n) {
        size_t &depth_ref = __rec_depth[name_];
        size_t cur_depth = depth_ref;
        depth_ref++;

        bool is_top_call = (cur_depth == 0);
        record_ = !top_only || (top_only && is_top_call);

        // add tag to report name
        report_name_ = name_;
        if(top_only) report_name_ += "[R]"; // Recursive top-only
        start_ = std::chrono::high_resolution_clock::now();

        parent_ = __timer_stack.empty() ? nullptr : __timer_stack.back();
        __timer_stack.push_back(this);
    }
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        uint64_t total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();

        // add total time of this timer to child time of parent timer
        if (parent_) parent_->child_ns_ += total_ns;

        // calculate exclusive time (excluding all child timers)
        uint64_t exclusive_ns = total_ns > child_ns_ ? total_ns - child_ns_ : 0;
        if (exclusive_ns > 0 && record_)
            timing_detail::add(report_name_, exclusive_ns);

        // pop from stack
        __timer_stack.pop_back();

        // decrement recursion depth
        __rec_depth[name_]--;
    }
};

#define TIME_BLOCK(tag)            ::ScopedTimer __timer_##__LINE__(tag, false)
#define TIME_BLOCK_RECURSIVE(tag)  ::ScopedTimer __timer_##__LINE__(tag, true)
#define TIME_FUNC()                TIME_BLOCK(__FUNCTION__)
#define TIME_FUNC_RECURSIVE()      TIME_BLOCK_RECURSIVE(__FUNCTION__)
#else
#define TIME_BLOCK(tag) ((void)0)
#define TIME_FUNC()     ((void)0)
#endif

#endif // TIMING_HEADER