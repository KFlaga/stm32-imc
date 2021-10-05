#pragma once

#include <containers/DynamicArray.hpp>
#include <misc/Callback.hpp>
#include <peripheral/InterruptTimerBase.hpp>
#include <array>
#include <cstdint>

namespace DynaSoft
{

/// Base class for UART peripheral, that abstracts hardware and implements some of send/receive logic.
///
/// \tparam Derived actual implementation of UART.
///
/// Class Derived needs to add UartBase as friend.
template<typename Derived, std::uint8_t sendBufferSize_, typename InterruptTimer>
class UartBase
{
public:
    using IdleCallback = Callback<void(CallbackContext)>;
    using RxCallback = Callback<void(CallbackContext)>;
    using TxCallback = Callback<void(CallbackContext)>;
    using ErrorCallback = Callback<void(CallbackContext, std::uint8_t)>;
    static constexpr std::uint8_t sendBufferSize = sendBufferSize_;

    UartBase(InterruptTimer& irqTimer_, std::uint32_t checkForIdleTimeUs_, std::uint32_t generateIdleTimeUs_) :
        irqTimer{irqTimer_},
        checkForIdleTimeUs{checkForIdleTimeUs_},
        generateIdleTimeUs{generateIdleTimeUs_}
    {
    }

    /// Starts UART communication.
    void turnOn()
    {
        static_cast<Derived*>(this)->_turnOn();
    }

    /// Stops UART communication
    void turnOff()
    {
        static_cast<Derived*>(this)->_turnOff();
    }

    /// Returns true if Uart is currently sending a message.
    bool isTransmitOngoing() const
    {
        return isTransmiting;
    }

    /// Enqueues data for sending.
    /// Only one message of max size sendBufferSize may be enqueued at the time.
    /// If there was space in queue returns true.
    /// After whole data is transmitted callback registered in setDataSentCallback() is called.
    bool send(std::uint8_t* data, std::uint8_t size)
    {
        if(size > 0 && size <= sendBufferSize && !isTransmiting)
        {
            isTransmiting = true;
            sendQueue.assign(data, data + size);
            sendQueueIndex = 1;
            sendByte(data[0]);
            return true;
        }
        return false;
    }

    /// Returns last received byte.
    /// Valid only after onDataRead() callback is fired at least once.
    std::uint8_t read()
    {
        return lastReceived;
    }

    /// Generates IDLE on Tx - so stop sending for some time.
    /// IDLE indicates end of message for receiver end.
    void generateIdleLine()
    {
        isGeneratingIdle = true;
        irqTimer.scheduleInterrupt(0, generateIdleTimeUs, {[](CallbackContext ctx, std::uint32_t)
        {
            UartBase& self = *static_cast<UartBase*>(ctx);
            self.isGeneratingIdle = false;
            if(self.sendQueueIndex < self.sendQueue.size())
            {
                self.sendByte(self.sendQueue[self.sendQueueIndex++]);
            }
        }, this});
    }

    /// Stops Uart transmit finish interrupt from firing.
    void suspendSend()
    {
        static_cast<Derived*>(this)->_suspendSend();
    }

    /// Resumes Uart transmit finish interrupts.
    void resumeSend()
    {
        static_cast<Derived*>(this)->_resumeSend();
    }

    /// Stops Uart data received interrupt from firing.
    void suspendReceive()
    {
        static_cast<Derived*>(this)->_suspendReceive();
    }

    /// Resumes Uart data received interrupts.
    void resumeReceive()
    {
        static_cast<Derived*>(this)->_resumeReceive();
    }

    /// Called when IDLE was detected on Rx.
    void setIdleLineDetectedCallback(IdleCallback callback)
    {
        onIdleLineDetected = callback;
    }

    /// Fires after every byte is received.
    /// Received byte is available via function read().
    void setDataReceivedCallback(RxCallback callback)
    {
        onDataReceived = callback;
    }

    /// Fires after whole send buffer is transmitted.
    /// isTransmitOngoing() will always be false when called from callback.
    /// Another message may be send from this callback.
    void setDataSentCallback(TxCallback callback)
    {
        onDataSent = callback;
    }

    /// Fires after an error is detected during receiving.
    void setReceiveErrorCallback(ErrorCallback callback)
    {
        onReceiveError = callback;
    }

protected:
    void sendByte(std::uint8_t data)
    {
        static_cast<Derived*>(this)->_sendByte(data);
    }

    std::uint8_t receiveByte()
    {
        return static_cast<Derived*>(this)->_receiveByte();
    }

    void handleTransmissionComplete()
    {
        if(!isGeneratingIdle)
        {
            if(sendQueueIndex < sendQueue.size())
            {
                sendByte(sendQueue[sendQueueIndex++]);
            }
            else
            {
                isTransmiting = false;
                onDataSent();
            }
        }
    }

    void handleDataReceived()
    {
        lastReceived = receiveByte();

        // Reset wait time for idle
        irqTimer.scheduleInterrupt(1, checkForIdleTimeUs, {[](CallbackContext ctx, std::uint32_t)
        {
            static_cast<UartBase*>(ctx)->onIdleLineDetected();
        }, this});

        onDataReceived();
    }

    InterruptTimer& irqTimer;

    DynamicArray<std::uint8_t, sendBufferSize> sendQueue{};
    std::uint8_t sendQueueIndex = 0;

    volatile bool isTransmiting = false;
    volatile bool isGeneratingIdle = false;
    volatile std::uint8_t lastReceived = 0;

    std::uint32_t checkForIdleTimeUs = 0;
    std::uint32_t generateIdleTimeUs = 0;

    IdleCallback onIdleLineDetected{};
    RxCallback onDataReceived{};
    TxCallback onDataSent{};
    ErrorCallback onReceiveError{};
};

}
