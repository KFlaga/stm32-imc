#pragma once

#include <imc/ImcProtocol.hpp>
#include <imc/ImcReceiver.hpp>
#include <imc/ImcSender.hpp>
#include <imc/ImcSettings.hpp>
#include <imc/ImcSlaveControl.hpp>
#include <imc/ImcMasterControl.hpp>
#include <imc/ImcRecipient.hpp>
#include <peripheral/UartBase.hpp>
#include <peripheral/CrcBase.hpp>
#include <misc/Callback.hpp>
#include <misc/Meta.hpp>
#include <misc/Assert.hpp>

namespace DynaSoft
{

/// Class used for communication between two MCUs using UART.
///
/// Very simple protocol is used for communication.
///
/// First two devices needs to establish connection - slave device sends Handshake and master device
/// responses with Acknowledge. Master marks marks connection as established when it receives Handshake,
/// slave when receives Acknowledge.
/// After connection is established slave must send any message in specified interval, otherwise
/// master resets connection state and stops responding. Slave keeps communication alive by sending
/// KeepAlive and should receive Acknowledge for each. If no Acknowledge is received for specified interval
/// it resets connection state and start sending Handshakes again.
///
/// This procedure is handled by ImcSlaveControl and ImcMasterControl classes.
///
/// Once connection is established user may send messages to other device.
/// Also registered callback is called when message with specified recipient number is received.
///
/// Every sent message have assigned a CRC value, which is validated on receiver side.
/// If message id is unexpected, received data size differs from expected, or crc differs from expected
/// receiver error is raised. This error is also raised when UART hardware detects a transmission error.
/// Message received with error is not dispatched to recipients and ReceiveError message is sent
/// to other device. However, for now they are ignored on the other side.
///
/// \tparam Uart Concrete implementation of UartBase class.
/// \tparam Crc Concrete implementation of CrcBase class.
/// \tparam maxMessageSize Maximum size of received and sent messages, should include fields in ImcProtocol::MessageBase.
/// \tparam isMaster Indicates whether device serves as master or slave.
///
/// \todo Do something with received ReceiverError
template<typename Uart, typename Crc, std::uint8_t maxMessageSize, bool isMaster = true>
class InterMcuCommunicationModule
{
private:
    using ReceivedMessage = typename ImcReceiver<Uart, maxMessageSize>::MessageBuffer;
    using ImcControl = std::conditional_t<isMaster, ImcMasterControl<Uart, maxMessageSize>, ImcSlaveControl<Uart, maxMessageSize>>;

public:
    /// Function signature for message recipients callbacks
    /// \param context Context passed to register function
    /// \param imc Reference to this ImcModule object
    /// \param id Id of received message
    /// \param dataSize Size of Message::Data
    /// \param data Pointer to Message object
    /// \return true if message is supported and has valid contents
    using MessageRecipientFunc = bool(*)(
        CallbackContext,
        InterMcuCommunicationModule&,
        std::uint8_t,
        std::uint8_t,
        std::uint8_t*
    );
    using MessageRecipient = Callback<MessageRecipientFunc>;

    InterMcuCommunicationModule(Uart& uart_, Crc& crc_, ImcSettings& settings_) :
        uart{ uart_ },
        crc{ crc_ },
        receiver{ uart },
        sender{ uart },
        control{ uart, receiver, sender, settings_ },
        settings{ settings_ }
    {
    }

    /// Registers callback that will be called when message with corresponding recipient number is received.
    ///
    /// \param recipientNumber Unique number of recipient module, should be one of {1, 2, 3}
    /// \param recipient Callback that will be called when message with corresponding recipient number is received.
    void registerMessageRecipient(std::uint8_t recipientNumber, MessageRecipient recipient)
    {
        dyna_assert(recipientNumber > 0 && recipientNumber < 4);
        recipients[recipientNumber-1] = recipient;
    }

    /// Should be called regularly from main loop.
    /// Updates control module and dispatches received messages.
    void update(std::uint32_t loopUs)
    {
        control.updateTimers(loopUs);

        while(handleReceivedMessage())
        {
        }

        if(receiver.hasError())
        {
            responseWithReceiveError();
            receiver.clearError();
        }

        control.updateStatus(*this);
    }

    /// Tries to send a message to other MCU.
    /// At most one application module (user) message may be enqueued at the time.
    /// Communication should also be established first.
    /// Returns true if message was successfully enqueued.
    ///
    /// Unless there were some errors with receiving only one control message should be sent at the time,
    /// leaving second ImcSender slot for user messages.
    ///
    /// There is no notification whether message was transmitted successfully.
    template<typename MessageT>
    bool sendMessage(MessageT& msg)
    {
        constexpr std::uint8_t rIdx = ImcProtocol::getRecipientNumber(MessageT::myId);
        if constexpr(rIdx == ImcProtocol::controlMessageRecipient)
        {
            return sendControlMessage(msg);
        }
        else
        {
            return sendUserMessage(msg);;
        }
    }

