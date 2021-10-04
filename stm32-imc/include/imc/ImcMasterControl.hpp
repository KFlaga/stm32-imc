#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/ImcReceiver.hpp>
#include <imc/ImcSender.hpp>
#include <imc/ImcSettings.hpp>
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
class ImcMasterControl
{
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

    template<typename SendMessage>
    void updateStatus(SendMessage)
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

    template<typename SendMessage>
    bool dispatchControlMessage(SendMessage sendMessage, std::uint8_t id, std::uint16_t sequence, std::uint8_t size, std::uint8_t* data)
    {
    	if(id == ImcProtocol::Handshake::myId)
    	{
    		return handleHandshake(sendMessage, data, size, sequence);
    	}
    	else if(id == ImcProtocol::KeepAlive::myId)
    	{
    		return handleKeepAlive(sendMessage, data, size, sequence);
    	}
    	else if(id == ImcProtocol::ReceiveError::myId)
    	{
    		return handleError();
    	}
    	return false;
    }

    void onMessageSent()
    {
    }

    void onMessageReceived()
    {
    	communicationTimeoutTimer = 0;
    }

private:
    template<typename SendMessage>
    bool handleHandshake(SendMessage sendMessage, std::uint8_t*, std::uint8_t size, std::uint16_t sequence)
    {
    	if(size != ImcProtocol::Handshake::dataSize)
    	{
    		return false;
    	}

        ImcProtocol::Acknowledge ack{};
        ack.data.ackId = ImcProtocol::Handshake::myId;
        ack.data.ackSequence = sequence;
    	sendMessage(ack);

		communicationIsEstablished = true;

    	return true;
    }

    template<typename SendMessage>
    bool handleKeepAlive(SendMessage sendMessage, std::uint8_t*, std::uint8_t size, std::uint16_t sequence)
    {
    	if(size != ImcProtocol::KeepAlive::dataSize)
    	{
    		return false;
    	}

    	if(communicationIsEstablished)
    	{
            ImcProtocol::Acknowledge ack{};
            ack.data.ackId = ImcProtocol::KeepAlive::myId;
            ack.data.ackSequence = sequence;
        	sendMessage(ack);
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
    std::uint32_t communicationTimeoutTimer = 0;
    bool communicationIsEstablished = false;
};

}
