set(STM32_IMC_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(stm32-imc INTERFACE)

target_include_directories(stm32-imc INTERFACE "${STM32_IMC_INCLUDE_DIR}")

target_sources(
    stm32-imc INTERFACE

    "${STM32_IMC_INCLUDE_DIR}/containers/UnorderedRingBuffer.hpp"
    "${STM32_IMC_INCLUDE_DIR}/containers/TripleBuffer.hpp"
    "${STM32_IMC_INCLUDE_DIR}/containers/Span.hpp"
    "${STM32_IMC_INCLUDE_DIR}/containers/DynamicArray.hpp"

    "${STM32_IMC_INCLUDE_DIR}/imc/ImcMasterControl.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/ImcProtocol.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/ImcReceiver.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/ImcSender.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/ImcSettings.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/ImcSlaveControl.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/InterMcuCommunicationModule.hpp"
    "${STM32_IMC_INCLUDE_DIR}/imc/UartLock.hpp"

    "${STM32_IMC_INCLUDE_DIR}/misc/Callback.hpp"
    "${STM32_IMC_INCLUDE_DIR}/misc/Macro.hpp"
    "${STM32_IMC_INCLUDE_DIR}/misc/Meta.hpp"

    "${STM32_IMC_INCLUDE_DIR}/peripheral/Button.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/CrcBase.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/GpioBase.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/InterruptTimerBase.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/Pins.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/UartBase.hpp"
    "${STM32_IMC_INCLUDE_DIR}/peripheral/UsTimerBase.hpp"
)
