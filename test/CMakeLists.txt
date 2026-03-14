# Used by root CMakeLists.txt when BUILD_TESTS=ON (add_subdirectory(test)).
# Builds one executable per test/*.cpp and test/FP/*.cpp, linked with somtparser.
cmake_minimum_required(VERSION 3.10)
project(SOMTParser_Tests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GDB Debug
if(NOT DEFINED ENABLE_DEBUG_SYMBOLS)
    option(ENABLE_DEBUG_SYMBOLS "Enable debug symbols for GDB support" OFF)
endif()


# Use pkg-config to find GMP and MPFR (QUIET to allow fallback to manual search)
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(GMP QUIET gmp)
    pkg_check_modules(MPFR QUIET mpfr)
endif()

# If pkg-config fails to find libraries, try manual search
if(NOT GMP_FOUND)
    find_library(GMP_LIBRARIES NAMES gmp libgmp)
    find_path(GMP_INCLUDE_DIRS NAMES gmp.h)
    if(GMP_LIBRARIES AND GMP_INCLUDE_DIRS)
        set(GMP_FOUND TRUE)
    else()
        message(FATAL_ERROR "GMP library not found. Please install GMP development package.")
    endif()
endif()

if(NOT MPFR_FOUND)
    find_library(MPFR_LIBRARIES NAMES mpfr libmpfr)
    find_path(MPFR_INCLUDE_DIRS NAMES mpfr.h)
    if(MPFR_LIBRARIES AND MPFR_INCLUDE_DIRS)
        set(MPFR_FOUND TRUE)
    else()
        message(FATAL_ERROR "MPFR library not found. Please install MPFR development package.")
    endif()
endif()

# Collect all test source files
file(GLOB TEST_SOURCES "*.cpp")
file(GLOB FP_TEST_SOURCES "FP/*.cpp")
list(APPEND TEST_SOURCES ${FP_TEST_SOURCES})

# Create an executable for each test file
set(TEST_TARGETS)
foreach(test_file ${TEST_SOURCES})
    # Extract filename without extension
    get_filename_component(test_name ${test_file} NAME_WE)
    
    # Add to test target list
    list(APPEND TEST_TARGETS ${test_name})
    
    # Create executable
    add_executable(${test_name} ${test_file})
    
    # If enabled Debug Mode
    if(ENABLE_DEBUG_SYMBOLS)
        target_compile_options(${test_name} PRIVATE -g -O0)
    endif()
    
    # Include parent project's include directories
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
    )
    
    # Link against SOMTParser library
    target_link_libraries(${test_name} PRIVATE 
        somtparser
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
    )
endforeach()

# ===== CTest Support (cross-platform) =====
# Note: enable_testing() is called in root CMakeLists.txt before add_subdirectory(test)

# Register each test with CTest
if(TEST_TARGETS)
    foreach(test_name ${TEST_TARGETS})
        add_test(
            NAME ${test_name}
            COMMAND ${test_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endforeach()
    message(STATUS "Registered ${CMAKE_CURRENT_LIST_DIR} tests with CTest: ${TEST_TARGETS}")
else()
    message(STATUS "No test files found in test directory")
endif()

# ===== Legacy targets (for local development) =====
# Add a convenience target to build all tests
if(TEST_TARGETS)
    add_custom_target(build_all_tests DEPENDS ${TEST_TARGETS})
    
    # Add a target to run all tests
    add_custom_target(run_all_tests)
    foreach(test_name ${TEST_TARGETS})
        add_custom_command(TARGET run_all_tests POST_BUILD
            COMMAND ${test_name}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running test ${test_name}"
        )
    endforeach()
endif()

# Add a custom target to run all tests sequentially
add_custom_target(run_tests
    COMMENT "Running all tests sequentially..."
)

# Add dependencies for the run_tests target
if(TEST_TARGETS)
    add_dependencies(run_tests ${TEST_TARGETS})
    # Add custom commands to run each test in sequence
    foreach(test_name ${TEST_TARGETS})
        add_custom_command(TARGET run_tests POST_BUILD
            COMMAND ${test_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Running test ${test_name}"
        )
    endforeach()
endif() 