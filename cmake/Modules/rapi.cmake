# in the point of writing 3.19 is the highest version of cmake
# therefore RAPI starts with this version.
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

# generate compilation database - needed by clang-tidy before the target definitions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "generate compilation database" FORCE)

include(rapi/debug)
include(rapi/utils)

# Include RAPI build system in a top level.
# in case build environment has configuration from shell
if (DEFINED ENV{RAPI_CMAKE_MODULE_PATH})
    list(APPEND CMAKE_MODULE_PATH "$ENV{RAPI_CMAKE_MODULE_PATH}")
endif ()
if (DEFINED ENV{RAPI_CMAKE_PREFIX_PATH})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{RAPI_CMAKE_PREFIX_PATH}")
    message(STATUS "Adding '$ENV{RAPI_CMAKE_PREFIX_PATH}' to CMAKE_PREFIX_PATH")
endif ()

# main module includes
include(rapi/compiler_settings)
include(rapi/add_functions)
# clang-format and clang-tidy
include(rapi/static_analysis)
# gcov
include(rapi/coverage)
