#include <tests/framework.hpp>
#include <imc/InterMcuCommunicationModule.hpp>
#include <peripheral/UartBase.hpp>
#include <vector>
#include <cstring>
#include <iomanip>

using namespace DynaSoft;

struct TestUartSettings
{
};

constexpr std::uint8_t sendBufferSize = 32;

template<typename Msg>
std::vector<std::uint8_t> payload(Msg msg)
{
    std::uint8_t* p = reinterpret_cast<std::uint8_t*>(&msg);
    return std::vector<std::uint8_t>(p, p + sizeof(Msg));
}

template<typename Msg, typename Buffer>
Msg fromBuffer(const Buffer& b)
{
    ASSERT_EQUAL(sizeof(Msg), b.size());

    Msg msg{};
    std::uint8_t* p = reinterpret_cast<std::uint8_t*>(&msg);
    std::copy(b.begin(), b.end(), p);
    return msg;
}

struct TestInterruptTimer : public InterruptTimerBase<TestInterruptTimer, 4>
{
    struct Event
    {
        Callback cb;
        std::uint32_t us;
    };

    bool scheduleInterrupt(std::uint8_t channel, std::uint32_t us, Callback cb)
    {
        ASSERT_TRUE(channel < 4);
        events[channel] = {cb, us};
        return true;
    }

    void invoke(std::uint8_t channel)
    {
        events[channel].cb(events[channel].us);
    }

    std::array<Event, 4> events{};
};

struct TestUart : public UartBase<TestUart, TestInterruptTimer, StaticVector<std::uint8_t, sendBufferSize>>
{
    TestUart(TestInterruptTimer& t) : UartBase(t, 100, 100) {}

    void _suspendSend()
    {
    }

    void _resumeSend()
    {
    }

    void _suspendReceive()
    {
    }

    void _resumeReceive()
    {
    }

    void _sendByte(std::uint8_t data)
    {
        sentBytes.push_back(data);
    }

    std::uint8_t _receiveByte()
    {
        return nextByte;
    }

    void callTransmissionComplete()
    {
        handleTransmissionComplete();
    }

    void sendAllQueuedBytes()
    {
        while(isTransmitOngoing())
        {
            callTransmissionComplete();
        }
    }

    void callDataReceived()
    {
        handleDataReceived();
    }

    void callDataReceived(std::vector<std::uint8_t> xs)
    {
        for(auto x: xs)
        {
            nextByte = x;
            callDataReceived();
        }
    }

    void callReceiveError(std::uint8_t error)
    {
        onReceiveError(error);
    }

    void callIdleLineDetected()
    {
        onIdleLineDetected();
    }

    void generateIdleLine() // shadows function from UartBase
    {
        idleLines++;
    }

    std::uint8_t nextByte = 0;
    std::uint8_t idleLines = 0;

    std::vector<std::uint8_t> sentBytes{};
};

struct TestCrc : public CrcBase<TestCrc>
{
    using CrcBase::CrcBase;

    void _add(std::uint32_t x)
    {
        crc += x;
    }

    std::uint32_t _get()
    {
        return crc;
    }

    void _reset()
    {
        crc = 0;
    }

    template<typename Message>
    std::uint32_t getCrc(Message msg)
    {
        auto buf = payload(msg);
        crc = 0;
        int headerAndContentsSize = 4 + msg.size;
        for(int i = 0; i < headerAndContentsSize; ++i)
        {
            add(buf[i]);
        }
        return crc;
    }

    std::uint32_t crc;
};

constexpr std::uint8_t maxMessageSize = 32;

using TestMasterIMC = InterMcuCommunicationModule<TestUart, TestCrc, maxMessageSize, true>;
using TestSlaveIMC = InterMcuCommunicationModule<TestUart, TestCrc, maxMessageSize, false>;

using TestImcReceiver = ImcReceiver<TestUart, maxMessageSize>;
using TestImcSender = ImcSender<TestUart, maxMessageSize>;

constexpr std::uint8_t testRecipent = 2;

struct TestMessageContents
{
    std::uint8_t a;
    std::uint32_t b;
};
using TestMessage = ImcProtocol::Message<TestMessageContents, ImcProtocol::makeMessageId(testRecipent, 1)>;

