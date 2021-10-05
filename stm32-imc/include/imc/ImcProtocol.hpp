#pragma once

#include <cstdint>
#include <type_traits>
#include <algorithm>

namespace DynaSoft
{
namespace ImcProtocol
{

/// Base type for all messages sent between MCUs via UART.
///
/// Contains unique id with 2 MSB being recipient (application module) number.
/// Recipient 0 is for control messages.
/// size indicates size of MessageContents (0 if empty).
/// sequence defines order in which messages where sent.
/// crc is used to validate contents of received message and is computed byte by byte
/// over first 4 + dataSize bytes.
///
/// All those field are set by ImcModule.
///
/// Derived message types should be defined using Message definition.
///
/// Messages are not encoded in any way as both endpoint are supposed to have
/// same (or at least with same architecture and fields layout) MCUs
template<typename MessageContents, typename Id>
struct MessageBase
{
    // 244 - so that type std::uint8_t can hold size of whole message including possible paddings after 'data'
    static_assert(sizeof(MessageContents) <= 244, "sizeof(MessageContents) must be <= 244");

    using Data = MessageContents;

    static constexpr std::uint8_t myId = Id::value;
    static constexpr std::uint8_t dataSize = std::is_empty_v<MessageContents> ? 0 : sizeof(MessageContents);

    std::uint8_t id = myId;
    std::uint8_t size = dataSize;
    std::uint16_t sequence = 0;

    MessageContents data;

    std::uint32_t crc = 0;
};

template<typename MessageContents, std::uint8_t myId>
using Message = MessageBase<MessageContents, std::integral_constant<std::uint8_t, myId>>;

template<typename T>
inline std::uint8_t* encode(T& message)
{
    return reinterpret_cast<std::uint8_t*>(&message);
}

template<typename T>
inline T& decode(std::uint8_t* message)
{
    return *reinterpret_cast<T*>(message);
}

struct EmptyMessageContents
{
};

struct AckMessageContents
{
    std::uint8_t ackId = 0;
    std::uint8_t _ = 0;
    std::uint16_t ackSequence = 0;

    AckMessageContents() = default;

    AckMessageContents(std::uint8_t ackId_, std::uint16_t ackSequence_) :
        ackId{ackId_},
        ackSequence{ackSequence_}
    {}
};

struct ReceiveErrorContents
{
    std::uint16_t lastOkSequence = 0;
    std::uint16_t _ = 0;

    ReceiveErrorContents() = default;

    ReceiveErrorContents(std::uint16_t lastOkSequence_) :
        lastOkSequence{lastOkSequence_}
    {}
};

constexpr std::uint8_t messageRecipientMask = 0xC0;

constexpr std::uint8_t makeRecipientId(std::uint8_t recipientNumber)
{
    return recipientNumber << 6;
}

constexpr std::uint8_t makeMessageId(std::uint8_t recipientNumber, std::uint8_t messageNumber)
{
    return (recipientNumber << 6) | (messageNumber & 0x3F);
}

constexpr std::uint8_t getRecipientNumber(std::uint8_t messageId)
{
    return (messageId & ImcProtocol::messageRecipientMask) >> 6;
}

constexpr std::uint8_t controlMessageRecipient = 0x00;

/// Handshake is used to initiate communication by Slave (sent by Slave only)
using Handshake = Message<EmptyMessageContents, makeMessageId(controlMessageRecipient, 0x01)>;

/// Acknowledge is used to respond to Handshake and KeepAlive messages sent by Slave (sent by Master only)
using Acknowledge = Message<AckMessageContents, makeMessageId(controlMessageRecipient, 0x02)>;

/// ReceiveError is used to respond when received data is corrupted and dropped (sent by both sides)
using ReceiveError = Message<ReceiveErrorContents, makeMessageId(controlMessageRecipient, 0x03)>;

/// KeepAlive is used to keep communication alive by Slave (sent by Slave only)
using KeepAlive = Message<EmptyMessageContents, makeMessageId(controlMessageRecipient, 0x04)>;

constexpr std::uint8_t controlMessageMaxSize = std::max({
    sizeof(Handshake),
    sizeof(Acknowledge),
    sizeof(ReceiveError),
    sizeof(KeepAlive),
});

}
}
