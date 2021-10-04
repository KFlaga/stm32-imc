#pragma once

#include <cstdint>

namespace DynaSoft
{

enum class InputType : std::uint8_t
{
    Analog,
    DigitalNormalLow,
    DigitalNormalHigh,
};

namespace OutputType
{
    constexpr std::uint8_t gpio = 0x00;
    constexpr std::uint8_t afio = 0x01;
    constexpr std::uint8_t pushPull = 0x00;
    constexpr std::uint8_t openDrain = 0x02;
    constexpr std::uint8_t normalSpeed = 0x00;
    constexpr std::uint8_t fastSpeed = 0x00;
}

struct PortPin
{
    std::uint8_t port;
    std::uint8_t pin;
};

struct InputPin
{
    std::uint8_t port;
    std::uint8_t pin;
    InputType inputType;
};

}

namespace ports
{
enum : std::uint8_t
{
    A, B, C, D, E
};
}


