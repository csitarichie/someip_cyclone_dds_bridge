option(ADSTLOG_TRACE_LEVEL_CORE_COMPILED_IN OFF
    "Enable TRACE level logging in core. Note: This is very verbose and has a big performance penalty, so the default is OFF")

rapi_add_component(
    TARGET adstlog_cxx
    SOURCE
        adstlog.cpp
    INTERFACE_HEADER
        adstlog.hpp
        p7_trace_wrapper.hpp
    DEPENDS
        P7
        fmt
        config_cxx
)

if(ADSTLOG_TRACE_LEVEL_CORE_COMPILED_IN)
    target_compile_definitions(adstlog_cxx PUBLIC ADSTLOG_TRACE_LEVEL_CORE_COMPILED_IN)
endif()
