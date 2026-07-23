include(FindPackageHandleStandardArgs)

set(_MPFR_HINTS)
if(APPLE)
    find_program(_MPFR_BREW_EXECUTABLE brew)
    if(_MPFR_BREW_EXECUTABLE)
        execute_process(
            COMMAND "${_MPFR_BREW_EXECUTABLE}" --prefix mpfr
            OUTPUT_VARIABLE _MPFR_BREW_PREFIX
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        list(APPEND _MPFR_HINTS "${_MPFR_BREW_PREFIX}")
    endif()
endif()

find_path(MPFR_INCLUDE_DIR NAMES mpfr.h
    HINTS ${_MPFR_HINTS}
    PATH_SUFFIXES include)
find_library(MPFR_LIBRARY NAMES mpfr libmpfr
    HINTS ${_MPFR_HINTS}
    PATH_SUFFIXES lib)

find_package_handle_standard_args(MPFR
    REQUIRED_VARS MPFR_INCLUDE_DIR MPFR_LIBRARY)

if(MPFR_FOUND AND NOT TARGET MPFR::MPFR)
    add_library(MPFR::MPFR UNKNOWN IMPORTED)
    set_target_properties(MPFR::MPFR PROPERTIES
        IMPORTED_LOCATION "${MPFR_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MPFR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES GMP::GMP)
endif()

mark_as_advanced(MPFR_INCLUDE_DIR MPFR_LIBRARY)
