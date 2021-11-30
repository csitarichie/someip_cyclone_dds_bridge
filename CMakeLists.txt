cmake_minimum_required(VERSION 3.22)

project(SOMEIP_CYCLONE_DDS_BRIDGE
     DESCRIPTION "small brain dump of possible someip cyclone DDS bridge implementation"
     HOMEPAGE_URL https://github.com/csitarichie/someip_cyclone_dds_bridge
     LANGUAGES C CXX
    )

message("CMake version:'${CMAKE_VERSION}'")

include(FetchContent)

FetchContent_Declare(
    iceoryx
    GIT_REPOSITORY https://github.com/eclipse-iceoryx/iceoryx.git
    SOURCE_SUBDIR iceoryx_meta
)

FetchContent_Declare(
    CycloneDDS
    GIT_REPOSITORY https://github.com/eclipse-cyclonedds/cyclonedds.git
)

FetchContent_Declare(
    vsomeip
    GIT_REPOSITORY https://github.com/COVESA/vsomeip.git
)

FetchContent_MakeAvailable(iceoryx CycloneDDS vsomeip)

if(NOT TARGET CycloneDDS::ddsc)
  message(FATAL_ERROR "CycloneDDS::ddsc Target not available")
endif()

if(NOT TARGET iceoryx_posh::iceoryx_posh)
    message(FATAL_ERROR "iceoryx_posh::iceoryx_posh Target not available")
endif()

find_package(iceoryx_posh CONFIG REQUIRED)
find_package(iceoryx_hoofs CONFIG REQUIRED)

get_target_property(ICEORYX_CXX_STANDARD iceoryx_posh::iceoryx_posh CXX_STANDARD)
include(IceoryxPlatform)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
include(rapi)

enable_testing()

add_subdirectory(apps)
add_subdirectory(components)
add_subdirectory(rapi_core)
add_subdirectory(common)
