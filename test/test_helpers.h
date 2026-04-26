#ifndef SOMTPARSER_TEST_HELPERS_H
#define SOMTPARSER_TEST_HELPERS_H

#include <cstdlib>
#include <iostream>
#include <string>

// NDEBUG-safe assertion for side-effectful expressions.
// Unlike <cassert>'s assert(), this always evaluates the expression even under NDEBUG.
#define VERIFY(expr) do { \
    if(!(expr)) { \
        std::cerr << "VERIFY failed: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        std::abort(); \
    } \
} while(0)

#endif