namespace detail
{
template<bool onlyCheckId = false, typename Message, typename Container>
std::uint8_t expectMessage(Container& uartMsg, const std::string& fileLine, const Message& expectedMsg, std::uint8_t index)
{
    if constexpr(onlyCheckId)
    {
        EXPECT_EQUAL_EXT(expectedMsg.id, uartMsg[index], fileLine);
    }
    else
    {
        auto msgBytes = payload(expectedMsg);
        int headerAndContentsSize = 4 + expectedMsg.size;
        int crcOffset = sizeof(Message) - 4;

        bool areContentsEqual = std::equal(msgBytes.begin(), msgBytes.begin() + headerAndContentsSize, uartMsg.begin() + index);
        bool areCrcEqual = std::equal(msgBytes.begin() + crcOffset, msgBytes.end(), uartMsg.begin() + index + crcOffset);

        if(!areContentsEqual || !areCrcEqual)
        {
            std::cout << "EXPECTED: ";
            for(int x: payload(expectedMsg))
            {
                std::cout << std::setw(4) << x;
            }
            std::cout << "\nACTUAL:   ";
            for(int x: uartMsg)
            {
                std::cout << std::setw(4) << x;
            }
            std::cout << "\n";
        }
        EXPECT_EQUAL_EXT(true, areContentsEqual, fileLine);
        EXPECT_EQUAL_EXT(true, areCrcEqual, fileLine);
    }
    return sizeof(Message);
}
}

template<bool onlyCheckId = false, typename... Messages>
void expectSentMessages(TestUart& uart, const std::string& fileLine, const Messages&... msg)
{
    auto msgSize = (sizeof(msg) + ...);
    ASSERT_EQUAL_EXT(msgSize, uart.sentBytes.size(), fileLine);
    std::uint8_t index = 0;
    ((index = detail::expectMessage<onlyCheckId>(uart.sentBytes, fileLine, msg, index)), ...);
    uart.sentBytes.clear();
}

template<typename Msg>
void expectReceivedMessage(TestImcReceiver& receiver, const std::string& fileLine)
{
    auto maybeMsg = receiver.getNextMessage();
    ASSERT_EQUAL_EXT(true, maybeMsg.has_value(), fileLine);
    auto& container = *maybeMsg.value();
    ASSERT_EQUAL_EXT(sizeof(Msg), container.size(), fileLine);
    auto msg = fromBuffer<Msg>(container);
    EXPECT_EQUAL_EXT(Msg::myId, msg.id, fileLine);
}

#define EXPECT_SENT_MESSAGES(uart, ...) expectSentMessages(uart, FILE_LINE(), __VA_ARGS__)
#define EXPECT_SENT_MESSAGES_ID(uart, ...) expectSentMessages<true>(uart, FILE_LINE(), __VA_ARGS__)

#define EXPECT_RECEIVED_MESSAGE(receiver, MessageT) expectReceivedMessage<MessageT>(receiver, FILE_LINE())

template<typename Message>
Message makeMessage(std::uint16_t sequence)
{
    Message msg{};
    msg.sequence = sequence;
    msg.crc = TestCrc{}.getCrc(msg);
    return msg;
}

template<typename Message, typename MessageContents>
Message makeMessage(std::uint16_t sequence, MessageContents contents)
{
    Message msg{};
    msg.sequence = sequence;
    msg.data = contents;
    msg.crc = TestCrc{}.getCrc(msg);
    return msg;
}

class ImcReceiverTest : public ::test::Test
{
public:
    ImcReceiverTest() :
        uart{timer},
        receiver{uart}
    {

    }

    TestInterruptTimer timer;
    TestUart uart;
    TestImcReceiver receiver;
};

ADD_TEST_F(ImcReceiverTest, resetState)
{
    // Starts in reset state
    // Should have no new message
    // Then receives some bytes, still should have no message
    // Then gets idle, but as it was first idle - drops partial message

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_FALSE(receiver.hasError());

    uart.callDataReceived(payload(TestMessage{}));

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_FALSE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_FALSE(receiver.hasError());
}

