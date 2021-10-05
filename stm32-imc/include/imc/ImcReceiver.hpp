#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/UartLock.hpp>
#include <containers/DynamicArray.hpp>
#include <containers/TripleBuffer.hpp>
#include <peripheral/UartBase.hpp>
#include <optional>

namespace DynaSoft
{

/// Wraps UART peripheral and buffers up to 2 received messages.
///
/// \tparam Uart Concrete implementation of UartBase class.
/// \tparam bufferSize Size of message buffers - should be equal to at least maximum expected message size.
template<typename Uart, std::uint8_t bufferSize>
class ImcReceiver
{
public:
    using MessageBuffer = DynamicArray<std::uint8_t, bufferSize>;

    /// Creates object using given uart implementation, which is used throughout entire lifespan of this object.
    /// Registers Uart callbacks related to receiving data.
    ImcReceiver(Uart& uart_) :
        uart{uart_},
        messageBuffer{}
    {
        uart.setIdleLineDetectedCallback({[](CallbackContext ctx)
        {
            static_cast<ImcReceiver*>(ctx)->onIdleLineDetected();
        }, this});
        uart.setDataReceivedCallback({[](CallbackContext ctx)
        {
            static_cast<ImcReceiver*>(ctx)->onDataReceived();
        }, this});
        uart.setReceiveErrorCallback({[](CallbackContext ctx, std::uint8_t error)
        {
            static_cast<ImcReceiver*>(ctx)->onReceiveError(error);
        }, this});
    }

    /// Retrieves next message from queue (up to two messages may be queued) if one is available.
    /// It is then removed from queue.
    /// Returned pointer is valid only until next call to 'getNextMessage()'.
    std::optional<MessageBuffer*> getNextMessage()
    {
        if(newMessagesCount > 0)
        {
            UartReceiveLock lock{uart};
            messageBuffer.read().clear();
            messageBuffer.swapRead();
            newMessagesCount--;
            if(newMessagesCount > 0)
            {
                messageBuffer.swapWrite();
            }

            MessageBuffer& message = messageBuffer.read();
            return &message;
        }
        else
        {
            return {};
        }
    }

    /// Returns true if UART detected an error or there was no space in buffer to store received message.
    /// If true no further messages are received until error is cleared.
    bool hasError() const
    {
        return hasReceiveError;
    }

    /// Clears error, so next message may be received.
    void clearError()
    {
        hasReceiveError = false;
    }

private:
    void onIdleLineDetected()
    {
        if(isReceiveReady && !hasReceiveError)
        {
            if(messageBuffer.write().size() > 0)
            {
                if(newMessagesCount == 0)
                {
                    // Intermediate buffer should be cleared in getNextMessage(), so after this call
                    // in write() we have clean buffer for next message and just received message in read()
                    messageBuffer.swapWrite();
                }
                newMessagesCount++;
            }
        }
        else if(hasReceiveError && newMessagesCount < 2)
        {
            messageBuffer.write().clear();
        }
        isReceiveReady = true;
    }

    void onDataReceived()
    {
        if(hasReceiveError)
        {
            return;
        }

        if(newMessagesCount >= 2)
        {
            hasReceiveError = true;
            return;
        }

        if(messageBuffer.write().size() < bufferSize)
        {
            std::uint8_t x = uart.read();
            messageBuffer.write().push_back(x);
        }
        else
        {
            hasReceiveError = true;
        }
    }

    void onReceiveError(std::uint8_t)
    {
        hasReceiveError = true;
    }

private:
    Uart& uart;
    TripleBuffer<MessageBuffer> messageBuffer;

    bool isReceiveReady = false; // Receive is not ready until 1st idle after reset
    volatile bool hasReceiveError = false;
    volatile std::uint8_t newMessagesCount = 0;
};

}
