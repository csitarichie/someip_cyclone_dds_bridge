# Constant Directory definitions for FAPI make install
# FAPI strictly follows GNU layout.
include(GNUInstallDirs)


set(RAPI_INSTALL_RESOURCE_DIR "${CMAKE_INSTALL_DATADIR}/rapi/resource"
        CACHE INTERNAL
        "Install prefix (path) for component resource files." FORCE)

# global target and modules setup
if (NOT TARGET __all_cxx_components)
    add_library(__all_cxx_components INTERFACE)
    define_property(TARGET
            PROPERTY _RAPI_COMPONENT_HEADER
            INHERITED
            BRIEF_DOCS "Collects all component headers (e.g. for clang-format)."
            FULL_DOCS "Collects all component headers (e.g. for clang-format).")

endif ()

if (NOT TARGET __all_cxx_interface_gen)
    add_library(__all_cxx_interface_gen INTERFACE)
endif ()

# Collects the following executable properties added by rapi_add_component function
# target logincal name
# target output name
# target source directory
# target binary directory
if (NOT TARGET __all_add_component_exes)
    add_library(__all_add_component_exes INTERFACE)
    define_property(TARGET
            PROPERTY _RAPI_COMPONENT_EXE_TARGETS
            INHERITED
            BRIEF_DOCS "Collects all exe targets from add_component."
            FULL_DOCS "Collects all exe targets from add_component.")
    define_property(TARGET
            PROPERTY _RAPI_COMPONENT_EXE_OUTPUT_NAMES
            INHERITED
            BRIEF_DOCS "Collects all exe target output names from add_component."
            FULL_DOCS "Collects all exe target output names from add_component.")
    define_property(TARGET
            PROPERTY _RAPI_COMPONENT_EXE_SRC_DIRS
            INHERITED
            BRIEF_DOCS "Collects all exe target source directory names from add_component."
            FULL_DOCS "Collects all exe target source directory names from add_component.")
    define_property(TARGET
            PROPERTY _RAPI_COMPONENT_EXE_BIN_DIRS
            INHERITED
            BRIEF_DOCS "Collects all exe target binary directory names from add_component."
            FULL_DOCS "Collects all exe target binary directory names from add_component.")
endif ()

# macro can be used only from rapi_add_component
# collects target properties from exes.
macro(_add_target_to_component_exes)
    set_property(TARGET __all_add_component_exes
            APPEND PROPERTY _RAPI_COMPONENT_EXE_TARGETS
            "${RAPI_ADD_COMPONENT_TARGET}")

    get_target_property(TARGET_OUT_NAME "${RAPI_ADD_COMPONENT_TARGET}" OUTPUT_NAME)
    # in case the OUTPUT_NAME is not set then cmake uses target name
    if (${TARGET_OUT_NAME} STREQUAL "TARGET_OUT_NAME-NOTFOUND")
        set(TARGET_OUT_NAME "${RAPI_ADD_COMPONENT_TARGET}")
    endif ()
    set_property(TARGET __all_add_component_exes
            APPEND PROPERTY _RAPI_COMPONENT_EXE_OUTPUT_NAMES
            "${TARGET_OUT_NAME}")

    get_target_property(TARGET_SRC_DIR "${RAPI_ADD_COMPONENT_TARGET}" SOURCE_DIR)
    set_property(TARGET __all_add_component_exes
            APPEND PROPERTY _RAPI_COMPONENT_EXE_SRC_DIRS
            "${TARGET_SRC_DIR}")

    get_target_property(TARGET_BIN_DIR "${RAPI_ADD_COMPONENT_TARGET}" BINARY_DIR)
    set_property(TARGET __all_add_component_exes
            APPEND PROPERTY _RAPI_COMPONENT_EXE_BIN_DIRS
            "${TARGET_BIN_DIR}")
endmacro()

# we enable (c)tests in top level of the build tree
enable_testing()

