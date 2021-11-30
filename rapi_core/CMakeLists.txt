rapi_add_component(
    SOURCE
        actor.cpp
        core.cpp
        environment.cpp
        port.cpp
        priority.cpp
    INTERFACE_HEADER
        actor.hpp
        core.hpp
        core_types.hpp
        environment.hpp
        message.hpp
        network.hpp
        port.hpp
        priority.hpp
        impl/async_call_back_vector.hpp
        impl/actor_life_cycle.hpp
        impl/call_back_vector.hpp
        impl/common.hpp
    DEPENDS
        adstlog_cxx
        fmt
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(RapiCore PUBLIC Threads::Threads)
