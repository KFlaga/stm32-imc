cmake_minimum_required(VERSION 3.12)

project(stm32-imc CXX C ASM)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(utils)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

ensure_is_one_of(CMAKE_BUILD_TYPE "Release;Debug" "Release")
ensure_is_one_of(BUILD_TARGET "stm32;tests" "stm32")

include(toolchain-${BUILD_TARGET})
include(flags/common)
include(flags/${BUILD_TARGET})

add_subdirectory(stm32-imc)

if(${BUILD_TARGET} STREQUAL "stm32")
    add_subdirectory(stm32-peripherals)
    add_subdirectory(imc-example)
else()
    add_subdirectory(tests)
endif()
