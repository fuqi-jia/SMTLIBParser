cmake_minimum_required(VERSION 3.16)
project(SOMTParser VERSION 1.0.0 LANGUAGES CXX)

include(CTest)
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(SOMTPARSER_BUILD_TESTS "Build SOMTParser tests" OFF)
option(SOMTPARSER_WARNINGS_AS_ERRORS "Treat SOMTParser warnings as errors" OFF)
option(SOMTPARSER_ENABLE_TIMING "Enable the timing profiler" OFF)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

find_package(GMP REQUIRED)
find_package(MPFR REQUIRED)

file(GLOB_RECURSE SOMTPARSER_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE SOMTPARSER_HEADERS CONFIGURE_DEPENDS "include/somtparser/*.h")

add_library(somtparser ${SOMTPARSER_SOURCES} ${SOMTPARSER_HEADERS})
add_library(SOMTParser::somtparser ALIAS somtparser)
set_target_properties(somtparser PROPERTIES
    EXPORT_NAME somtparser
    POSITION_INDEPENDENT_CODE ON)
target_compile_features(somtparser PUBLIC cxx_std_17)
target_include_directories(somtparser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_link_libraries(somtparser PUBLIC GMP::GMPXX MPFR::MPFR)

if(SOMTPARSER_ENABLE_TIMING)
    target_compile_definitions(somtparser PUBLIC ENABLE_TIMING)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(somtparser PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-unused-variable
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-unused-value
        -Wno-unused-local-typedefs)
    if(SOMTPARSER_WARNINGS_AS_ERRORS)
        target_compile_options(somtparser PRIVATE -Werror)
    endif()
endif()

if(SOMTPARSER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

install(TARGETS somtparser
    EXPORT SOMTParserTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/somtparser
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h")
install(EXPORT SOMTParserTargets
    FILE SOMTParserTargets.cmake
    NAMESPACE SOMTParser::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SOMTParser)

configure_package_config_file(
    cmake/SOMTParserConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/SOMTParserConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SOMTParser)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/SOMTParserConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/SOMTParserConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/SOMTParserConfigVersion.cmake
    cmake/FindGMP.cmake
    cmake/FindMPFR.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SOMTParser)