ADD_TEST_F(ImcReceiverTest, receiveSingleMessage)
{
    // Starts in reset state
    // Gets idle and moves to proper receive state
    // Receives some bytes, but has no new message until gets idle

    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_FALSE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.hasError());
    EXPECT_RECEIVED_MESSAGE(receiver, TestMessage);

    // Message may be taken only once
    EXPECT_FALSE(receiver.getNextMessage().has_value());
}

ADD_TEST_F(ImcReceiverTest, receiveNextMessages)
{
    // Every message may be requested once
    // Then we can receive another one, which will be available after next idle line

    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));
    uart.callIdleLineDetected();

    auto maybeMsg = receiver.getNextMessage();
    EXPECT_TRUE(maybeMsg.has_value());

    uart.callDataReceived(payload(TestMessage{}));
    uart.callIdleLineDetected();

    EXPECT_RECEIVED_MESSAGE(receiver, TestMessage);

    EXPECT_FALSE(receiver.getNextMessage().has_value());

    uart.callDataReceived(payload(ImcProtocol::Handshake{}));

    EXPECT_FALSE(receiver.getNextMessage().has_value());

    uart.callIdleLineDetected();

    EXPECT_RECEIVED_MESSAGE(receiver, ImcProtocol::Handshake);

    uart.callDataReceived(payload(TestMessage{}));

    EXPECT_FALSE(receiver.getNextMessage().has_value());

    uart.callIdleLineDetected();

    EXPECT_RECEIVED_MESSAGE(receiver, TestMessage);
}

ADD_TEST_F(ImcReceiverTest, errorDuringReceive)
{
    // On receive error message is unavailable

    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));
    uart.callReceiveError(0);

    EXPECT_TRUE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_TRUE(receiver.hasError());
}

ADD_TEST_F(ImcReceiverTest, whenNewMessagesNotProcessedInTime_risesErrorAndIgnoresNextMessages)
{
    // Receiver may hold up to 2 messages for reading
    // When data is received while holding 2 unprocessed messages an error is generated
    // But both unprocessed messages are still available

    uart.callIdleLineDetected();
    uart.callDataReceived(payload(ImcProtocol::Handshake{}));
    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));

    EXPECT_FALSE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.hasError());

    uart.callDataReceived(payload(ImcProtocol::Handshake{}));

    EXPECT_TRUE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_TRUE(receiver.hasError());

    EXPECT_RECEIVED_MESSAGE(receiver, ImcProtocol::Handshake);
    EXPECT_RECEIVED_MESSAGE(receiver, TestMessage);
    EXPECT_FALSE(receiver.getNextMessage().has_value());
}

ADD_TEST_F(ImcReceiverTest, whenBufferOverrun_risesErrorAndDropsMessages)
{
    // We received message longer than expected max, it is treated as error

    uart.callIdleLineDetected();
    uart.callDataReceived(std::vector<std::uint8_t>(100, 1));

    EXPECT_TRUE(receiver.hasError());

    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.getNextMessage().has_value());
    EXPECT_TRUE(receiver.hasError());
}

ADD_TEST_F(ImcReceiverTest, whenErrorIsRisenAndCleared_receivesNextMessagesNormally)
{
    // On receive error message is unavailable

    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));
    uart.callReceiveError(0);

    EXPECT_TRUE(receiver.hasError());

    uart.callIdleLineDetected();
    receiver.clearError();

    uart.callDataReceived(payload(ImcProtocol::Acknowledge{}));
    uart.callIdleLineDetected();

    EXPECT_RECEIVED_MESSAGE(receiver, ImcProtocol::Acknowledge);
}

ADD_TEST_F(ImcReceiverTest, manyIdlesWithNoDataAreOk)
{
    // On receive error message is unavailable

    uart.callIdleLineDetected();
    uart.callIdleLineDetected();
    uart.callIdleLineDetected();
    uart.callDataReceived(payload(TestMessage{}));
    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.hasError());
    EXPECT_RECEIVED_MESSAGE(receiver, TestMessage);

    uart.callIdleLineDetected();
    uart.callIdleLineDetected();

    EXPECT_FALSE(receiver.hasError());
}

class ImcSenderTest : public ::test::Test
{
public:
    ImcSenderTest() :
        timer{},
        uart{timer},
        sender{uart}
    {

    }

    TestInterruptTimer timer;
    TestUart uart;
    TestImcSender sender;
};

