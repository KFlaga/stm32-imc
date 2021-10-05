#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/ImcReceiver.hpp>
#include <imc/ImcSender.hpp>
#include <imc/ImcSettings.hpp>
#include <peripheral/UartBase.hpp>

namespace DynaSoft
{

/// Handles control messages of IMC for slave device.
/// Establishes communication with master and keeps it alive.
///
/// On reset state sends Handshake periodically (every ImcSettings::slaveHandshakeIntervalUs)
/// until Acknowledge with Handshake::myId is received.
///
/// Then ensures that some message is sent periodically (at least every ImcSettings::slaveKeepAliveIntervalUs),
/// so that master know it is alive. It does that by sending KeepAlive message if other message wasn't sent
/// for this period.
///
/// If Acknowledge is not received for too long (ImcSettings::slaveAckTimeoutUs) assumes master was
/// reset and goes back to reset state.
///
/// Should handle receiver errors but for now they are just ignored - no application requires it.
template<typename Uart, std::uint8_t maxMessageSize>
class ImcSlaveControl
{
public:
    using ReceivedMessage = typename ImcReceiver<Uart, maxMessageSize>::MessageBuffer;

    ImcSlaveControl(
        Uart& uart_,
        ImcReceiver<Uart, maxMessageSize>& receiver_,
        ImcSender<Uart, maxMessageSize>& sender_,
        ImcSettings& settings_
    ) :
        uart{uart_},
        receiver{receiver_},
        sender{sender_},
        settings{settings_},
        notificationTimer{settings.slaveHandshakeIntervalUs}
    {
    }

    void updateTimers(std::uint32_t loopUs)
    {
        notificationTimer += loopUs;
        keepAliveAckTimeout += loopUs;
    }

    template<typename SendMessage>
    void updateStatus(SendMessage sendMessage)
    {
        checkKeepAliveAckTimeout();
        sendNotification(sendMessage);
    }

    bool hasCommunicationEstablished() const
    {
        return communicationIsEstablished;
    }

    template<typename SendMessage>
    bool dispatchControlMessage(SendMessage, std::uint8_t id, std::uint16_t, std::uint8_t size, std::uint8_t* data)
    {
        if(id == ImcProtocol::Acknowledge::myId)
        {
            return handleAck(data, size);
        }
        else if(id == ImcProtocol::ReceiveError::myId)
        {
            return handleError();
        }
        return false;
    }

    void onMessageSent()
    {
        notificationTimer = 0;
    }

    void onMessageReceived()
    {
    }

private:
    template<typename SendMessage>
    void sendNotification(SendMessage sendMessage)
    {
        if(!communicationIsEstablished)
        {
            sendNotificationMessage<ImcProtocol::Handshake>(sendMessage, settings.slaveHandshakeIntervalUs);
        }
        else
        {
            sendNotificationMessage<ImcProtocol::KeepAlive>(sendMessage, settings.slaveKeepAliveIntervalUs);
        }
    }

    template<typename Message, typename SendMessage>
    void sendNotificationMessage(SendMessage sendMessage, std::uint32_t interval)
    {
        if(notificationTimer >= interval)
        {
            Message notification{};
            sendMessage(notification);
        }
    }

    void checkKeepAliveAckTimeout()
    {
        if(communicationIsEstablished)
        {
            if(keepAliveAckTimeout >= settings.slaveAckTimeoutUs)
            {
                communicationIsEstablished = false;
            }
        }
    }

    bool handleAck(std::uint8_t* data, std::uint8_t size)
    {
        if(size != sizeof(ImcProtocol::AckMessageContents))
        {
            return false;
        }

        ImcProtocol::AckMessageContents& ack = *reinterpret_cast<ImcProtocol::AckMessageContents*>(data);
        if(!communicationIsEstablished)
        {
            if(ack.ackId == ImcProtocol::Handshake::myId)
            {
                communicationIsEstablished = true;
                keepAliveAckTimeout = 0;
            }
        }
        else
        {
            if(ack.ackId == ImcProtocol::KeepAlive::myId)
            {
                keepAliveAckTimeout = 0;
            }
        }
        return true;
    }

    bool handleError()
    {
        return true;
    }

    Uart& uart;
    ImcReceiver<Uart, maxMessageSize>& receiver;
    ImcSender<Uart, maxMessageSize>& sender;

    ImcSettings& settings;
    std::uint32_t notificationTimer = 0;
    std::uint32_t keepAliveAckTimeout = 0;
    bool communicationIsEstablished = false;
};

}
