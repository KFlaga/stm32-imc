#include <stm32/peripheral/Crc.hpp>
#include <stm32/peripheral/Gpio.hpp>
#include <stm32/peripheral/InterruptTimer.hpp>
#include <stm32/peripheral/Uart.hpp>
#include <stm32/peripheral/UsTimer.hpp>
#include <peripheral/Button.hpp>
#include <imc/InterMcuCommunicationModule.hpp>

// Example program to test communication between two microcontrollers
//   using UART and InterMcuCommunicationModule.
//
// Both MCUs will send a message to each other every 1ms.
// Message contains desired state of other device's Led1 and current state of its button.
// Each MCU have 1 button and 3 leds.
// Led1 will be light up according to state received in message,
//   that will change every 1s or 2s, depending on device's role.
// Led2 will be light up when button is pressed.
// Led3 will be light up when button of other device is pressed.

namespace DynaSoft
{

#ifdef BUILD_SLAVE_DEVICE
constexpr bool isImcMaster = false;
#else
constexpr bool isImcMaster = true;
#endif

constexpr std::uint8_t mainRecipent = 1;

struct MasterMessageData
{
    bool buttonState = false;
    bool led1State = false;
    std::uint16_t dummy = 0;
};
using MasterMessage = ImcProtocol::Message<MasterMessageData, ImcProtocol::makeMessageId(mainRecipent, 1)>;

struct SlaveMessageData
{
    bool led1State = false;
    std::uint16_t dummy = 0;
    bool buttonState = false;
};
using SlaveMessage = ImcProtocol::Message<SlaveMessageData, ImcProtocol::makeMessageId(mainRecipent, 2)>;

using MessageToSend = std::conditional_t<isImcMaster, MasterMessage, SlaveMessage>;
using MessageToReceive = std::conditional_t<isImcMaster, SlaveMessage, MasterMessage>;

constexpr std::uint8_t messageMaxSize = std::max({
    sizeof(MasterMessage),
    sizeof(SlaveMessage),
	ImcProtocol::controlMessageMaxSize
});

using Imc = InterMcuCommunicationModule<StmUart, StmCrc, messageMaxSize, isImcMaster>;

class MainRecipient : public ImcRecipent<MainRecipient, mainRecipent, MessageToReceive>
{
public:
    template<typename ImcModule>
    bool handleMessage(MessageToReceive& m, ImcModule&)
    {
        // Callback handler for received messages
        // Will light up leds according to received message contents.
        gpio.port(led1Pin.port).set(led1Pin.pin, m.data.led1State);
        gpio.port(led3Pin.port).set(led3Pin.pin, m.data.buttonState);

        return true;
    }

    MainRecipient(StmGpio& gpio_, PortPin led1Pin_, PortPin led3Pin_) :
        gpio{gpio_},
        led1Pin{led1Pin_},
        led3Pin{led3Pin_}
    {
    }

    StmGpio& gpio;
    PortPin led1Pin;
    PortPin led3Pin;
};

}

void dyna_assert_impl(bool, const char*, const char*, int)
{
    // Here goes assertion code. May save some diagnostics and rise a fault, but its out of scope of this example.
}

int main(void)
{
    using namespace DynaSoft;

    // First initialize required peripherals and other stuff
    StmGpio gpio{};
    StmInterruptTimer irqTimer{};
    StmUsTimer usTimer{};
    StmCrc crc{};

    UartSettings uartSettings =
    {
        UartChannel::Uart1, // uart channel
        115200, // baud rate, its max rate for build w/o optimizations
        200, // check for idle us
        300 // generate idle us
    };
    std::array<std::uint8_t, messageMaxSize> uartSendBuffer;
    StmUart uart{gpio, irqTimer, uartSettings, makeSpan(uartSendBuffer.data(), uartSendBuffer.size())};

    InputPin buttonPin{ports::B, 5, InputType::DigitalNormalLow};

    PortPin led1Pin{ports::A, 0}; // Led1 will be set to state send by other device
    PortPin led2Pin{ports::A, 1}; // Led2 will light up when button of this device is pressed
    PortPin led3Pin{ports::A, 4}; // Led2 will light up when button of other device is pressed

    Button button{buttonPin};
    gpio.port(led1Pin.port).makeOutput(led1Pin.pin);
    gpio.port(led2Pin.port).makeOutput(led2Pin.pin);
    gpio.port(led3Pin.port).makeOutput(led3Pin.pin);

    ImcSettings imcSettings
    {
        1000000, // slaveHandshakeIntervalUs
        1000000, // slaveKeepAliveIntervalUs
        5000000, // slaveAckTimeoutUs
        5000000 // masterCommunicationTimeoutUs
    };
    Imc imc{uart, crc, imcSettings};

    auto receiver = MainRecipient{gpio, led1Pin, led3Pin};
    receiver.registerRecipient(imc);

    usTimer.turnOn();
    uart.turnOn();

    constexpr std::uint32_t led1IntervalUs = 1000 * 1000;
    std::uint32_t led1Timer = 0;
    bool led1State = false;

    while (true)
    {
        std::uint32_t loopUs = usTimer.readUs(0);
        if(loopUs >= 1000) // 1ms control loop
        {
            usTimer.reset(0);

            button.update(gpio, loopUs);
            gpio.port(led2Pin.port).set(led2Pin.pin, button.isPressed());

            led1Timer += loopUs;
            if(led1Timer >= led1IntervalUs)
            {
                led1State = !led1State;
                led1Timer = 0;
            }

            imc.update(loopUs);

            if(imc.hasCommunicationEstablished() && imc.canEnqueueMessage())
            {
                MessageToSend m{};
                m.data.led1State = led1State;
                m.data.buttonState = button.isPressed();
                imc.sendMessage(m);
            }
        }
    }
}
