#pragma once

#include <imc/ImcProtocol.hpp>
#include <misc/Meta.hpp>

namespace DynaSoft
{

/// Base class for message recipients responsible for handling received messages
///
/// Derived class should define function handleMessage - one overload for each type in Messages,
/// with signature:
/// \code bool handleMessage(MessageContents&, ImcModule&) \endcode
/// , where MessageContents is Message::Data.
/// It will be called when message with corresponding id is received.
/// handleMessage should return true if received message is valid.
///
/// \tparam Derived Actual recipient implementation.
/// \tparam recipentNumber_ Unique number of this recipient.
/// \tparam Messages List of all Message types that are expected by this recipient.
template<typename Derived, std::uint8_t recipentNumber_, typename ... Messages>
class ImcRecipent
{
public:
    static constexpr auto maxMessageSize = std::max({ sizeof(Messages)... });
    static constexpr std::uint8_t recipentNumber = recipentNumber_;

    /// Registers itself in ImcModule
    template<typename ImcModule>
    void registerRecipient(ImcModule& imc)
    {
        imc.registerMessageRecipient(
            recipentNumber,
            typename ImcModule::MessageRecipient
            {
                [](CallbackContext ctx, ImcModule& imc, std::uint8_t id, std::uint8_t dataSize, std::uint8_t* data)
                {
                    return reinterpret_cast<ImcRecipent*>(ctx)->dispatch(imc, id, dataSize, data);
                },
                this
            }
        );
    }

    /// Calls handleMessage(MessageContents&, ImcModule&) with MessageContents type that matches received id
    /// If there's no such id in any of Messages... or derived class doesn't implement handler returns false.
    template<typename ImcModule>
    bool dispatch(
        ImcModule& imc,
        std::uint8_t id,
        std::uint8_t dataSize,
        std::uint8_t* data)
    {
        return dispatchImpl(imc, id, dataSize, data, std::integral_constant<int, 0>{});
    }

private:
    template<typename ImcModule, int i>
    bool dispatchImpl(
        ImcModule& imc,
        [[maybe_unused]] std::uint8_t id,
        [[maybe_unused]] std::uint8_t dataSize,
        [[maybe_unused]] std::uint8_t* data,
        std::integral_constant<int, i>)
    {
        if constexpr(i < sizeof...(Messages))
        {
            using Message = mp::type_at<i, Messages...>;
            if(id == Message::myId)
            {
                return dispatchKnownMessage<Message>(imc, dataSize, data);
            }
            else
            {
                return dispatchImpl(imc, id, dataSize, data, std::integral_constant<int, i+1>{});
            }
        }
        else
        {
            return false;
        }
    }

    template<typename Message, typename ImcModule>
    bool dispatchKnownMessage(
        ImcModule& imc,
        std::uint8_t dataSize,
        std::uint8_t* data)
    {
        using MessageContents = typename Message::Data;
        if(dataSize == sizeof(MessageContents))
        {
            MessageContents& contents = ImcProtocol::decode<MessageContents>(data);
            return static_cast<Derived*>(this)->handleMessage(contents, imc);
        }
        else
        {
            return false;
        }
    }
};

}
