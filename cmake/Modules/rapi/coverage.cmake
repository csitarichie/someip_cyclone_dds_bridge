if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "Windows")
    return()
endif()

find_program(GCOV_EXECUTABLE gcov)
find_program(GCOVR_EXECUTABLE gcovr)

if (GCOV_EXECUTABLE AND GCOVR_EXECUTABLE)
    # create target to call "gcovr -r .. -s -e '.*\.pb\.cc|.*\.pb\.h' --exclude-directories '.*/test' ."

    # the directory for HTML ouput - clean it, too
    set (rapi_COVERAGE_DIR "${CMAKE_BINARY_DIR}/rapi_coverage")
    set_property(DIRECTORY "${CMAKE_SOURCE_DIR}" APPEND PROPERTY
                 ADDITIONAL_MAKE_CLEAN_FILES
                 "${rapi_COVERAGE_DIR}"
                 "${CMAKE_CURRENT_BINARY_DIR}/cov_sum.txt")

    # get the number of cores for parallel execution
    include(ProcessorCount)
    ProcessorCount(PROCESSOR_COUNT)
    if(PROCESSOR_COUNT EQUAL 0)
        set(PROCESSOR_COUNT 1)
    endif()

    # exclude patterns: ProtoBuf files and test directories
    string(JOIN "|" GCOVR_EXCLUDE_FILE_PATTERN
           ".*\\.pb\\.cc"
           ".*\\.pb\\.h"
           ".*/test/.*"
          )
    string(JOIN "|" GCOVR_EXCLUDE_DIR_PATTERN
#           ".*/test"
          )

    # command line options
    set(GCOVR_OPTIONS
        --root ${CMAKE_SOURCE_DIR}
        --print-summary
        --exclude '${GCOVR_EXCLUDE_FILE_PATTERN}'
        --exclude-unreachable-branches
#        --exclude-directories '${GCOVR_EXCLUDE_DIR_PATTERN}'
        -j ${PROCESSOR_COUNT}
       )

    # command line options to generate html output
    set(GCOVR_HTML_OPTIONS
        ${GCOVR_OPTIONS}
        --print-summary
        --output ${rapi_COVERAGE_DIR}/coverage.html
        --html-details
        --html-title "RAPI code coverage report]"
       )

    add_custom_target("coverage"
                      COMMENT "Running gcovr"
                      COMMAND ${GCOVR_EXECUTABLE} ${GCOVR_OPTIONS} ${CMAKE_CURRENT_BINARY_DIR} | tee cov_sum.txt
                      DEPENDS _sync_pipenv
                     )

    add_custom_target("coverage_html"
                      COMMENT "Generating HTML code coverage report"
                      COMMAND ${CMAKE_COMMAND} -E make_directory ${rapi_COVERAGE_DIR}
                      COMMAND ${GCOVR_EXECUTABLE} ${GCOVR_HTML_OPTIONS} ${CMAKE_CURRENT_BINARY_DIR}
                      DEPENDS _sync_pipenv
                     )
else ()
    rapi_message_(WARNING "gcov or gcovr not found. Code coverage measurement is not available")
endif ()
