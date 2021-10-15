#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/ImcReceiver.hpp>
#include <imc/ImcSender.hpp>
#include <imc/ImcSettings.hpp>
#include <imc/ImcRecipient.hpp>
#include <peripheral/UartBase.hpp>

namespace DynaSoft
{

/// Handles control messages of IMC for master device.
/// Establishes communication with slave.
///
/// On reset state listens for Handshake message.
/// When received responses with Acknowledge and sets communicationEstablished flags.
///
/// Then checks if any message was received in some period (ImcSettings::masterCommunicationTimeoutUs).
/// If not then assumes slave device was reset and moves to reset state itself.
/// Responds to KeepAlive messages with Acknowledge.
///
/// Should handle receiver errors but for now they are just ignored - no application requires it.
template<typename Uart, std::uint8_t maxMessageSize>
class ImcMasterControl : public ImcRecipent<
        ImcMasterControl<Uart, maxMessageSize>,
        ImcProtocol::controlMessageRecipient,
        ImcProtocol::Handshake,
        ImcProtocol::KeepAlive,
        ImcProtocol::ReceiveError
    >
{
    template<typename, std::uint8_t, typename...>
    friend class ImcRecipent; // for handleMessage to be private

public:
    using ReceivedMessage = typename ImcReceiver<Uart, maxMessageSize>::MessageBuffer;

    ImcMasterControl(
        Uart& uart_,
        ImcReceiver<Uart, maxMessageSize>& receiver_,
        ImcSender<Uart, maxMessageSize>& sender_,
        ImcSettings& settings_
    ) :
        uart{uart_},
        receiver{receiver_},
        sender{sender_},
        settings{settings_}
    {
    }

    void updateTimers(std::uint32_t loopUs)
    {
        communicationTimeoutTimer += loopUs;
    }

    template<typename ImcModule>
    void updateStatus(ImcModule&)
    {
        if(communicationTimeoutTimer >= settings.masterCommunicationTimeoutUs)
        {
            communicationIsEstablished = false;
        }
    }

    bool hasCommunicationEstablished() const
    {
        return communicationIsEstablished;
    }

    void onMessageSent()
    {
    }

    void onMessageReceived()
    {
        communicationTimeoutTimer = 0;
    }

private:
    template<typename ImcModule>
    bool handleMessage(ImcProtocol::Handshake& m, ImcModule& imc)
    {
        ImcProtocol::Acknowledge ack{};
        ack.data.ackId = ImcProtocol::Handshake::myId;
        ack.data.ackSequence = m.sequence;
        imc.sendMessage(ack);

        communicationIsEstablished = true;

        return true;
    }

    template<typename ImcModule>
    bool handleMessage(ImcProtocol::KeepAlive& m, ImcModule& imc)
    {
        if(communicationIsEstablished)
        {
            ImcProtocol::Acknowledge ack{};
            ack.data.ackId = ImcProtocol::KeepAlive::myId;
            ack.data.ackSequence = m.sequence;
            imc.sendMessage(ack);
        }
        return true;
    }

    template<typename ImcModule>
    bool handleMessage(ImcProtocol::ReceiveError& m, ImcModule& imc)
    {
        return true;
    }

    Uart& uart;
    ImcReceiver<Uart, maxMessageSize>& receiver;
    ImcSender<Uart, maxMessageSize>& sender;

    ImcSettings& settings;
    std::uint32_t communicationTimeoutTimer = 0;
    bool communicationIsEstablished = false;
};

}
