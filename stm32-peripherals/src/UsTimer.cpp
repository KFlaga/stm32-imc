#include <stm32/peripheral/Irq.h>
#include <stm32/peripheral/UsTimer.hpp>
#include "stm32f10x_rcc.h"
#include "misc.h"

static constexpr std::uint32_t prescaler1us = 71;
static std::int32_t maxOverflows_;
static volatile std::int32_t* overflows_;

extern "C"
{
void TIM2_IRQHandler(void)
{
    if(TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;
        if(*overflows_ < maxOverflows_)
        {
            (*overflows_)++;
        }
        else
        {
            (*overflows_) = 0;
        }
    }
}
}

namespace DynaSoft
{
StmUsTimer::StmUsTimer()
{
    overflows = 0;
    overflows_ = &overflows;
    maxOverflows_ = maxOverflows;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim;
    TIM_TimeBaseStructInit(&tim);
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    tim.TIM_Prescaler = prescaler1us;
    tim.TIM_Period = desiredPeriodUs - 1;
    tim.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &tim);

    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = IRQ_PRIORITY_GROUP_CRITICAL;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = IRQ_PRIORITY_GROUP_HIGH;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    // Irq on overflow
    TIM2->DIER = TIM_DIER_UIE;
}

void StmUsTimer::_turnOn()
{
    TIM_Cmd(TIM2, ENABLE);
    for(std::uint8_t i = 0; i < lastReadings.size(); ++i)
    {
        lastReadings[i] = 0;
    }
}

void StmUsTimer::_turnOff()
{
    TIM_Cmd(TIM2, DISABLE);
}

}