ADD_TEST_F(ImcSenderTest, sendsMessageAndFollowsWithIdle)
{
    EXPECT_EQUAL(2u, sender.queueCapacity());

    TestMessage msg{};
    bool success = sender.sendMessage(msg);

    EXPECT_TRUE(success);
    EXPECT_EQUAL(1u, sender.queueCapacity());

    uart.sendAllQueuedBytes();

    EXPECT_EQUAL(1, uart.idleLines);
    EXPECT_SENT_MESSAGES(uart, msg);
    EXPECT_EQUAL(2u, sender.queueCapacity());
}

ADD_TEST_F(ImcSenderTest, queuesSecondMessage)
{
    TestMessage msg{};
    bool success = sender.sendMessage(msg);
    EXPECT_TRUE(success);
    EXPECT_EQUAL(1u, sender.queueCapacity());

    ImcProtocol::Handshake msg2{};

    success = sender.sendMessage(msg2);
    EXPECT_TRUE(success);
    EXPECT_EQUAL(0u, sender.queueCapacity());

    uart.sendAllQueuedBytes();

    EXPECT_EQUAL(2, uart.idleLines);
    EXPECT_SENT_MESSAGES(uart, msg, msg2);
    EXPECT_EQUAL(2u, sender.queueCapacity());
}

ADD_TEST_F(ImcSenderTest, doesntQueueThirdMessage)
{
    TestMessage msg{};
    sender.sendMessage(msg);
    sender.sendMessage(msg);
    bool success = sender.sendMessage(msg);

    EXPECT_FALSE(success);

    uart.sendAllQueuedBytes();

    EXPECT_EQUAL(2, uart.idleLines);
    EXPECT_SENT_MESSAGES(uart, msg, msg);
}

class ImcSlaveTest : public ::test::Test
{
public:
    ImcSlaveTest() :
        timer{},
        settings{},
        uart{timer},
        imc{uart, crc, settings}
    {
        settings.slaveHandshakeIntervalUs = 1000;
        settings.slaveKeepAliveIntervalUs = 1000;
        settings.slaveAckTimeoutUs = 3000;

        uart.callIdleLineDetected();
    }

    void sendAck(std::uint16_t sequence, std::uint8_t ackedId, std::uint16_t ackedSequence)
    {
        ImcProtocol::Acknowledge ack{};
        ack.sequence = sequence;
        ack.data.ackId = ackedId;
        ack.data.ackSequence = ackedSequence;
        ack.crc = TestCrc{}.getCrc(ack);
        uart.callDataReceived(payload(ack));
        uart.callIdleLineDetected();
    }

    void establishCommunication()
    {
        imc.update(1);
        uart.sendAllQueuedBytes();
        EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));
        sendAck(getNextReceivedSequence(), ImcProtocol::Handshake::myId, 22);
        imc.update(1);
        uart.sendAllQueuedBytes();
        EXPECT_EQUAL(0u, uart.sentBytes.size());
    }

    std::uint16_t getNextSentSequence()
    {
        return nextSentSequence++;
    }

    std::uint16_t getNextReceivedSequence()
    {
        return nextReceivedSequence++;
    }

    void checkImcProcessesData()
    {
        bool dispatchedFlag = false;
        imc.registerMessageRecipient(
            testRecipent, {
            [](void* ctx, auto&, std::uint8_t, std::uint8_t, std::uint8_t*)
            {
                *reinterpret_cast<bool*>(ctx) = true;
                return true;
            },
            &dispatchedFlag
        });

        TestMessage msg = makeMessage<TestMessage>(getNextReceivedSequence(), TestMessageContents{1, 2});
        uart.callDataReceived(payload(msg));
        uart.callIdleLineDetected();

        imc.update(1);
        uart.sendAllQueuedBytes();

        EXPECT_EQUAL(0u, uart.sentBytes.size());
        EXPECT_TRUE(dispatchedFlag);
    }

    TestCrc crc;
    TestInterruptTimer timer;
    ImcSettings settings;
    TestUart uart;
    TestSlaveIMC imc;

    std::uint16_t nextSentSequence = 0;
    std::uint16_t nextReceivedSequence = 0;
};

