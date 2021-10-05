#pragma once

#include <peripheral/UsTimerBase.hpp>
#include <array>
#include <cstdint>

#include "stm32f10x_tim.h"

namespace DynaSoft
{
class StmUsTimer : public UsTimerBase<StmUsTimer>
{
    static constexpr std::int32_t desiredPeriodUs = 50000;
    static constexpr std::int32_t maxOverflows = 10000;

public:
    static constexpr std::int32_t maxTime = (maxOverflows + 1) * desiredPeriodUs;

    StmUsTimer();

    std::uint32_t readRaw()
    {
        volatile std::int32_t cnt = TIM2->CNT;
        return overflows * desiredPeriodUs + cnt;
    }

protected:
    friend UsTimerBase<StmUsTimer>;

    void _turnOn();
    void _turnOff();

    std::uint32_t _readUs(std::uint8_t channel)
    {
        volatile std::int32_t cnt = TIM2->CNT;
        std::int32_t current = overflows * desiredPeriodUs + cnt;
        std::int32_t last = lastReadings[channel];
        std::int32_t diff = current - last;
        if(diff >= 0)
        {
            return static_cast<std::uint32_t>(diff);
        }
        else
        {
            return static_cast<std::uint32_t>(diff + maxTime);
        }
    }

    void _reset(std::uint8_t channel)
    {
        // std::int32_t cnt = TIM2->CNT;
        lastReadings[channel] = overflows * desiredPeriodUs + TIM2->CNT;
    }

    std::uint32_t _maxReading()
    {
        return maxTime;
    }

    std::array<std::int32_t, channelsCount> lastReadings;
    volatile std::int32_t overflows;
};
}
