#pragma once

#include <cstdint>

namespace DynaSoft
{

/// Base class for timer peripheral, that abstracts hardware.
/// Timer resolution is 1 microsecond.
///
/// \tparam Derived actual implementation of timer.
///
/// Class Derived needs to add UsTimerBase as friend.
template<typename UsTimer>
class UsTimerBase
{
public:
    static constexpr std::uint_fast8_t channelsCount = 8;

    /// Starts the timer.
    void turnOn()
    {
        static_cast<UsTimer*>(this)->_turnOn();
    }

    /// Stops the timer.
    void turnOff()
    {
        static_cast<UsTimer*>(this)->_turnOff();
    }

    /// Returns us passed since last reset on given channel.
    std::uint32_t readUs(std::uint8_t channel)
    {
        return static_cast<UsTimer*>(this)->_readUs(channel);
    }

    /// Resets us count of given channel.
    void reset(std::uint8_t channel)
    {
        static_cast<UsTimer*>(this)->_reset(channel);
    }

    /// Works like readUs() and reset() combined.
    std::uint32_t readUsAndReset(std::uint8_t channel)
    {
         std::uint32_t us = static_cast<UsTimer*>(this)->_readUs(channel);
        static_cast<UsTimer*>(this)->_reset(channel);
        return us;
    }

    /// Returns max possible value returned from readUs().
    std::uint32_t maxReading()
    {
        return static_cast<UsTimer*>(this)->_maxReading();
    }

};

}
