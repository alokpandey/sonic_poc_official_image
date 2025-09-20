# SONiC POC C++ Components CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(SONiC_POC_CPP VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sai
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp
    ${CMAKE_CURRENT_SOURCE_DIR}/swss
    ${CMAKE_CURRENT_SOURCE_DIR}/syncd
    ${CMAKE_CURRENT_SOURCE_DIR}/common
)

# Find required packages
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

# Find SONiC libraries (optional)
pkg_check_modules(SWSSCOMMON libswsscommon)
pkg_check_modules(SAIREDIS libsairedis)

# Include SONiC headers
include_directories(/usr/local/include)
if(SWSSCOMMON_FOUND)
    include_directories(${SWSSCOMMON_INCLUDE_DIRS})
    add_definitions(-DHAVE_SWSSCOMMON)
endif()
if(SAIREDIS_FOUND)
    include_directories(${SAIREDIS_INCLUDE_DIRS})
    add_definitions(-DHAVE_SAIREDIS)
endif()

# Mock SAI library (fallback for testing)
add_library(mock_sai STATIC
    mock/mock_sai.cpp
)

# Common utilities library
add_library(sonic_common STATIC
    common/logger.cpp
    common/utils.cpp
)

# BSP library
add_library(sonic_bsp STATIC
    bsp/platform_health_monitor.cpp
    bsp/led_controller.cpp
    bsp/platform_api.cpp
)

target_link_libraries(sonic_bsp
    sonic_common
    Threads::Threads
)

# SAI library
add_library(sonic_sai STATIC
    sai/sai_adapter.cpp
    sai/sai_vlan_manager.cpp
    sai/sai_route_manager.cpp
    sai/sai_port_manager.cpp
)

target_link_libraries(sonic_sai
    mock_sai
    sonic_common
    Threads::Threads
    dl
)

# Link SONiC libraries if available
if(SWSSCOMMON_FOUND)
    target_link_libraries(sonic_sai ${SWSSCOMMON_LIBRARIES})
endif()
if(SAIREDIS_FOUND)
    target_link_libraries(sonic_sai ${SAIREDIS_LIBRARIES})
endif()

# SwSS library
add_library(sonic_swss STATIC
    swss/orchagent.cpp
    swss/portsorch.cpp
    swss/vlanorch.cpp
    swss/routeorch.cpp
)

target_link_libraries(sonic_swss
    sonic_sai
    sonic_common
    mock_sai
    Threads::Threads
)

# Link SONiC libraries if available
if(SWSSCOMMON_FOUND)
    target_link_libraries(sonic_swss ${SWSSCOMMON_LIBRARIES})
endif()
if(SAIREDIS_FOUND)
    target_link_libraries(sonic_swss ${SAIREDIS_LIBRARIES})
endif()

# Syncd library
add_library(sonic_syncd STATIC
    syncd/syncd.cpp
    syncd/sai_redis.cpp
)

target_link_libraries(sonic_syncd
    sonic_sai
    sonic_common
    mock_sai
    Threads::Threads
)

# Main SONiC application
add_executable(sonic_poc
    main.cpp
)

target_link_libraries(sonic_poc
    sonic_bsp
    sonic_sai
    sonic_swss
    sonic_syncd
    sonic_common
    mock_sai
    Threads::Threads
)

# BSP Use Case Demo
add_executable(bsp_demo
    demos/bsp_demo.cpp
)

target_link_libraries(bsp_demo
    sonic_bsp
    sonic_common
    Threads::Threads
)

# SAI Use Case Demo
add_executable(sai_demo
    demos/sai_demo.cpp
)

target_link_libraries(sai_demo
    sonic_sai
    sonic_common
    mock_sai
    Threads::Threads
)

# SAI Command Processor for Python-C++ Communication
add_executable(sai_command_processor
    sai/sai_command_processor.cpp
)

target_link_libraries(sai_command_processor
    sonic_sai
    sonic_common
    mock_sai
    Threads::Threads
)

# Unit tests (if enabled)
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()

    # Find Google Test
    find_package(GTest QUIET)
    if(GTest_FOUND AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        add_subdirectory(tests)
    else()
        message(WARNING "Google Test not found or tests directory missing, skipping tests")
    endif()
endif()

# Installation
install(TARGETS sonic_poc bsp_demo sai_demo sai_command_processor
    RUNTIME DESTINATION bin
)

install(TARGETS sonic_bsp sonic_sai sonic_swss sonic_syncd sonic_common
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY sai/ bsp/ swss/ syncd/ common/
    DESTINATION include/sonic
    FILES_MATCHING PATTERN "*.h"
)

# Package configuration
set(CPACK_PACKAGE_NAME "sonic-poc-cpp")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SONiC POC C++ Components")
set(CPACK_PACKAGE_VENDOR "SONiC POC Team")
set(CPACK_GENERATOR "DEB;RPM;TGZ")

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "SONiC POC C++ Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Build tests: ${BUILD_TESTS}")
message(STATUS "")