class ImcMasterTest : public ::test::Test
{
public:
    ImcMasterTest() :
        timer{},
        settings{},
        uart{timer},
        imc{uart, crc, settings}
    {
        settings.slaveHandshakeIntervalUs = 1000;
        settings.slaveKeepAliveIntervalUs = 1000;
        settings.slaveAckTimeoutUs = 3000;

        settings.masterCommunicationTimeoutUs = 3000;

        uart.callIdleLineDetected();
    }

    void establishCommunication()
    {
        std::uint16_t s = getNextReceivedSequence();
        ImcProtocol::Handshake handshake = makeMessage<ImcProtocol::Handshake>(s);
        uart.callDataReceived(payload(handshake));
        uart.callIdleLineDetected();

        imc.update(1);
        uart.sendAllQueuedBytes();
        EXPECT_TRUE(imc.hasCommunicationEstablished());
        EXPECT_SENT_MESSAGES(uart,
            makeMessage<ImcProtocol::Acknowledge>(getNextSentSequence(), ImcProtocol::AckMessageContents{ImcProtocol::Handshake::myId, s})
        );
    }

    std::uint16_t getNextSentSequence()
    {
        return nextSentSequence++;
    }

    std::uint16_t getNextReceivedSequence()
    {
        return nextReceivedSequence++;
    }

    TestCrc crc;
    TestInterruptTimer timer;
    ImcSettings settings;
    TestUart uart;
    TestMasterIMC imc;

    std::uint16_t nextSentSequence = 0;
    std::uint16_t nextReceivedSequence = 0;
};

// ImcModuleTest tests part of InterMcuCommunicationModule common for slave and master, uses slave control
class ImcModuleTest : public ImcSlaveTest
{
};

ADD_TEST_F(ImcSlaveTest, onResetState_sendsHandshakesAfterInterval)
{
    // First handshake is sent immediately
    imc.update(1);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));
    EXPECT_EQUAL(1, uart.idleLines);
    EXPECT_FALSE(imc.hasCommunicationEstablished());

    // Another one after programmed interval (1000)
    imc.update(999);
    uart.sendAllQueuedBytes();

    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(1);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));
    EXPECT_EQUAL(2, uart.idleLines);
}

ADD_TEST_F(ImcSlaveTest, whenHandshakeAcked_sendsKeepAlive)
{
    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));

    sendAck(getNextReceivedSequence(), ImcProtocol::Handshake::myId, 0);

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::KeepAlive>(getNextSentSequence()));

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::KeepAlive>(getNextSentSequence()));
}

ADD_TEST_F(ImcSlaveTest, whenHandshakeAckedWithWrongId_ignoresIt_acceptsWrongSquenceId)
{
    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));

    sendAck(getNextReceivedSequence(), 0x54, 0);

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));
    EXPECT_FALSE(imc.hasCommunicationEstablished());

    sendAck(getNextReceivedSequence(), ImcProtocol::Handshake::myId, 22);

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::KeepAlive>(getNextSentSequence()));
    EXPECT_TRUE(imc.hasCommunicationEstablished());
}

ADD_TEST_F(ImcSlaveTest, whenKeepAliveNotAckedAfterTimeout_switchesToResetState)
{
    establishCommunication();

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::KeepAlive>(getNextSentSequence()));

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::KeepAlive>(getNextSentSequence()));
    EXPECT_TRUE(imc.hasCommunicationEstablished());

    imc.update(1000);
    uart.sendAllQueuedBytes();

    EXPECT_SENT_MESSAGES(uart, makeMessage<ImcProtocol::Handshake>(getNextSentSequence()));
    EXPECT_FALSE(imc.hasCommunicationEstablished());
}

ADD_TEST_F(ImcSlaveTest, whenUserDataIsSent_delaysKeepAlive)
{
    establishCommunication();

    imc.update(600);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    TestMessage msg = makeMessage<TestMessage>(getNextSentSequence(), TestMessageContents{1, 2});
    imc.sendMessage(msg);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart, msg);

    imc.update(600);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());
}

ADD_TEST_F(ImcMasterTest, onResetState_doesNothing)
{
    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_FALSE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(5000);
    uart.sendAllQueuedBytes();
    EXPECT_FALSE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());
}

