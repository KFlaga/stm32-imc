#pragma once

#include <peripheral/UartBase.hpp>
#include <stm32/peripheral/Gpio.hpp>
#include <stm32/peripheral/InterruptTimer.hpp>
#include <misc/Macro.hpp>
#include <array>
#include <cstdint>

#include "stm32f10x_usart.h"

// Library should be built with UART_SEND_BUFFER_SIZE defined specific for given application
#ifndef UART_SEND_BUFFER_SIZE
#define UART_SEND_BUFFER_SIZE 32
#endif

namespace DynaSoft
{
class StmUart;
void uartIrqHandler(StmUart*);

enum class UartChannel : std::uint8_t
{
    Uart1 = 1,
    Uart2 = 2,
    Uart3 = 3
};

struct UartSettings
{
    UartChannel channel = UartChannel::Uart1;
    std::uint32_t baudRate = 921600;
    std::uint32_t checkForIdleTimeUs = 50;
    std::uint32_t generateIdleTimeUs = 100;
};

enum class UartError : std::uint8_t
{
    None = 0,
    ReceiveBufferOverrun,
    ReceiveNoise,
    ReceiveFrameError,
    ReceiveParityError,
};

class StmUart : public UartBase<StmUart, UART_SEND_BUFFER_SIZE, StmInterruptTimer>
{
public:
    StmUart(StmGpio& gpio, StmInterruptTimer& irqTimer, const UartSettings& settings);

protected:
    using Base = UartBase<StmUart, UART_SEND_BUFFER_SIZE, StmInterruptTimer>;
    friend Base; // Needed for UartBase to access protected methods

    void _turnOn();
    void _turnOff();
    void _suspendSend();
    void _resumeSend();
    void _suspendReceive();
    void _resumeReceive();

    void _sendByte(std::uint8_t data)
    {
        uart->DR = data;
    }

    std::uint8_t _receiveByte()
    {
        return static_cast<std::uint8_t>(uart->DR & 0xFF);
    }

    void _generateIdleLine();

private:
    friend void uartIrqHandler(StmUart*); // Needed to call private method from interrupt
    void handleUartIrq();

    void handleReceiveOverrunError();
    void handleReceiveNoiseError();
    void handleReceiveFrameError();
    void handleReceiveParityError();

    StmGpio& gpio;
    USART_TypeDef* uart = nullptr;
};

}
