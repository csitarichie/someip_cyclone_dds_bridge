
# The FapiSettings interface target contains all requirements for all FAPI
# libraries and executables.
#
# All FAPI target must add FapiSettings as target link library to inherit
# common settings (e.g. compiler options). This is achieved by using the
# rapi_add_component and rapi_add_test functions provided by the fapi modules.

add_library(RapiSettings INTERFACE)
target_compile_features(RapiSettings INTERFACE cxx_std_17)

if("${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(rapi_GCC_LIKE_COMPILER_FLAGS
        -fno-strict-aliasing
        -fno-exceptions
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        -fdiagnostics-show-option
        -Wall
        -Wextra
        -Wdisabled-optimization
        -Wformat=2
        -Winit-self
        -Wmissing-include-dirs
        -Wshadow
        -Wstrict-overflow=5
        -Wcast-align
        -DNO_CAR_SCOPEDISPLAY_DLL
        $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
        $<$<COMPILE_LANGUAGE:CXX>:-Wsign-promo>)

rapi_debug_log_("Compiler is '${CMAKE_CXX_COMPILER_ID}'")

# The CMAKE_CXX_COMPILER_ID parameter is used to differentiate between
# different compilers and influence e.g. the compiler flags.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")

    rapi_debug_log_("Compiler is '${CMAKE_CXX_COMPILER_ID}' setting it up")

    # setting MSVC compiler settings are crap mostly can be done via global vars.
    if (MSVC_VERSION LESS 142)
        rapi_message_(FATAL_ERROR "MSVC version must be at least '142 = VS 2019 (16.0)' ")
    endif ()

    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    string(JOIN ";" CMAKE_COMMON_MSVC_FLAGS "/W4" "/WX" "/wd4206" "/wd4100" "/wd4200" "/wd4706" "/wd4204" "/wd4132"
            "/wd4324" "/wd4267" "/wd4098" "/wd4244" "/wd4127" "/wd4201" "/wd5220" "/wd5219" "/D_ENABLE_ATOMIC_ALIGNMENT_FIX"
            "/D_CRT_SECURE_NO_WARNINGS" "/D_USE_MATH_DEFINES" "/D_SCL_SECURE_NO_WARNINGS"
            "/D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
            "/D_ENABLE_EXTENDED_ALIGNED_STORAGE"
            )

    removeelement(${CMAKE_C_FLAGS} CMAKE_C_FLAGS "/W3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GR-")
    target_compile_options(RapiSettings INTERFACE ${CMAKE_COMMON_MSVC_FLAGS})


    removeelement(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS "/GR")
    removeelement(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS "/EHsc")
    removeelement(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS "/W3")
    removeelement(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS "/DUNICODE")
    removeelement(${CMAKE_CXX_FLAGS} CMAKE_CXX_FLAGS "/D_UNICODE")

    target_compile_options(RapiSettings INTERFACE "-D_HAS_EXCEPTIONS=0")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:10000000 /MaxILKSize:0x7FF00000")
    if(${CMAKE_SYSTEM_NAME} STREQUAL "WindowsStore")
        set(CMAKE_SYSTEM_NAME "Windows")
    endif()

    string(TOUPPER "${CMAKE_CONFIGURATION_TYPES}" _conftypesUC)

    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
            string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()

    set(CMAKE_CROSSCOMPILING OFF)

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU"
        OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang"
        OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")

        # apple compiler clang-1001.0.46.4 does not implement the following warnings
        # when -Werror is on -> only enable them when using the GNU compiler.
        if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
            list(APPEND rapi_GCC_LIKE_COMPILER_FLAGS
                    $<$<COMPILE_LANGUAGE:CXX>:-Wstrict-null-sentinel>
                    $<$<COMPILE_LANGUAGE:CXX>:-Wnoexcept>
                    -static-libstdc++)
        endif()

        # Add CXX and linker options for coverage build type
        if(${CMAKE_BUILD_TYPE} STREQUAL "Coverage")
            list(APPEND rapi_GCC_LIKE_COMPILER_FLAGS
                    -fprofile-arcs
                    -ftest-coverage
                    -O0)
            set(rapi_GCC_LIKE_LINKER_FLAGS
                    -lgcov
                    --coverage)
        endif()

        target_compile_options(RapiSettings INTERFACE ${rapi_GCC_LIKE_COMPILER_FLAGS})
        target_link_options(RapiSettings INTERFACE ${rapi_GCC_LIKE_LINKER_FLAGS})
        if("${CMAKE_SYSTEM_NAME}" STREQUAL "MSYS")
            target_link_libraries(atomic)
        endif()


else()
    rapi_message_(FATAL_ERROR "Unsupported compiler type:'${CMAKE_CXX_COMPILER_ID}'.")
endif()

# There are target properties which are not inherited transitively
# from an interface target like the ones bellow (see the problem at
# https://gitlab.kitware.com/cmake/cmake/issues/17183):
#
# set_target_properties(RapiSettings PROPERTIES CXX_EXTENSIONS OFF)
# set_target_properties(RapiSettings PROPERTIES CMAKE_CXX_STANDARD_REQUIRED ON)
#
# As a workaround a function is called for each RAPI cmake target to set these
# target properties directly on the target.

function(rapi_add_target_cxx_requirements_ rapi_target)
    set_target_properties(${rapi_target} PROPERTIES CXX_EXTENSIONS OFF)
    set_target_properties(${rapi_target} PROPERTIES CXX_STANDARD_REQUIRED ON)
endfunction()
