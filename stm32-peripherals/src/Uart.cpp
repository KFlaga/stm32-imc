#include <stm32/peripheral/Uart.hpp>
#include <stm32/peripheral/Irq.h>
#include "misc.h"

namespace
{
DynaSoft::StmUart* gUart1 = nullptr;
DynaSoft::StmUart* gUart2 = nullptr;
DynaSoft::StmUart* gUart3 = nullptr;

constexpr std::uint32_t maxBaudRate = 2250 * 1000;
}

extern "C"
{
void USART1_IRQHandler(void)
{
    uartIrqHandler(gUart1);
}

void USART2_IRQHandler(void)
{
    uartIrqHandler(gUart2);
}

void USART3_IRQHandler(void)
{
    uartIrqHandler(gUart3);
}
}

namespace DynaSoft
{
void uartIrqHandler(StmUart* uart)
{
    uart->handleUartIrq();
}

StmUart::StmUart(StmGpio& gpio_, StmInterruptTimer& irqTimer_, const UartSettings& settings) :
    Base{irqTimer_, settings.checkForIdleTimeUs, settings.generateIdleTimeUs},
    gpio{gpio_}
{
    std::uint8_t irqn = 0;

    switch(settings.channel)
    {
    case UartChannel::Uart2:
        uart = USART2;
        irqn = USART2_IRQn;
        gUart2 = this;

        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
        gpio.port(ports::A).makeOutput(2, OutputType::afio | OutputType::fastSpeed); // Tx
        gpio.port(ports::A).makeInput(3, InputType::DigitalNormalLow); // Rx
        break;
    case UartChannel::Uart3:
        uart = USART3;
        irqn = USART3_IRQn;
        gUart3 = this;

        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
        gpio.port(ports::B).makeOutput(10, OutputType::afio | OutputType::fastSpeed); // Tx
        gpio.port(ports::B).makeInput(11, InputType::DigitalNormalLow); // Rx
        break;
    case UartChannel::Uart1:
    default:
        uart = USART1;
        irqn = USART1_IRQn;
        gUart1 = this;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
        gpio.port(ports::A).makeOutput(9, OutputType::afio | OutputType::fastSpeed); // Tx
        gpio.port(ports::A).makeInput(10, InputType::DigitalNormalLow); // Rx
        break;
    }

    USART_InitTypeDef uartInit{};
    uartInit.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    uartInit.USART_BaudRate = std::min(settings.baudRate, maxBaudRate);
    uartInit.USART_WordLength = USART_WordLength_9b;
    uartInit.USART_StopBits = USART_StopBits_1;
    uartInit.USART_Parity = USART_Parity_Even;
    uartInit.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(uart, &uartInit);

    // Enable interrupts
    uart->CR1 |= USART_CR1_PEIE;
    uart->CR1 |= USART_CR1_TCIE;
    uart->CR1 |= USART_CR1_RXNEIE;
    uart->CR1 |= USART_CR1_IDLEIE;
    uart->CR3 |= USART_CR3_EIE;

    NVIC_InitTypeDef uartNvicInit;
    uartNvicInit.NVIC_IRQChannel = irqn;
    uartNvicInit.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_GROUP_CRITICAL;
    uartNvicInit.NVIC_IRQChannelSubPriority = IRQ_PRIORITY_GROUP_LOW;
    uartNvicInit.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&uartNvicInit);
}

void StmUart::_turnOn()
{
    USART_Cmd(uart, ENABLE);
}

void StmUart::_turnOff()
{
    USART_Cmd(uart, DISABLE);
}

void StmUart::_suspendSend()
{
    uart->CR1 &= ~(USART_CR1_IDLEIE);
}

void StmUart::_resumeSend()
{
    // Interrupts for pending events will be called after this call
    uart->CR1 |= USART_CR1_IDLEIE;
}

void StmUart::_suspendReceive()
{
    uart->CR1 &= ~(USART_CR1_RXNEIE);
}

void StmUart::_resumeReceive()
{
    // Interrupts for pending events will be called after this call
    uart->CR1 |= USART_CR1_RXNEIE;
}

void StmUart::handleUartIrq()
{
    if(uart->SR & USART_FLAG_TC)
    {
        USART_ClearFlag(uart, USART_FLAG_TC);
        handleTransmissionComplete();
    }
    else if(uart->SR & USART_FLAG_RXNE)
    {
        // Some errors are checked here as well, as RXNE and error may appear in single interrupt
        if(uart->SR & USART_FLAG_PE)
        {
            handleReceiveParityError();
        }
        else if(uart->SR & USART_FLAG_NE)
        {
            handleReceiveNoiseError();
        }
        else if(uart->SR & USART_FLAG_FE)
        {
            handleReceiveFrameError();
        }
        else
        {
            handleDataReceived();
        }
    }
    else if(uart->SR & USART_FLAG_IDLE)
    {
        _receiveByte();
    }
    else if(uart->SR & USART_FLAG_ORE)
    {
        handleReceiveOverrunError();
    }
    else if(uart->SR & USART_FLAG_NE)
    {
        handleReceiveNoiseError();
    }
    else if(uart->SR & USART_FLAG_FE)
    {
        handleReceiveFrameError();
    }
}

void StmUart::handleReceiveOverrunError()
{
    // Receive buffer overrun error - so last read wasn't fast enough
    // Shouldn't really happen unless 'onDataReceive' is too long
    onReceiveError(static_cast<std::uint8_t>(UartError::ReceiveBufferOverrun));
    _receiveByte();
    USART_ClearFlag(uart, USART_FLAG_ORE);
}

void StmUart::handleReceiveNoiseError()
{
     _receiveByte();
    onReceiveError(static_cast<std::uint8_t>(UartError::ReceiveNoise));
}

void StmUart::handleReceiveFrameError()
{
    // Framing error - desynchronization or noise
     _receiveByte();
    onReceiveError(static_cast<std::uint8_t>(UartError::ReceiveFrameError));
}

void StmUart::handleReceiveParityError()
{
     _receiveByte();
    onReceiveError(static_cast<std::uint8_t>(UartError::ReceiveParityError));
}

}
