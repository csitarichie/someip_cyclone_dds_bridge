if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    return()
endif()

function(rapi_create_analyse_target)
    set(multiValueArgs COMMAND)
    set(options)
    set(oneValueArgs COMMENT TARGET DEPENDS WORKING_DIRECTORY)
    cmake_parse_arguments(rapi_FUNC_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    rapi_debug_log_("rapi_create_analyse_target(${ARGN})")

    list(JOIN rapi_FUNC_ARGS_COMMAND " " GEN_COMMAND)
    rapi_debug_log_("GEN_COMMAND: '${GEN_COMMAND}'")

    # adds error check
    # string(PREPEND GEN_COMMAND "#!/usr/bin/env bash\nset -euo pipefail\n")
    if (rapi_FUNC_ARGS_WORKING_DIRECTORY)
        list(JOIN "\n" GEN_COMMAND
            "mkdir ${rapi_FUNC_ARGS_WORKING_DIRECTORY}"
            "pushd ${rapi_FUNC_ARGS_WORKING_DIRECTORY} >/dev/null"
            "${GEN_COMMAND}"
            "popd >/dev/null")
    endif()
    string(PREPEND GEN_COMMAND "#!/usr/bin/env bash\n")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${rapi_FUNC_ARGS_TARGET}.sh"
         CONTENT "${GEN_COMMAND}\n")
    set(EXECUTE_GEN_COMMAND bash "${CMAKE_CURRENT_BINARY_DIR}/${rapi_FUNC_ARGS_TARGET}.sh")

    add_custom_target(${rapi_FUNC_ARGS_TARGET}
                      COMMENT "${rapi_FUNC_ARGS_COMMENT}"
                      COMMAND ${EXECUTE_GEN_COMMAND}
                      DEPENDS "${rapi_FUNC_ARGS_DEPENDS}" _sync_pipenv
    )
    rapi_message_(STATUS "Adding analyse target: '${rapi_FUNC_ARGS_TARGET}'")

    set_property(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
        ADDITIONAL_MAKE_CLEAN_FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${rapi_FUNC_ARGS_TARGET}.sh"
    )
endfunction()

set(ALL_SOURCE_FILES "$<JOIN:$<TARGET_PROPERTY:__all_cxx_components,INTERFACE_SOURCES>, >")
set(ALL_HEADER_FILES "$<JOIN:$<TARGET_PROPERTY:__all_cxx_components,_rapi_COMPONENT_HEADER>, >")
set(ALL_FILES ${ALL_HEADER_FILES} ${ALL_SOURCE_FILES})

# build format/lint commands
find_package(ClangFormat)

if (ClangFormat_FOUND)
    # enable verbose mode when running clang-format
    if (rapi_VERBOSE_CHECKS OR rapi_VERBOSE_CHECKS_CXX)
        set(CLANG_FORMAT_ARGS "--verbose")
        set(CLANG_FORMAT_CHECK_ARGS "-v")
    endif ()

    rapi_create_analyse_target(COMMAND
                               "$<TARGET_PROPERTY:clang::format,IMPORTED_LOCATION>"
                               "${CLANG_FORMAT_ARGS}"
                               "-style=file"
                               "-i"
                               ${ALL_FILES}
                               TARGET format_cxx
                               COMMENT "Running clang-format to change files")

    rapi_create_analyse_target(COMMAND
                               "${CMAKE_SOURCE_DIR}/scripts/cmake/clang_format_check.sh"
                               "-b $<TARGET_PROPERTY:clang::format,IMPORTED_LOCATION>"
                               "${CLANG_FORMAT_CHECK_ARGS}"
                               ${ALL_FILES}
                               TARGET format_check_cxx
                               COMMENT "Checking for clang-format violations")

else ()
    rapi_message_(WARNING "clang-format not found. Not setting up format targets")
endif ()

find_package(ClangTidy)

if (ClangTidy_FOUND)
    rapi_create_analyse_target(TARGET tidy_cxx
                               COMMENT "Running clang-tidy to change files"
                               DEPENDS
                               __all_cxx_interface_gen
                               COMMAND
                               "${CLANG_TIDY_COMMAND}"
                               "-fix"
                               ${ALL_SOURCE_FILES})

    rapi_create_analyse_target(TARGET tidy_check_cxx
                               COMMENT "Checking for clang-tidy violations"
                               DEPENDS
                               __all_cxx_interface_gen
                               COMMAND
                               "${CLANG_TIDY_COMMAND}"
                               ${ALL_SOURCE_FILES})
else ()
    rapi_message_(WARNING "clang-tidy not found. No clang-tidy analysis target")
endif ()
