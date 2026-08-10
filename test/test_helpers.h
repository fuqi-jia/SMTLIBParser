#ifndef SOMTPARSER_TEST_HELPERS_H
#define SOMTPARSER_TEST_HELPERS_H

#include <cstdlib>
#include <iostream>
#include <string>

// The test targets are compiled with -UNDEBUG (see test/CMakeLists.txt) so that
// assert() keeps working under the default Release build type, which defines
// NDEBUG. If that flag is ever lost, every assert() in the suite silently stops
// checking -- and any whose expression does the work being tested stops running
// at all. Fail the build instead of losing coverage quietly.
#ifdef NDEBUG
#error "Tests must be built with assertions enabled; test/CMakeLists.txt is expected to pass -UNDEBUG."
#endif

// NDEBUG-safe assertion for side-effectful expressions.
// Unlike <cassert>'s assert(), this always evaluates the expression even under NDEBUG.
#define VERIFY(expr) do { \
    if(!(expr)) { \
        std::cerr << "VERIFY failed: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        std::abort(); \
    } \
} while(0)

#endif
