cmake_minimum_required(VERSION 3.10)
project(SOMTParser VERSION 1.0.0 LANGUAGES CXX)

# set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable -Wno-unused-value")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")

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

# use pkg-config to find GMP, MPFR, and libcurl
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GMP QUIET gmp)
    pkg_check_modules(MPFR QUIET mpfr)
    pkg_check_modules(CURL QUIET libcurl)
endif()

# find curl manually if pkg-config fails
if(NOT CURL_FOUND)
    find_library(CURL_LIBRARIES NAMES curl libcurl)
    find_path(CURL_INCLUDE_DIRS NAMES curl/curl.h)
    if(CURL_LIBRARIES AND CURL_INCLUDE_DIRS)
        set(CURL_FOUND TRUE)
    else()
        message(WARNING "libcurl not found. NL4SMT LLM backend will not be available. Install libcurl-dev.")
    endif()
endif()

# if pkg-config doesn't find the libraries, try manually finding them
if(NOT GMP_FOUND)
    # try searching for library files directly
    find_library(GMP_LIBRARIES NAMES gmp libgmp)
    find_path(GMP_INCLUDE_DIRS NAMES gmp.h)
    if(GMP_LIBRARIES AND GMP_INCLUDE_DIRS)
        set(GMP_FOUND TRUE)
    else()
        message(FATAL_ERROR "GMP library not found. Please install GMP development package.")
    endif()
endif()

if(NOT MPFR_FOUND)
    # try searching for library files directly
    find_library(MPFR_LIBRARIES NAMES mpfr libmpfr)
    find_path(MPFR_INCLUDE_DIRS NAMES mpfr.h)
    if(MPFR_LIBRARIES AND MPFR_INCLUDE_DIRS)
        set(MPFR_FOUND TRUE)
    else()
        message(FATAL_ERROR "MPFR library not found. Please install MPFR development package.")
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/third_party
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
    )
    
    # link static library dependencies
    target_link_libraries(somtparser_static PUBLIC
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
        ${CURL_LIBRARIES}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/third_party
        ${GMP_INCLUDE_DIRS}
        ${MPFR_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
    )
    
    # link shared library dependencies
    target_link_libraries(somtparser_shared PUBLIC
        ${GMP_LIBRARIES}
        ${MPFR_LIBRARIES}
        ${CURL_LIBRARIES}
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
        ${CMAKE_CURRENT_SOURCE_DIR}/include/third_party
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