ADD_TEST_F(ImcMasterTest, whenReceivesHandshake_sendsAck_establishesCommunication)
{
    ImcProtocol::Handshake handshake = makeMessage<ImcProtocol::Handshake>(0);
    uart.callDataReceived(payload(handshake));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::Acknowledge>(getNextSentSequence(), ImcProtocol::AckMessageContents{ImcProtocol::Handshake::myId, 0})
    );
}

ADD_TEST_F(ImcMasterTest, whenReceivesKeepAlive_sendsAck)
{
    establishCommunication();

    ImcProtocol::KeepAlive keepAlive = makeMessage<ImcProtocol::KeepAlive>(1);
    uart.callDataReceived(payload(keepAlive));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::Acknowledge>(getNextSentSequence(), ImcProtocol::AckMessageContents{ImcProtocol::KeepAlive::myId, 1})
    );
}

ADD_TEST_F(ImcMasterTest, onResetState_whenReceivesKeepAlive_doesNothing)
{
    ImcProtocol::KeepAlive keepAlive = makeMessage<ImcProtocol::KeepAlive>(1);
    uart.callDataReceived(payload(keepAlive));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_FALSE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());
}

ADD_TEST_F(ImcMasterTest, whenNoMessageIsReceivedForLongTime_tearsDownConnection)
{
    // No message
    establishCommunication();

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(1000);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());
    EXPECT_FALSE(imc.hasCommunicationEstablished());

    // KeepAlive refreshes timer
    establishCommunication();

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    ImcProtocol::KeepAlive keepAlive = makeMessage<ImcProtocol::KeepAlive>(getNextReceivedSequence());
    uart.callDataReceived(payload(keepAlive));
    uart.callIdleLineDetected();

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::Acknowledge>(getNextSentSequence(), ImcProtocol::AckMessageContents{ImcProtocol::KeepAlive::myId, keepAlive.sequence})
    );

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(1000);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());
    EXPECT_FALSE(imc.hasCommunicationEstablished());

    // User message refreshes timer
    establishCommunication();

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.registerMessageRecipient(testRecipent, {[](void*, auto&, std::uint8_t, std::uint8_t, std::uint8_t*)
    {
        return true;
    }, nullptr});

    TestMessage msg = makeMessage<TestMessage>(getNextReceivedSequence());
    uart.callDataReceived(payload(msg));
    uart.callIdleLineDetected();

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(2000);
    uart.sendAllQueuedBytes();
    EXPECT_TRUE(imc.hasCommunicationEstablished());
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    imc.update(1000);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());
    EXPECT_FALSE(imc.hasCommunicationEstablished());

}

ADD_TEST_F(ImcModuleTest, whenUserDataIsReceived_dispatchesToRecipient)
{
    establishCommunication();

    bool dispatchedFlag = false;

    imc.registerMessageRecipient(
        testRecipent, {
        [](void* ctx, auto&, std::uint8_t id, std::uint8_t size, std::uint8_t* data)
        {
            ASSERT_EQUAL(TestMessage::myId, id);
            ASSERT_EQUAL(TestMessage::dataSize, size);

            TestMessageContents& message = reinterpret_cast<TestMessage*>(data)->data;
            EXPECT_EQUAL(1u, message.a);
            EXPECT_EQUAL(2u, message.b);
            *reinterpret_cast<bool*>(ctx) = true;

            return true;
        },
        &dispatchedFlag
    });

    TestMessage msg = makeMessage<TestMessage>(getNextReceivedSequence(), TestMessageContents{1, 2});
    uart.callDataReceived(payload(msg));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    EXPECT_TRUE(dispatchedFlag);
}

ADD_TEST_F(ImcModuleTest, whenNoRecipentIsRegistred_sendsRecieveError)
{
    establishCommunication();

    TestMessage msg = makeMessage<TestMessage>(getNextReceivedSequence(), TestMessageContents{1, 2});
    uart.callDataReceived(payload(msg));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{0})
    );

    // After error is sent further data should be processed normally
    checkImcProcessesData();
}

