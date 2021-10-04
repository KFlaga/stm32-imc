#pragma once

#include <misc/Callback.hpp>
#include <cstdint>
#include <atomic>

namespace DynaSoft
{

/// Interface for timer that allows to schedule a callback to be called after given time
template<typename Derived, std::uint8_t channelsCount_>
class InterruptTimerBase
{
public:
	static constexpr std::uint8_t channelsCount = channelsCount_;

	/// Function signature of callback that is called when timer interrupt fires.
	/// /param Context context - context pointer passed to scheduleInterrupt()
	/// /param std::uint32_t passedUs - count of microseconds that passed since call to scheduleInterrupt()
	using CallbackFunc = void(CallbackContext, std::uint32_t);
	using Callback = DynaSoft::Callback<CallbackFunc>;

	/// Schedules a callback to be called after given time on given channel.
	/// Previous callback on this channel is overridden.
	/// Channels are shared between InterruptTimer instances.
	/// It shouldn't be called when in interrupt, except from inside callback function.
	/// Callback is called only once, then reset.
	/// \param channel Channel which on callback is registers. Only one callback per channel may be active.
	/// \param fireAfterUs Time after which callback is fired, in microseconds. Rounded up to next 10us.
	/// \param cb Callback to be called.
	/// \return True if callback was successfully registered.
	bool scheduleInterrupt(std::uint8_t channel, std::uint32_t fireAfterUs, Callback cb)
	{
        return static_cast<Derived*>(this)->_scheduleInterrupt(channel, fireAfterUs, cb);
	}
};

}
