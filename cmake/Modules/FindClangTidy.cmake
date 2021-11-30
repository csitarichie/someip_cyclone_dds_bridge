# look for clang-tidy, not the runner script - the runner script is in our repo
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-8 clang-tidy-7 clang-tidy)

if (CLANG_TIDY_EXECUTABLE)
    set(CLANG_TIDY_BUILD_PATH_ARG "-p=${CMAKE_BINARY_DIR}")
    set(CLANG_TIDY_CONFIG_ARG "-config=")
    set(CLANG_TIDY_BINARY_ARG "-clang-tidy-binary=${CLANG_TIDY_EXECUTABLE}")
    string(JOIN " " CLANG_TIDY_OTHER_ARGS "-extra-arg=-Wno-unknown-warning-option" "-quiet")

    string(JOIN " " CLANG_TIDY_ALL_ARGS
        "${CLANG_TIDY_BUILD_PATH_ARG}"
        "${CLANG_TIDY_CONFIG_ARG}"
        "${CLANG_TIDY_BINARY_ARG}"
        "${CLANG_TIDY_OTHER_ARGS}"
        )

    string(JOIN " " CLANG_TIDY_COMMAND
        "${PIPENV_EXECUTABLE} run python ${CMAKE_SOURCE_DIR}/scripts/cmake/run-clang-tidy/run-clang-tidy.py"
        "${CLANG_TIDY_ALL_ARGS}"
        )
endif ()

# handle the QUIETLY and REQUIRED arguments and set CLANG_TIDY_FOUND to TRUE if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
    ClangTidy
    DEFAULT_MSG
    CLANG_TIDY_EXECUTABLE)

mark_as_advanced(
    CLANG_TIDY_EXECUTABLE
    CLANG_TIDY_BINARY_ARG
    CLANG_TIDY_BUILD_PATH_ARG
    CLANG_TIDY_CONFIG_ARG
    CLANG_TIDY_OTHER_ARGS)
