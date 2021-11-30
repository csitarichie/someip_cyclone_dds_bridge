rapi_add_component(
    TARGET adstutil_cxx
    SOURCE
        error_handler.cpp
    INTERFACE_HEADER
        compiler_diagnostics.hpp
        gtest_timeout.hpp
        gtest_util.hpp
        type_id.hpp
        make_array.hpp
        error_handler.hpp
        static_string.hpp
)
