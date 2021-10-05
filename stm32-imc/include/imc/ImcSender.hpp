#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/UartLock.hpp>
#include <peripheral/UartBase.hpp>
#include <containers/DynamicArray.hpp>

namespace DynaSoft
{

/// Wraps UART peripheral and buffers an additional message for transmission.
///
/// Type Uart should be have UartBase interface
template<typename Uart, std::uint8_t maxMessageSize>
class ImcSender
{
public:
    using MessageBuffer = DynamicArray<std::uint8_t, maxMessageSize>;

    static_assert(Uart::sendBufferSize >= maxMessageSize, "Uart sendBufferSize is too small");

    ImcSender(Uart& uart_) :
        uart{uart_},
        messageBuffer{}
    {
        uart.setDataSentCallback({[](CallbackContext ctx)
        {
            static_cast<ImcSender*>(ctx)->onDataSent();
        }, this});
    }

    /// Enqueues given message for sending - up to two messages may be queued
    /// Returns true if there was space in queue
    template<typename MessageT>
    bool sendMessage(MessageT& msg)
    {
        std::uint8_t* data = reinterpret_cast<std::uint8_t*>(&msg);
        return sendMessage(data, sizeof(MessageT));
    }

    bool sendMessage(std::uint8_t* data, std::uint8_t size)
    {
        UartSendLock lock{uart};
        if(uart.send(data, size))
        {
            capacity = 1;
            return true;
        }
        else if(capacity > 0)
        {
            messageBuffer.assign(data, data + size);
            hasMessageInBuffer = true;;
            capacity = 0;
            return true;
        }
        else
        {
            return false;
        }
    }

    std::uint8_t queueCapacity()
    {
        return capacity;
    }

private:
    void onDataSent()
    {
        uart.generateIdleLine();
        if(hasMessageInBuffer)
        {
            std::uint8_t* data = reinterpret_cast<std::uint8_t*>(messageBuffer.data());
            uart.send(data, messageBuffer.size());
            hasMessageInBuffer = false;
        }
        capacity += 1;
    }

    Uart& uart;
    MessageBuffer messageBuffer{};
    volatile std::uint8_t capacity = 2;
    volatile bool hasMessageInBuffer = false;
};

}
