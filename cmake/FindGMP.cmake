include(FindPackageHandleStandardArgs)

set(_GMP_HINTS)
if(APPLE)
    find_program(_GMP_BREW_EXECUTABLE brew)
    if(_GMP_BREW_EXECUTABLE)
        execute_process(
            COMMAND "${_GMP_BREW_EXECUTABLE}" --prefix gmp
            OUTPUT_VARIABLE _GMP_BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        list(APPEND _GMP_HINTS "${_GMP_BREW_PREFIX}")
    endif()
endif()

find_path(GMP_INCLUDE_DIR NAMES gmp.h gmpxx.h
    HINTS ${_GMP_HINTS}
    PATH_SUFFIXES include)
find_library(GMP_LIBRARY NAMES gmp libgmp
    HINTS ${_GMP_HINTS}
    PATH_SUFFIXES lib)
find_library(GMPXX_LIBRARY NAMES gmpxx libgmpxx
    HINTS ${_GMP_HINTS}
    PATH_SUFFIXES lib)

find_package_handle_standard_args(GMP
    REQUIRED_VARS GMP_INCLUDE_DIR GMP_LIBRARY GMPXX_LIBRARY)

if(GMP_FOUND AND NOT TARGET GMP::GMP)
    add_library(GMP::GMP UNKNOWN IMPORTED)
    set_target_properties(GMP::GMP PROPERTIES
        IMPORTED_LOCATION "${GMP_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")
endif()

if(GMP_FOUND AND NOT TARGET GMP::GMPXX)
    add_library(GMP::GMPXX UNKNOWN IMPORTED)
    set_target_properties(GMP::GMPXX PROPERTIES
        IMPORTED_LOCATION "${GMPXX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES GMP::GMP)
endif()

mark_as_advanced(GMP_INCLUDE_DIR GMP_LIBRARY GMPXX_LIBRARY)
