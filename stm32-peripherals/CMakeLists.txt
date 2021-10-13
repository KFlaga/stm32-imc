file(GLOB CMSIS_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/STM32F10x_StdPeriph_Driver/src/*.c"
)

add_library(
    cmsis-stm32f10x STATIC 
    ${CMSIS_SOURCES}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/system_stm32f10x.c"
)

target_include_directories(
    cmsis-stm32f10x PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/CMSIS/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/CMSIS/Device/ST/STM32F10x/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/STM32F10x_StdPeriph_Driver/inc"
)

add_library(
    stm32-peripherals STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/Crc.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/Gpio.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/InterruptTimer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/Irq.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/Uart.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/stm32/peripheral/UsTimer.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Gpio.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/InterruprTimer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Uart.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UsTimer.cpp"
)

target_link_libraries(stm32-peripherals stm32-imc cmsis-stm32f10x)
target_include_directories(stm32-peripherals PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
