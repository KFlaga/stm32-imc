#pragma once

#include <peripheral/Pins.hpp>
#include <array>

namespace DynaSoft
{

/// Base class for single GPIO port that abstracts hardware.
///
/// \tparam Derived actual implementation of PortBase.
/// \tparam portsCount_ Count of ports on device.
///
/// Class Derived needs to add PortBase as friend.
template<typename Derived, std::size_t pinsCount_>
class PortBase
{
public:
    static constexpr std::size_t pinsCount = pinsCount_;

    enum class PinFunction
    {
        Reset = 0,
        Input = 1,
        Output = 2
    };

    void makeOutput(std::uint8_t pin, std::uint8_t outputType = OutputType::gpio)
    {
        static_cast<Derived*>(this)->_makeOutput(pin, outputType);
    }

    bool isOutput(std::uint8_t pin) const
    {
        return static_cast<const Derived*>(this)->_isOutput(pin);
    }

    void makeInput(std::uint8_t pin, InputType inputType)
    {
        static_cast<Derived*>(this)->_makeInput(pin, inputType);
    }

    bool isInput(std::uint8_t pin) const
    {
        return static_cast<const Derived*>(this)->_isInput(pin);
    }

    void set(std::uint8_t pin, bool value)
    {
        static_cast<Derived*>(this)->_set(pin, value);
    }

    bool readInput(std::uint8_t pin) const
    {
        return static_cast<const Derived*>(this)->_readInput(pin);
    }

    bool readOutput(std::uint8_t pin) const
    {
        return static_cast<const Derived*>(this)->_readOutput(pin);
    }
};

/// Base class for GPIO peripheral that abstracts hardware.
/// Contains all ports on device.
///
/// \tparam Derived actual implementation of GPIO.
/// \tparam Port_ actual implementation of PortBase.
/// \tparam portsCount_ Count of ports on device.
template<typename Derived, typename Port_, std::size_t portsCount_>
class GpioBase
{
public:
    static constexpr std::size_t portsCount = portsCount_;
    using Port = PortBase<Port_, Port_::pinsCount>;

    /// Initializes array of ports with given values.
    template<typename... Args>
    GpioBase(Args&&... args) :
        ports_{std::forward<Args>(args)...}
    { }

    /// Returns given port.
    Port& port(std::uint8_t idx)
    {
        return ports_[idx];
    }

protected:
    std::array<Port_, portsCount> ports_;
};

}
