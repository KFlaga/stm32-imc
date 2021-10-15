#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/ImcReceiver.hpp>
#include <imc/ImcSender.hpp>
#include <imc/ImcSettings.hpp>
#include <imc/ImcRecipient.hpp>
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
class ImcSlaveControl : public ImcRecipent<
        ImcSlaveControl<Uart, maxMessageSize>,
        ImcProtocol::controlMessageRecipient,
        ImcProtocol::Acknowledge,
        ImcProtocol::ReceiveError
    >
{
    template<typename, std::uint8_t, typename...>
    friend class ImcRecipent; // for handleMessage to be private

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

    template<typename ImcModule>
    void updateStatus(ImcModule& imc)
    {
        checkKeepAliveAckTimeout();
        sendNotification(imc);
    }

    bool hasCommunicationEstablished() const
    {
        return communicationIsEstablished;
    }

    void onMessageSent()
    {
        notificationTimer = 0;
    }

    void onMessageReceived()
    {
    }

private:
    template<typename ImcModule>
    void sendNotification(ImcModule& imc)
    {
        if(!communicationIsEstablished)
        {
            sendNotificationMessage<ImcProtocol::Handshake>(imc, settings.slaveHandshakeIntervalUs);
        }
        else
        {
            sendNotificationMessage<ImcProtocol::KeepAlive>(imc, settings.slaveKeepAliveIntervalUs);
        }
    }

    template<typename Message, typename ImcModule>
    void sendNotificationMessage(ImcModule& imc, std::uint32_t interval)
    {
        if(notificationTimer >= interval)
        {
            Message notification{};
            imc.sendMessage(notification);
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

    template<typename ImcModule>
    bool handleMessage(ImcProtocol::Acknowledge& m, ImcModule&)
    {
        if(!communicationIsEstablished)
        {
            if(m.data.ackId == ImcProtocol::Handshake::myId)
            {
                communicationIsEstablished = true;
                keepAliveAckTimeout = 0;
            }
        }
        else
        {
            if(m.data.ackId == ImcProtocol::KeepAlive::myId)
            {
                keepAliveAckTimeout = 0;
            }
        }
        return true;
    }

    template<typename ImcModule>
    bool handleMessage(ImcProtocol::ReceiveError&, ImcModule&)
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
