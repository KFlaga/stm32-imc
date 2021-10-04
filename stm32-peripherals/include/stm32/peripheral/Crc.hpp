#pragma once

#include <peripheral/CrcBase.hpp>
#include "stm32f10x_crc.h"
#include "stm32f10x_rcc.h"

namespace DynaSoft
{

class StmCrc : public CrcBase<StmCrc>
{
public:
    StmCrc()
    {
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
        reset();
    }

    template<typename T>
    StmCrc(Span<T> x) : CrcBase{x}
    {
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC, ENABLE);
        reset();
    }

    void _add(std::uint32_t x)
    {
        CRC->DR = x;
    }

    std::uint32_t _get()
    {
        return CRC->DR;
    }

    void _reset()
    {
        CRC->CR = CRC_CR_RESET;
    }
};

}