# helper functions
macro(rapi_add_component_check_input_)
    set(multiValueArgs SOURCE MPS_SOURCE DEPENDS INTERFACE_HEADER PRIVATE_HEADER)
    set(options SHARED EXE INTERFACE)
    set(oneValueArgs TARGET)
    cmake_parse_arguments(RAPI_ADD_COMPONENT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    rapi_debug_log_("rapi_add_component(${ARGN})")

    # Basic error checks
    if (RAPI_ADD_COMPONENT_SHARED AND RAPI_ADD_COMPONENT_EXE)
        rapi_message_(FATAL_ERROR "SHARED and EXE option is specified, only one of them can be active at a time")
    endif ()

    if (RAPI_ADD_COMPONENT_INTERFACE)
        if (RAPI_ADD_COMPONENT_SOURCE)
            rapi_message_(FATAL_ERROR "SOURCE parameter shall not be provided for INTERFACE components")
        endif ()

        if (NOT RAPI_ADD_COMPONENT_INTERFACE_HEADER)
            rapi_message_(FATAL_ERROR
                    "Please provide INTERFACE_HEADER parameter! E.g.: 'INTERFACE_HEADER hello_world.hpp'")
        endif ()

        if (RAPI_ADD_COMPONENT_PRIVATE_HEADER)
            rapi_message_(FATAL_ERROR "INTERFACE component can only have INTERFACE_HEADER not PRIVATE_HEADER")
        endif ()
    elseif (NOT (RAPI_ADD_COMPONENT_SOURCE OR RAPI_ADD_COMPONENT_MPS_SOURCE))
            rapi_message_(FATAL_ERROR "Please provide SOURCE parameter! E.g.: 'SOURCE hello_world.cpp'")
    endif()

    get_filename_component(DIR_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    if (NOT RAPI_ADD_COMPONENT_TARGET)
        snake_to_camel_case("${DIR_FILE_NAME}" RAPI_ADD_COMPONENT_TARGET)
    endif ()

    rapi_debug_log_("RAPI_ADD_COMPONENT_MPS_SOURCE:'${RAPI_ADD_COMPONENT_MPS_SOURCE}'")
    rapi_debug_log_("RAPI_ADD_COMPONENT_SOURCE:'${RAPI_ADD_COMPONENT_SOURCE}'")

endmacro()

macro(rapi_add_component_cxx_)
    # As of now RAPI components are either object or shared libraries.
    # Shared libraries get installed whereas object libraries can only be
    # used in the current build tree.
    if (RAPI_ADD_COMPONENT_SHARED)
        set(RAPI_LIB_TYPE_ "SHARED")
    elseif (RAPI_ADD_COMPONENT_INTERFACE)
        set(RAPI_LIB_TYPE_ "INTERFACE")
    endif ()

    list(TRANSFORM RAPI_ADD_COMPONENT_SOURCE PREPEND "src/")
    list(TRANSFORM RAPI_ADD_COMPONENT_PRIVATE_HEADER PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/src/")
    list(TRANSFORM RAPI_ADD_COMPONENT_INTERFACE_HEADER PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/include/${DIR_FILE_NAME}/")

    if (RAPI_ADD_COMPONENT_EXE)
        rapi_message_(STATUS "Creating cxx executable '${RAPI_ADD_COMPONENT_TARGET}'")
        add_executable(${RAPI_ADD_COMPONENT_TARGET}
                ${RAPI_ADD_COMPONENT_SOURCE})

        _add_target_to_component_exes(${RAPI_ADD_COMPONENT_TARGET})

        install(TARGETS ${RAPI_ADD_COMPONENT_TARGET}
                EXPORT Rapi
                RUNTIME
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT Runtime)
    else ()
        rapi_message_(STATUS "Creating cxx ${RAPI_LIB_TYPE_} library '${RAPI_ADD_COMPONENT_TARGET}'")
        add_library(${RAPI_ADD_COMPONENT_TARGET} ${RAPI_LIB_TYPE_}
                ${RAPI_ADD_COMPONENT_SOURCE})
    endif ()

    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include")
        rapi_debug_log_(
                "target '${RAPI_ADD_COMPONENT_TARGET}' public include dir '${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
        set(COMPONENT_INCLUDE_COMMAND include)
        if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/win")
            list(APPEND COMPONENT_INCLUDE_COMMAND "include/win")
        elseif(("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"
                 OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin"
                 OR "${CMAKE_SYSTEM_NAME}" STREQUAL "MSYS")
                AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/include/posix")
            list(APPEND COMPONENT_INCLUDE_COMMAND "include/posix")
        endif()
    endif ()

    set(COMPONENT_PRIVATE_INCLUDE src)
    if("${CMAKE_SYSTEM_NAME}" STREQUAL "Windows" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/win")
        list(APPEND COMPONENT_PRIVATE_INCLUDE "src/win")
    elseif(("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux"
            OR "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin"
            OR "${CMAKE_SYSTEM_NAME}" STREQUAL "MSYS")
            AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/posix")
        list(APPEND COMPONENT_PRIVATE_INCLUDE "src/posix")
    endif()

    if (NOT RAPI_ADD_COMPONENT_INTERFACE)
        # the predefined way of handling include dirs by RAPI convention
        target_include_directories(${RAPI_ADD_COMPONENT_TARGET}
                PUBLIC ${COMPONENT_INCLUDE_COMMAND}
                PRIVATE ${COMPONENT_PRIVATE_INCLUDE})

        # All rapi targets must inherit the FapiSettings.
        # This is an interface target containing project requirements as interface
        # target properties (thus avoiding pollution of global CMAKE variables).
        target_link_libraries(${RAPI_ADD_COMPONENT_TARGET}
                PRIVATE RapiSettings
                PUBLIC
                ${RAPI_ADD_COMPONENT_DEPENDS})

        # setting target properties which can not be a part of an interface library
        rapi_add_target_cxx_requirements_(${RAPI_ADD_COMPONENT_TARGET})
    else ()
        # interface targets only have interface folder
        target_include_directories(${RAPI_ADD_COMPONENT_TARGET}
                INTERFACE ${COMPONENT_INCLUDE_COMMAND})

        # interface targets inherit only interface libs.
        target_link_libraries(${RAPI_ADD_COMPONENT_TARGET}
                INTERFACE RapiSettings
                ${RAPI_ADD_COMPONENT_DEPENDS})
    endif ()

    if(rapi_ADD_COMPONENT_MPS_SOURCE)
        target_link_libraries(${RAPI_ADD_COMPONENT_TARGET} PUBLIC ${RAPI_ADD_COMPONENT_TARGET}_mps_cxx)
    endif()

    rapi_debug_log_("adding sources to __all_cxx_components: ${RAPI_ADD_COMPONENT_SOURCE}")
    target_sources(__all_cxx_components INTERFACE
            ${RAPI_ADD_COMPONENT_SOURCE})

    string(APPEND HEADER_DEBUG
            "adding headers to __all_cxx_components:_RAPI_COMPONENT_HEADER:"
            "${RAPI_ADD_COMPONENT_INTERFACE_HEADER}, "
            "${RAPI_ADD_COMPONENT_PRIVATE_HEADER}")
    rapi_debug_log_(${HEADER_DEBUG})
    set_property(TARGET __all_cxx_components
            APPEND PROPERTY _RAPI_COMPONENT_HEADER
            ${RAPI_ADD_COMPONENT_INTERFACE_HEADER}
            ${RAPI_ADD_COMPONENT_PRIVATE_HEADER})

    string(CONCAT RAPI_ADD_COMPONENT_OBJECT_DIR
            "${CMAKE_CURRENT_BINARY_DIR}/"
            "CMakeFiles/"
            "${RAPI_ADD_COMPONENT_TARGET}.dir/")
    set(RAPI_ADD_COMPONENT_OBJECT ${RAPI_ADD_COMPONENT_SOURCE})
    list(TRANSFORM RAPI_ADD_COMPONENT_OBJECT PREPEND "${RAPI_ADD_COMPONENT_OBJECT_DIR}")
    set(RAPI_ADD_COMPONENT_GCDA ${RAPI_ADD_COMPONENT_OBJECT})
    set(RAPI_ADD_COMPONENT_GCNO ${RAPI_ADD_COMPONENT_OBJECT})

    list(TRANSFORM RAPI_ADD_COMPONENT_GCDA APPEND ".gcda")
    list(TRANSFORM RAPI_ADD_COMPONENT_GCNO APPEND ".gcno")
    set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
            ADDITIONAL_MAKE_CLEAN_FILES
            "${RAPI_ADD_COMPONENT_GCDA}"
            "${RAPI_ADD_COMPONENT_GCNO}")

    target_link_libraries(__all_cxx_components INTERFACE ${RAPI_ADD_COMPONENT_TARGET})
    add_dependencies(__all_cxx_components ${RAPI_ADD_COMPONENT_TARGET})
endmacro()

#! rapi_add_interface creates a MPS generated interface for use of the component lib.
#
# DESCRIPTION:
#
# This function is meant to be used in internally by the rapi_add_component()
# to declare an RAPI interface component. It expects at least one SOURCE file.
#
# It creates:
#
#  *  source interface target for include path inheritance on MPS level
#  *  interface c++ target
#
# In case that the TARGET parameter is omitted, the components CMake TARGET name gets
# derived from the component source dir (see function rapi_add_component for more
# details).
#
# \param:TARGET CMake target name.
#
#               The following naming conventions apply
#
#                   *  source interface target:   mps_<target_name>
#                   *  mps generated c++ target:  <target_name>_mps_cxx
#
# MANDATORY parameters:
#
# \param:MPS_SOURCE List of MPS source files (*.mps). It must be only the source file
#               names located in the ${CMAKE_CURRENT_SOURCE_DIR}/interface folder
#
# \param:DEPENDS List of source interface targets which this targets depends on.
#
# OPTIONS :
#
# none for now
#
function(rapi_add_interface_)

    rapi_debug_log_("rapi_add_interface(${ARGN})")

    # Basic error checks
    get_filename_component(DIR_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    if (NOT RAPI_ADD_COMPONENT_TARGET)
        snake_to_camel_case("${DIR_FILE_NAME}" RAPI_ADD_COMPONENT_TARGET)
    endif ()

    if (NOT RAPI_ADD_COMPONENT_MPS_SOURCE)
        rapi_message_(FATAL_ERROR "rapi_add_interface at least one SOURCE must be specified")
    endif ()

    set(RAPI_ADD_INTERFACE_SOURCE_ABS ${RAPI_ADD_COMPONENT_MPS_SOURCE})
    list(TRANSFORM RAPI_ADD_INTERFACE_SOURCE_ABS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/interface/")

    # first creating interface source target
    # creating interface target for the MPS sources
    rapi_message_(STATUS "Creating MPS interface 'mps_${RAPI_ADD_COMPONENT_TARGET}'")
    add_library("mps_${RAPI_ADD_COMPONENT_TARGET}" INTERFACE)
    target_sources("mps_${RAPI_ADD_COMPONENT_TARGET}" INTERFACE ${RAPI_ADD_INTERFACE_SOURCE_ABS})

    # set INTERFACE_INCLUDE_DIRECTORIES for MPS imports
    target_include_directories("mps_${RAPI_ADD_COMPONENT_TARGET}" INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/interface)

    # this is the necessary step to collect all dependent interface dirs as
    # include path and to get all sources in all levels of dependency.
    set(MPS_DEPENDS ${RAPI_ADD_COMPONENT_DEPENDS})
    list(TRANSFORM MPS_DEPENDS PREPEND "mps_")
    if(MPS_DEPENDS)
        target_link_libraries("mps_${RAPI_ADD_COMPONENT_TARGET}" INTERFACE ${MPS_DEPENDS})
    endif()

    # 2nd step to create the custom commands and targets
    # Poor man's nested list in cmake to collect MPS generator arguments and calls.
    # the '#' character is used as list separator for the 2nd level nesting
    string(JOIN "#" MPSGEN_CXX_CALL
            "_mps_cxx"
            ""
            ""
            "mps_cxx_gen"
            "hpp cpp")
    list(APPEND MPSGEN_CALL_ARGS "${MPSGEN_CXX_CALL}")


    # The big trick here is the generator expression to create the -I argument list for the MPS compiler.
    # There is a big implication with it, the generator expression in custom commands.
    # It puts extra "" around itself so if it passed to VERBATIM command (see later on)
    # it won't be treated any more as separate argument list but as a string
    # with several "-I ... -I ..." components. The cmake dev list suggested a workaround to put the command in to a file
    # and then execute it in the custom command. So the boilerplate code below is due to this limitation.
    set(IMPORT_PATHS "$<TARGET_PROPERTY:mps_${RAPI_ADD_COMPONENT_TARGET},INTERFACE_INCLUDE_DIRECTORIES>")

    foreach (MPS_ARG ${MPSGEN_CALL_ARGS})
        # first making real list from the 2nd level nest.
        string(REGEX REPLACE "#" ";" MPS_ARG ${MPS_ARG})
        rapi_debug_log_("MPS_ARG:${MPS_ARG}")

        list(GET MPS_ARG 0 TARGET_POSTFIX)
        list(GET MPS_ARG 1 MPS_OUTPUT_ARGS)
        list(GET MPS_ARG 2 MPS_EXTRA_ARGS)
        list(GET MPS_ARG 3 OUTPUT_DIR)
        list(GET MPS_ARG 4 OUTPUT_EXTENTIONS)

        string(JOIN "/" OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}" "${OUTPUT_DIR}")

        # library target
        set(LIB_TARGET_NAME "${RAPI_ADD_COMPONENT_TARGET}${TARGET_POSTFIX}")
        # code generation target
        set(GEN_TARGET_NAME "gen_${LIB_TARGET_NAME}")

        string(REGEX REPLACE " " ";" OUTPUT_EXTENTIONS ${OUTPUT_EXTENTIONS})
        rapi_debug_log_("OUTPUT_EXTENTIONS:${OUTPUT_EXTENTIONS}")

        string(REGEX REPLACE " " ";" MPS_OUTPUT_ARGS "${MPS_OUTPUT_ARGS}")
        set(MPS_OUTPUT_GEN_LIST "")
        foreach (MPS_OUTPUT_ARG ${MPS_OUTPUT_ARGS})
            string(APPEND MPS_OUTPUT_GEN_LIST "${MPS_OUTPUT_ARG}${OUTPUT_DIR}" " ")
        endforeach ()

        # RSZRSZ TODO this is the generator call part where we emulate gen call
        # by copying files from the gen_hand_code dir in to the build dir
        # as they would come as a result of the gen step.
        string(JOIN " " GEN_MPS_COMMANDS
                "${CMAKE_COMMAND} -E copy_directory"
                # "$<$<BOOL:${IMPORT_PATHS}>:-I$<JOIN:${IMPORT_PATHS}, -I>>"
                "${CMAKE_SOURCE_DIR}/gen_hand_code/rapi_cpp_gen/components/${DIR_FILE_NAME}"
                # "${MPS_OUTPUT_GEN_LIST}"
                # "${MPS_EXTRA_ARGS}"
                #"$<JOIN:${rapi_ADD_INTERFACE_SOURCE_ABS}, >")
                "${OUTPUT_DIR}/${DIR_FILE_NAME}")

        # adds error check
        # string(PREPEND GEN_MPS_COMMANDS "#!/usr/bin/env bash\nset -euo pipefail\n")
        string(PREPEND GEN_MPS_COMMANDS "#!/usr/bin/env bash\n")
        rapi_debug_log_("GEN_MPS_COMMANDS:${GEN_MPS_COMMANDS}")

        set(GEN_MPS_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/${LIB_TARGET_NAME}.sh")
        file(GENERATE OUTPUT "${GEN_MPS_SCRIPT}"
                CONTENT "${GEN_MPS_COMMANDS}\n")

        # setup output file list
        unset(OUTPUT_FILE_LIST)
        # temp solution it is just for now until no real files are generated.
        # the input files are needed this shall be a part of the MPS generator job
        unset(INPUT_FILE_LST)
        foreach (ITF_SOURCE ${RAPI_ADD_COMPONENT_MPS_SOURCE})
            get_filename_component(SOURCE_BASE_NAME "${ITF_SOURCE}" NAME_WE)
            foreach (OUTPUT_EXT ${OUTPUT_EXTENTIONS})
                string(JOIN "/" SOURCE_PATH
                        "${OUTPUT_DIR}"
                        "${DIR_FILE_NAME}"
                        "${SOURCE_BASE_NAME}.${OUTPUT_EXT}")
                list(APPEND OUTPUT_FILE_LIST "${SOURCE_PATH}")
                # this is temporary
                unset(SOURCE_PATH)
                string(JOIN "/" SOURCE_PATH
                        "${CMAKE_SOURCE_DIR}/gen_hand_code/rapi_cpp_gen/components"
                        "${DIR_FILE_NAME}"
                        "${SOURCE_BASE_NAME}.${OUTPUT_EXT}")
                list(APPEND INPUT_FILE_LST "${SOURCE_PATH}")

            endforeach ()
        endforeach ()

        rapi_debug_log_("OUTPUT_FILE_LIST:${OUTPUT_FILE_LIST}")
        # this is temporary
        rapi_debug_log_("INPUT_FILE_LST:${INPUT_FILE_LST}}")

        # This is the real code generation step. Here the trick is that the custom command needs
        # file level dependency. This is achieved by using target property INTERFACE_SOURCES
        # this is transitively populated by cmake at build time. Therefore a generator expression is used.
        # Make this generator target depend on all dependencies source files is a
        # neat way using generator expressions in a custom command DEPENDS list.
        add_custom_command(
                OUTPUT ${OUTPUT_FILE_LIST}
                COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_DIR}"
                COMMAND ${RAPI_DEBUG_COMMAND_LOG_SCRIPT} "${GEN_MPS_SCRIPT}"
                COMMAND bash "${GEN_MPS_SCRIPT}"
                DEPENDS
                "${GEN_MPS_SCRIPT}"
                $<TARGET_PROPERTY:mps_${RAPI_ADD_COMPONENT_TARGET},INTERFACE_SOURCES>
                # this is temporary
                "${INPUT_FILE_LST}"
                "${CMAKE_BINARY_DIR}/gen_global_mps_cxx.stamp"
                # $<TARGET_PROPERTY:protobuf::protoc,IMPORTED_LOCATION> RSZRSZ TODO need to add here the gen BIN
                COMMENT "Generating MPS ${TARGET_POSTFIX} interface lib ${LIB_TARGET_NAME}"
                VERBATIM
        )

        set_source_files_properties(
                ${OUTPUT_FILE_LIST}
                PROPERTIES GENERATED TRUE
        )

        set_property(
                DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" APPEND PROPERTY
                ADDITIONAL_MAKE_CLEAN_FILES
                "${OUTPUT_DIR}"
                "${GEN_MPS_SCRIPT}"
        )

        add_custom_target(${GEN_TARGET_NAME}
                DEPENDS "${OUTPUT_FILE_LIST}"
                COMMENT "Scanning dependencies mps_${RAPI_ADD_COMPONENT_TARGET}")

        # Make this generator target depend on generator targets from dependency.
        # Unlike the cpp/c target dependency which takes care of rebuilding the dependency tree
        # this is not the case with the custom target tree
        # (that problem is solved with direct file dependency from above).
        # This dependency is used by cmake to determine the order of calling
        # the generator on the dependency tree.
        foreach (DEP ${RAPI_ADD_COMPONENT_DEPENDS})
            add_dependencies(${GEN_TARGET_NAME} "gen_${DEP}${TARGET_POSTFIX}")
        endforeach ()
        add_dependencies(${GEN_TARGET_NAME} gen_global_mps_cxx)

        # setting up cxx lib and its dependencies this is used to link
        # against the user code lib
        # it transitively populates all interface hierarchy
        if (${TARGET_POSTFIX} MATCHES "_mps_cxx$")

            add_dependencies(__all_cxx_interface_gen ${GEN_TARGET_NAME})

            rapi_message_(STATUS "Creating interface target '${LIB_TARGET_NAME}'")

            # the real target which depends on the generation trigger target
            add_library(${LIB_TARGET_NAME} STATIC ${OUTPUT_FILE_LIST})
            foreach (DEP ${RAPI_ADD_COMPONENT_DEPENDS})
                target_link_libraries(${LIB_TARGET_NAME} PUBLIC
                        "${DEP}${TARGET_POSTFIX}")
            endforeach ()

            add_dependencies(${LIB_TARGET_NAME}
                    ${GEN_TARGET_NAME})
            target_include_directories(${LIB_TARGET_NAME}
                    PUBLIC "${OUTPUT_DIR}")

            # mps_cxx generated libs needs rapi_core
            target_link_libraries(${LIB_TARGET_NAME}
                    PUBLIC
                        RapiCore
                    PRIVATE
                        RapiSettings)

        endif()
    endforeach ()
endfunction()

#! rapi_add_component creates an fapi component.
#
# DESCRIPTION:
#
# This function is meant to be used in the RAPI cmake files to declare a RAPI source component. It
# requires at least one SOURCE file (except for the INTERFACE component type). It declares a cmake
# library or executable target.
#
# In case that the TARGET parameter is omitted, the components CMake TARGET name gets derived from
# the component source dir by converting its name from snake case to camel case notation.
#
# Example:
#
#     directory name: hello_world_server
#     CMake target name: HelloWorldServer
#
# OPTIONAL parameters:
#
# \param:TARGET CMake target name.
#
#               If TARGET is omitted the component directory name converted to CamelCase notation
#               will be used as TARGET name.
#
#               The default target type is an STATIC library but this can be modified using one of
#               the following options:
#
#               * EXE: Creates an executable target.
#               * SHARED Creates a shared library.
#               * INTERFACE: Creates an interface target.
#
#               The EXE, SHARED and INTERFACE options are mutually exclusive.
#
# \param:SOURCE List of C++ or C source files.
#
#               For C++, the prefix ${CMAKE_CURRENT_SOURCE_DIR}/src will automatically be
#
#               SOURCE must list all cpp source files for C++ components and all C files for
#               C components
#
#
# \param:INTERFACE_HEADER List of C++ header files.
#
#                         If target type is C++ and it has interface headers to be used by other
#                         components they must be specified here
#
#                         This parameter is mandatory for INTERFACE only components
#
#
# \param: PRIVATE_HEADER  List of C++ private header files.
#
#                         If target is C++ and it has private headers it must be specified here
#
#
# \param:DEPENDS List of library targets which this target depends on.
#
# \param:MPS_SOURCE list of mps source files in the interface directory of the component.
#
# \option:EXE Generates an executable. The generated executable is also installed with fapi package.
#
# \option:SHARED Generates a shared library. The target's include headers and the generated shared
#                library are also installed with fapi package.
#
# \option:INTERFACE Generates a cmake INTERFACE library. This option is useful for header only
#                   libraries (in this case no SOURCE parameter is required).
#
#
function(rapi_add_component)
    rapi_add_component_check_input_(${ARGN})

    if (RAPI_ADD_COMPONENT_MPS_SOURCE)
        rapi_add_interface_()
    endif ()

    if (RAPI_ADD_COMPONENT_SOURCE OR RAPI_ADD_COMPONENT_INTERFACE)
        rapi_add_component_cxx_()
    else()
        # if there is no user lib created then the generated interface target is aliased to the
        # component target so the users of the component can ignore the fact that the
        # component only an interface probably only message definition component.
        add_library(${RAPI_ADD_COMPONENT_TARGET} ALIAS ${RAPI_ADD_COMPONENT_TARGET}_mps_cxx)
    endif ()

    install(DIRECTORY resource/
            DESTINATION "${FAPI_INSTALL_RESOURCE_DIR}/${DIR_FILE_NAME}"
            OPTIONAL)
endfunction()
