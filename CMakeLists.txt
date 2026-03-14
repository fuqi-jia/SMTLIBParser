cmake_minimum_required(VERSION 3.15)
project(SOMTParser VERSION 1.0.0 LANGUAGES CXX)

# set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific flags
if(MSVC)
    # MSVC flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    # Disable specific warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4267 /wd4996 /wd4100 /wd4101 /wd4189")
else()
    # GCC/Clang flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -Wextra -Werror")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable -Wno-unused-value")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
    
    # Clang-specific: suppress unused lambda capture warning
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-lambda-capture")
    endif()
endif()

# set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# add options
option(BUILD_SHARED_LIBS "Build shared libraries (.so/.dll)" OFF)
option(BUILD_BOTH_LIBS "Build both static and shared libraries" ON)
option(BUILD_TESTS "Build the tests" OFF)
option(ENABLE_DEBUG_SYMBOLS "Enable debug symbols for GDB support" OFF)
option(ENABLE_TIMING "Enable timing profiler" OFF)

# enable time functions
if(ENABLE_TIMING)
    add_compile_definitions(ENABLE_TIMING)
endif()

# Find GMP and MPFR libraries
# First try pkg-config, then fall back to manual search
find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(GMP QUIET gmp)
    pkg_check_modules(MPFR QUIET mpfr)
endif()

# if pkg-config doesn't find GMP, try manually finding it
if(NOT GMP_FOUND)
    find_library(GMP_LIBRARIES NAMES gmp libgmp)
    find_path(GMP_INCLUDE_DIRS NAMES gmp.h)
    if(GMP_LIBRARIES AND GMP_INCLUDE_DIRS)
        set(GMP_FOUND TRUE)
        message(STATUS "Found GMP via manual search: ${GMP_LIBRARIES}")
    else()
        message(FATAL_ERROR "GMP library not found. Please install GMP development package (e.g., libgmp-dev).")
    endif()
endif()

# if pkg-config doesn't find MPFR, try manually finding it
if(NOT MPFR_FOUND)
    find_library(MPFR_LIBRARIES NAMES mpfr libmpfr)
    find_path(MPFR_INCLUDE_DIRS NAMES mpfr.h)
    if(MPFR_LIBRARIES AND MPFR_INCLUDE_DIRS)
        set(MPFR_FOUND TRUE)
        message(STATUS "Found MPFR via manual search: ${MPFR_LIBRARIES}")
    else()
        message(FATAL_ERROR "MPFR library not found. Please install MPFR development package (e.g., libmpfr-dev).")
    endif()
endif()

# collect all source files under src/ (mirrors include/somtparser/)
file(GLOB_RECURSE SOURCES "src/*.cpp")

# collect all header files under include/somtparser/
file(GLOB_RECURSE HEADERS "include/somtparser/*.h")

# create library
if(BUILD_BOTH_LIBS)
    # create static library
    add_library(somtparser_static STATIC ${SOURCES} ${HEADERS})
    set_target_properties(somtparser_static PROPERTIES 
        OUTPUT_NAME somtparser
        POSITION_INDEPENDENT_CODE ON
    )
    
    if(ENABLE_DEBUG_SYMBOLS)
        target_compile_options(somtparser_static PRIVATE -g -O0)
    endif()
    
    # set static library include directories (include root so that #include <somtparser/...> works)
    target_include_directories(somtparser_static PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
    )
    
    # link static library dependencies
    target_link_libraries(somtparser_static PUBLIC
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
    )
    
    # create shared library
    add_library(somtparser_shared SHARED ${SOURCES} ${HEADERS})
    set_target_properties(somtparser_shared PROPERTIES 
        OUTPUT_NAME somtparser
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
    
    if(ENABLE_DEBUG_SYMBOLS)
        target_compile_options(somtparser_shared PRIVATE -g -O0)
    endif()
    
    # set shared library include directories
    target_include_directories(somtparser_shared PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
    )
    
    # link shared library dependencies
    target_link_libraries(somtparser_shared PUBLIC
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
    )
    
    # create alias target for compatibility
    add_library(somtparser ALIAS somtparser_static)
    
    # install configuration
    install(TARGETS somtparser_static somtparser_shared
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
else()
    # create single library (static or shared, depending on BUILD_SHARED_LIBS)
    add_library(somtparser ${SOURCES} ${HEADERS})
    
    if(ENABLE_DEBUG_SYMBOLS)
        target_compile_options(somtparser PRIVATE -g -O0)
    endif()
    
    # set library include directories
    target_include_directories(somtparser PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
    )
    
    # link library dependencies
    target_link_libraries(somtparser PUBLIC
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
    )
    
    # install configuration
    install(TARGETS somtparser
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

# install header files only (no .cpp)
install(DIRECTORY include/somtparser DESTINATION include FILES_MATCHING PATTERN "*.h")

# if needed, add test subdirectory
if(BUILD_TESTS)
    add_subdirectory(test)
endif()

# ===== Python Bindings (Optional) =====
option(BUILD_PYTHON "Build Python bindings" OFF)

if(BUILD_PYTHON)
    find_package(Python 3.9 COMPONENTS Interpreter Development.Module REQUIRED)
    find_package(pybind11 CONFIG REQUIRED)
    
    pybind11_add_module(_somtparser python/bindings.cpp)
    
    # Link against the static library
    target_link_libraries(_somtparser PRIVATE 
        somtparser_static
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
    )
    
    target_include_directories(_somtparser PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
    )
    
    # Install the Python module (destination relative to wheel.install-dir in pyproject.toml)
    install(TARGETS _somtparser LIBRARY DESTINATION .)
endif()