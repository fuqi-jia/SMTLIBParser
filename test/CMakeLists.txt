file(GLOB_RECURSE SOMTPARSER_TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

set(SOMTPARSER_TEST_TARGETS)
foreach(test_file IN LISTS SOMTPARSER_TEST_SOURCES)
    get_filename_component(test_name "${test_file}" NAME_WE)
    list(APPEND SOMTPARSER_TEST_TARGETS "${test_name}")

    add_executable("${test_name}" "${test_file}")
    target_compile_features("${test_name}" PRIVATE cxx_std_17)
    target_include_directories("${test_name}" PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries("${test_name}" PRIVATE SOMTParser::somtparser)

    add_test(NAME "SOMTParser.${test_name}" COMMAND "${test_name}")
    set_tests_properties("SOMTParser.${test_name}" PROPERTIES
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endforeach()

if(SOMTPARSER_TEST_TARGETS)
    add_custom_target(build_all_tests DEPENDS ${SOMTPARSER_TEST_TARGETS})
endif()
