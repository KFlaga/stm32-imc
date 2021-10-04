#pragma once

#include <peripheral/InterruptTimerBase.hpp>

namespace DynaSoft
{
class StmInterruptTimer : public InterruptTimerBase<StmInterruptTimer, 4>
{
public:
	StmInterruptTimer();

	bool _scheduleInterrupt(std::uint8_t channel, std::uint32_t us, Callback cb);
};
}
