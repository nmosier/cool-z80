cmake_minimum_required(VERSION 3.2)
project(midd_cool C CXX)

enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_BUILD_TYPE Debug)

if(APPLE)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformMacOS.cmake)
elseif(UNIX)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/PlatformLinuxGCC.cmake)
else()
    message(WARNING "Unsupported platform/compiler combination")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find appropriate Flex and Bison executables and libraries
find_package(BISON)
find_package(FLEX 2.5.35 REQUIRED)

# Set some useful compiler flags for all targets
set_directory_properties(PROPERTIES
    COMPILE_DEFINITIONS "$<$<CONFIG:Debug>:DEBUG>"
)
add_compile_options("-Wall;-Wno-unused-function")
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/src/include ${CMAKE_CURRENT_SOURCE_DIR}/pa5/z80_lib)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(pa2)
add_subdirectory(pa3)
add_subdirectory(pa4)
add_subdirectory(pa5)
add_subdirectory(pax)
