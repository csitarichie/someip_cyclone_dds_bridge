# Find Clang format
#
#
if(NOT CLANG_FORMAT_BIN_NAME)
    set(CLANG_FORMAT_BIN_NAME clang-format)
endif()

# if custom path check there first
if(CLANG_FORMAT_ROOT_DIR)
    find_program(CLANG_FORMAT_BIN
                 NAMES
                 ${CLANG_FORMAT_BIN_NAME}
                 PATHS
                 "${CLANG_FORMAT_ROOT_DIR}"
                 NO_DEFAULT_PATH)
endif()

find_program(CLANG_FORMAT_BIN NAMES ${CLANG_FORMAT_BIN_NAME})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    ClangFormat
    DEFAULT_MSG
    CLANG_FORMAT_BIN)

mark_as_advanced(
    CLANG_FORMAT_BIN)

if(ClangFormat_FOUND)
    # A CMake script to find all source files and setup clang-format targets for them
    add_executable(clang::format IMPORTED)
    set_target_properties(clang::format PROPERTIES
                          IMPORTED_LOCATION "${CLANG_FORMAT_BIN}")
endif()