    /// Returns true if communication with other device was established.
    bool hasCommunicationEstablished() const
    {
        return control.hasCommunicationEstablished();
    }

    /// Returns true if module currently have capacity to enqueue message for sending.
    bool canEnqueueMessage()
    {
        return sender.queueCapacity() > 1;
    }

private:
    template<typename MessageT>
    bool sendUserMessage(MessageT& msg)
    {
        if(hasCommunicationEstablished() && sender.queueCapacity() > 1)
        {
            // Always reserve one slot for control messages
            return sendMessageImpl(msg);
        }
        else
        {
            return false;
        }
    }

    template<typename MessageT>
    bool sendControlMessage(MessageT& msg)
    {
        if(sender.queueCapacity() > 0)
        {
            return sendMessageImpl(msg);
        }
        else
        {
            return false;
        }
    }

    template<typename MessageT>
    bool sendMessageImpl(MessageT& msg)
    {
        static_assert(mp::is_instantiation_of<ImcProtocol::MessageBase, MessageT>::value, "MessageT needs to be instantiation of InterMcuProtocol::Message");

        msg.id = MessageT::myId;
        msg.size = MessageT::dataSize;
        msg.sequence = nextSequence++;
        msg.crc = computeCrc(reinterpret_cast<std::uint8_t*>(&msg), 4 + MessageT::dataSize);

        if(sender.sendMessage(msg))
        {
            control.onMessageSent();
            return true;
        }
        else
        {
            return false;
        }
    }

    std::uint32_t computeCrc(std::uint8_t* message, std::uint8_t headerAndContentsSize)
    {
        crc.reset();
        crc.add(makeSpan(message, headerAndContentsSize));
        return crc.get();
    }

    bool handleReceivedMessage()
    {
        auto maybeMessage = receiver.getNextMessage();
        if(maybeMessage.has_value())
        {
            ReceivedMessage& message = *maybeMessage.value();
            handleReceivedMessage(message);
        }
        return maybeMessage.has_value();
    }

    void handleReceivedMessage(ReceivedMessage& message)
    {
        if(checkReceivedMessageIsValid(message))
        {
            if(dispatchMessage(message))
            {
                constexpr std::uint8_t sequenceOffset = 2;
                std::uint16_t sequence = *reinterpret_cast<std::uint16_t*>(message.data() + sequenceOffset);
                lastReceivedSequence = sequence;
                control.onMessageReceived();
            }
            else
            {
                responseWithReceiveError();
            }
        }
        else
        {
            responseWithReceiveError();
        }
    }

    bool checkReceivedMessageIsValid(ReceivedMessage& message)
    {
        constexpr std::uint8_t headerSize = 4;
        constexpr std::uint8_t headerAndCrcSize = 8;

        if(message.size() < headerAndCrcSize)
        {
            return false;
        }

        std::uint8_t dataSize = message[1];
        // As crc is 4-byte aligned, message contents are padded to 4 bytes
        std::uint8_t dataSizeWithPadding = dataSize > 4 ? dataSize + 3 - ((dataSize + 3) % 4) : 4;

        if(dataSizeWithPadding != message.size() - headerAndCrcSize)
        {
            return false;
        }

        std::uint8_t crcOffset = message.size() - headerSize;
        std::uint32_t crc = *reinterpret_cast<std::uint32_t*>(message.data() + crcOffset);

        if(crc != computeCrc(message.data(), headerSize + dataSize))
        {
            return false;
        }

        return true;
    }

    bool dispatchMessage(ReceivedMessage& message)
    {
        std::uint8_t id = message[0];
        std::uint8_t dataSize = message[1];
        std::uint8_t* data = message.data();

        std::uint8_t rIdx = ImcProtocol::getRecipientNumber(id);
        if(rIdx == ImcProtocol::controlMessageRecipient)
        {
            return control.dispatch(*this, id, dataSize, data);
        }
        else
        {
            auto& recipient = recipients[rIdx-1];
            return recipient(*this, id, dataSize, data);
        }
        return false;
    }

    void responseWithReceiveError()
    {
        ImcProtocol::ReceiveError response {};
        response.data.lastOkSequence = lastReceivedSequence;
        sendControlMessage(response);
    }

    Uart& uart;
    Crc& crc;
    ImcReceiver<Uart, maxMessageSize> receiver;
    ImcSender<Uart, maxMessageSize> sender;
    ImcControl control;

    std::array<MessageRecipient, 3> recipients {};

    ImcSettings& settings;
    std::uint16_t nextSequence = 0;
    std::uint16_t lastReceivedSequence = 0;
};

}
