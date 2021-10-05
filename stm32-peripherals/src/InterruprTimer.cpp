#include <stm32/peripheral/InterruptTimer.hpp>
#include <stm32/peripheral/Irq.h>
#include "stm32f10x_tim.h"
#include "stm32f10x_rcc.h"
#include <atomic>
#include <array>
#include "misc.h"

namespace
{
using namespace DynaSoft;

constexpr std::uint16_t timStepUs = 10;
constexpr std::uint16_t prescaler10us = 719;
constexpr std::uint16_t timPeriod = 0xFFFF;

struct Event
{
    StmInterruptTimer::Callback c{};
    std::uint32_t startCNT = 0;
};

std::array<Event, StmInterruptTimer::channelsCount> events{};

inline std::uint32_t getUs(std::uint32_t startCNT)
{
    std::uint32_t cnt = TIM3->CNT;
    return cnt > startCNT ? (cnt - startCNT) * timStepUs : (timPeriod - startCNT + cnt) * timStepUs;
}

inline std::uint16_t getCNT(std::uint32_t us)
{
    std::uint16_t timSteps = ((us + timStepUs - 1) / timStepUs);
    std::uint16_t cnt = TIM3->CNT;
    std::uint16_t rem = timPeriod - cnt;
    return rem >= timSteps ? cnt + timSteps : timSteps - rem;
}

inline void updateCCRx(int channel, std::uint16_t flag, std::uint16_t itFlag)
{
    if((TIM3->SR & flag) != 0)
    {
        TIM_ITConfig(TIM3, itFlag, DISABLE);

        events[channel].c.callAndReset(getUs(events[channel].startCNT));

        TIM_ClearITPendingBit(TIM3, flag);
    }
}

}

extern "C"
{
void TIM3_IRQHandler(void)
{
    updateCCRx(0, TIM_FLAG_CC1, TIM_IT_CC1);
    updateCCRx(1, TIM_FLAG_CC2, TIM_IT_CC2);
    updateCCRx(2, TIM_FLAG_CC3, TIM_IT_CC3);
    updateCCRx(3, TIM_FLAG_CC4, TIM_IT_CC4);
}
}

namespace DynaSoft
{

StmInterruptTimer::StmInterruptTimer()
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInitTypeDef tim;
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Prescaler = prescaler10us;
    tim.TIM_Period = timPeriod;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM3, &tim);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_GROUP_NORMAL;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = IRQ_PRIORITY_SUB_NORMAL;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    TIM_Cmd(TIM3, ENABLE);
}

bool StmInterruptTimer::_scheduleInterrupt(std::uint8_t channel, std::uint32_t us, Callback info)
{
    switch(channel)
    {
    case 0:
        TIM_SetCompare1(TIM3, getCNT(us));
        TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
        break;
    case 1:
        TIM_SetCompare2(TIM3, getCNT(us));
        TIM_ITConfig(TIM3, TIM_IT_CC2, ENABLE);
        break;
    case 2:
        TIM_SetCompare3(TIM3, getCNT(us));
        TIM_ITConfig(TIM3, TIM_IT_CC3, ENABLE);
        break;
    case 3:
        TIM_SetCompare4(TIM3, getCNT(us));
        TIM_ITConfig(TIM3, TIM_IT_CC4, ENABLE);
        break;
    default:
        return false;
    }

    std::uint16_t cnt = TIM3->CNT;
    events[channel].c = info;
    events[channel].startCNT = cnt;
    return true;
}

}