ADD_TEST_F(ImcModuleTest, whenRecipentReturnsFalse_sendsRecieveError)
{
    establishCommunication();

    bool dispatchedFlag = false;

    imc.registerMessageRecipient(testRecipent, {[](void* ctx, auto&, std::uint8_t, std::uint8_t, std::uint8_t*)
    {
        *reinterpret_cast<bool*>(ctx) = true;
        return false;
    }, &dispatchedFlag});

    TestMessage msg = makeMessage<TestMessage>(getNextReceivedSequence(), TestMessageContents{1, 2});
    uart.callDataReceived(payload(msg));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{0})
    );

    EXPECT_TRUE(dispatchedFlag);

    checkImcProcessesData();
}

ADD_TEST_F(ImcModuleTest, whenGotReceiveError_sendsReceiveError)
{
    establishCommunication();

    uart.callReceiveError(1);

    imc.update(1000);
    uart.sendAllQueuedBytes();

    // KeepAlive is not sent, as its timer is reset after any sent message
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{0})
    );
    EXPECT_TRUE(imc.hasCommunicationEstablished());

    imc.update(1);
    uart.sendAllQueuedBytes();

    checkImcProcessesData();
}

ADD_TEST_F(ImcModuleTest, whenExpectedAndReceivedMessageFieldsDoesntMatch_sendsReceiveError)
{
    establishCommunication();

    std::uint8_t lastOkSequence = 0;

    ImcProtocol::Acknowledge ack{};
    ack.data.ackId = ImcProtocol::Handshake::myId;
    ack.data.ackSequence = 0;

    // 0) data smaller than 8bytes
    uart.callDataReceived({1, 2, 3, 4, 5, 6, 7});
    uart.callIdleLineDetected();
    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{lastOkSequence})
    );

    // 1) data size too small compared to received bytes
    ack.size = ImcProtocol::Acknowledge::dataSize - 1;
    ack.sequence = getNextReceivedSequence();
    ack.crc = TestCrc{}.getCrc(ack);
    uart.callDataReceived(payload(ack));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{lastOkSequence})
    );

    // 1.1) send proper message
    ack.size = ImcProtocol::Acknowledge::dataSize;
    ack.sequence = getNextReceivedSequence();
    ack.crc = TestCrc{}.getCrc(ack);
    uart.callDataReceived(payload(ack));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    lastOkSequence = ack.sequence;

    // 2) data size too big compared to received bytes
    ack.size = ImcProtocol::Acknowledge::dataSize + 1;
    ack.sequence = getNextReceivedSequence();
    ack.crc = TestCrc{}.getCrc(ack);
    uart.callDataReceived(payload(ack));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{lastOkSequence})
    );

    // 3) crc doesnt match
    ack.size = ImcProtocol::Acknowledge::dataSize;
    ack.sequence = getNextReceivedSequence();
    ack.crc = TestCrc{}.getCrc(ack) - 1;
    uart.callDataReceived(payload(ack));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart,
        makeMessage<ImcProtocol::ReceiveError>(getNextSentSequence(), ImcProtocol::ReceiveErrorContents{lastOkSequence})
    );

    checkImcProcessesData();
}

ADD_TEST_F(ImcModuleTest, whenReceiveErrorReceived_ignoresIt)
{
    establishCommunication();

    std::uint16_t s = getNextSentSequence();
    TestMessage msg = makeMessage<TestMessage>(s, TestMessageContents{1, 2});
    imc.sendMessage(msg);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart, msg);

    ImcProtocol::ReceiveError error = makeMessage<ImcProtocol::ReceiveError>(getNextReceivedSequence(), ImcProtocol::ReceiveErrorContents{0});
    uart.callDataReceived(payload(error));
    uart.callIdleLineDetected();

    imc.update(1);
    uart.sendAllQueuedBytes();
    EXPECT_EQUAL(0u, uart.sentBytes.size());

    msg = makeMessage<TestMessage>(getNextSentSequence(), TestMessageContents{1, 2});
    imc.sendMessage(msg);
    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES(uart, msg);
}

ADD_TEST_F(ImcModuleTest, actualErrorHandlingNeedsImplementation)
{
    // For now errors are just ignored as its not problem to have skipped packets in current application which uses IMC.
    // On error few things may happen:
    // - last message (or lastOkSequence + 1) is re-send if reason is transmission error
    //      (for this we need error type in ReceiveError)
    // - if reason is unsupported message then probably do nothing - however it may indicate that
    //      there is some problem with program on slave or master
    // - some diagnostic info is saved and may be available for inspection from PC application

    bool TODO_implementActualErrorHandling = false;
    EXPECT_TRUE(TODO_implementActualErrorHandling);
}

