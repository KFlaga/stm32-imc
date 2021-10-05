list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(utils)

ensure_is_specified(TOOLCHAIN_PREFIX "arm-none-eabi-gcc")
ensure_is_specified(TARGET_TRIPLET "arm-none-eabi")

if(WIN32)
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

set(TOOLCHAIN_BIN_DIR "${TOOLCHAIN_PREFIX}/bin" CACHE INTERNAL "")
set(TOOLCHAIN_INC_DIR "${TOOLCHAIN_PREFIX}/${TARGET_TRIPLET}/include" CACHE INTERNAL "")
set(TOOLCHAIN_LIB_DIR "${TOOLCHAIN_PREFIX}/${TARGET_TRIPLET}/lib" CACHE INTERNAL "")

set(CMAKE_SYSTEM_NAME Generic CACHE INTERNAL "")
set(CMAKE_SYSTEM_PROCESSOR arm CACHE INTERNAL "")

set(CMAKE_C_COMPILER "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-gcc${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-g++${EXE_SUFFIX}" CACHE INTERNAL "")
SET(CMAKE_ASM_COMPILER "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-as${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_AR "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-ar${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_NM "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-nm${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_STRIP "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-strip${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_RANLIB "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-ranlib${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_OBJCOPY "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-objcopy${EXE_SUFFIX}" CACHE INTERNAL "")
set(CMAKE_OBJDUMP "${TOOLCHAIN_BIN_DIR}/${TARGET_TRIPLET}-objdump${EXE_SUFFIX}" CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_PREFIX}/${TARGET_TRIPLET}" CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM BOTH CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY CACHE INTERNAL "")
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY CACHE INTERNAL "")

include_directories(BEFORE "${TOOLCHAIN_INC_DIR}")

set(CMAKE_EXECUTABLE_SUFFIX_C   .elf)
set(CMAKE_EXECUTABLE_SUFFIX_CXX .elf)
set(CMAKE_EXECUTABLE_SUFFIX_ASM .elf)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)