ADD_TEST_F(ImcModuleTest, whenMessageQueueIsAlmostFull_doesntSendUserMessage_stillSendsControlMessage)
{
    establishCommunication();

    TestMessage msg = makeMessage<TestMessage>(0, TestMessageContents{1, 2});

    // Only one space in user data queue
    EXPECT_TRUE(imc.sendMessage(msg));
    EXPECT_FALSE(imc.sendMessage(msg));

    uart.sendAllQueuedBytes();
    uart.sentBytes.clear();

    EXPECT_TRUE(imc.sendMessage(msg));
    imc.update(1000);

    uart.sendAllQueuedBytes();
    EXPECT_SENT_MESSAGES_ID(uart, msg, makeMessage<ImcProtocol::KeepAlive>(0));
}

ADD_TEST_F(ImcModuleTest, whenMessageQueueIsFull_doesntSendAnyMessages)
{
    establishCommunication();

    imc.update(1000);

    TestMessage msg = makeMessage<TestMessage>(0, TestMessageContents{1, 2});
    EXPECT_FALSE(imc.sendMessage(msg));

    imc.update(1000);
    imc.update(1000);

    uart.sendAllQueuedBytes();

    // Only 2 KAs are queued and sent despite 3 updates() which tries to send them
    EXPECT_SENT_MESSAGES_ID(uart, makeMessage<ImcProtocol::KeepAlive>(0), makeMessage<ImcProtocol::KeepAlive>(0));
}


struct TestMessageContents2
{
    std::uint32_t a = 0;
    std::uint32_t b = 0;
    std::uint32_t c = 0;
    std::uint32_t d = 0;
};
using TestMessage2 = ImcProtocol::Message<TestMessageContents2, ImcProtocol::makeMessageId(testRecipent, 2)>;

struct TestRecipient : public ImcRecipent<TestRecipient, testRecipent, TestMessage, TestMessage2>
{
    bool handleMessage(TestMessage& m, int&)
    {
        m.data.b = 10;
        return true;
    }

    bool handleMessage(TestMessage2& m, int&)
    {
        m.data.c = 20;
        return true;
    }
};

static_assert(TestRecipient::maxMessageSize == 24, "Expected: TestRecipient::maxMessageSize == 24");
static_assert(TestRecipient::recipentNumber == 2, "Expected: TestRecipient::recipentNumber == 2");

ADD_TEST(ImcRecipientTest, dispatchValidMessages)
{
    TestRecipient r{};
    int dummy = 0;
    TestMessage m1{};
    TestMessage2 m2{};

    bool v1 = r.dispatch(dummy, TestMessage::myId, TestMessage::dataSize, reinterpret_cast<std::uint8_t*>(&m1));
    EXPECT_TRUE(v1);
    EXPECT_EQUAL(10u, m1.data.b);
    EXPECT_EQUAL(0u, m2.data.c);

    bool v2 = r.dispatch(dummy, TestMessage2::myId, TestMessage2::dataSize, reinterpret_cast<std::uint8_t*>(&m2));
    EXPECT_TRUE(v2);
    EXPECT_EQUAL(10u, m1.data.b);
    EXPECT_EQUAL(20u, m2.data.c);
}

ADD_TEST(ImcRecipientTest, dispatchInalidMessages)
{
    TestRecipient r{};
    int dummy = 0;
    TestMessage m1{};
    TestMessage2 m2{};

    bool v1 = r.dispatch(dummy, 0, TestMessage::dataSize, reinterpret_cast<std::uint8_t*>(&m1));
    EXPECT_FALSE(v1);
    EXPECT_EQUAL(0u, m1.data.b);
    EXPECT_EQUAL(0u, m2.data.c);

    bool v2 = r.dispatch(dummy, TestMessage2::myId, 143, reinterpret_cast<std::uint8_t*>(&m2));
    EXPECT_FALSE(v2);
    EXPECT_EQUAL(0u, m1.data.b);
    EXPECT_EQUAL(0u, m2.data.c);
}
