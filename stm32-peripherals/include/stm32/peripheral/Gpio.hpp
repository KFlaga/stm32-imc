#pragma once

#include <peripheral/GpioBase.hpp>
#include <peripheral/Pins.hpp>
#include <cstdint>

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

namespace DynaSoft
{
class StmPort : public PortBase<StmPort, 16>
{
public:
    StmPort(GPIO_TypeDef* port_) :
        port{port_}
    {
        GPIO_DeInit(port_);
    }

    std::uint16_t toPin(std::uint8_t x) const
    {
        return 1u << static_cast<std::uint16_t>(x);
    }

protected:
    friend PortBase<StmPort, 16>;

    bool _isOutput(std::uint8_t pin) const
    {
        return _readMode(pin) > 0u;
    }

    bool _isInput(std::uint8_t pin) const
    {
        return _readMode(pin) == 0u;
    }

    uint32_t _readMode(std::uint8_t pin) const
    {
        constexpr uint32_t modeMask = 0x03;
        if(pin >= 8)
        {
            return (port->CRH & (modeMask << static_cast<std::uint32_t>((pin - 8) * 4)));
        }
        else
        {
            return (port->CRL & (modeMask << static_cast<std::uint32_t>(pin * 4)));
        }
    }

    void _set(std::uint8_t pin, bool value)
    {
        if (value)
        {
            port->BSRR = toPin(pin);
        }
        else
        {
            port->BRR = toPin(pin);
        }
    }

    bool _readOutput(std::uint8_t pin) const
    {
        return (port->ODR & toPin(pin)) != 0u;
    }

    bool _readInput(std::uint8_t pin) const
    {
        return (port->IDR & toPin(pin)) != 0u;
    }

    void _makeInput(std::uint8_t pin, InputType inputType)
    {
        GPIO_InitTypeDef pinInitializer;
        pinInitializer.GPIO_Pin = toPin(pin);
        if(inputType == InputType::Analog)
        {
            pinInitializer.GPIO_Mode = GPIOMode_TypeDef::GPIO_Mode_AIN;
            pinInitializer.GPIO_Speed = GPIOSpeed_TypeDef::GPIO_Speed_2MHz;
        }
        else if(inputType == InputType::DigitalNormalLow)
        {
            pinInitializer.GPIO_Mode =  GPIOMode_TypeDef::GPIO_Mode_IPD;
            pinInitializer.GPIO_Speed = GPIOSpeed_TypeDef::GPIO_Speed_10MHz;
        }
        else
        {
            pinInitializer.GPIO_Mode =  GPIOMode_TypeDef::GPIO_Mode_IPU;
            pinInitializer.GPIO_Speed = GPIOSpeed_TypeDef::GPIO_Speed_10MHz;
        }
        GPIO_Init(port, &pinInitializer);
    }

    void _makeOutput(std::uint8_t pin, std::uint8_t outputType)
    {
        GPIO_InitTypeDef pinInitializer;
        pinInitializer.GPIO_Pin = toPin(pin);

        if(outputType & OutputType::afio)
        {
            if(outputType & OutputType::openDrain)
            {
                pinInitializer.GPIO_Mode = GPIOMode_TypeDef::GPIO_Mode_AF_OD;
            }
            else
            {
                pinInitializer.GPIO_Mode = GPIOMode_TypeDef::GPIO_Mode_AF_PP;
            }
        }
        else
        {
            if(outputType & OutputType::openDrain)
            {
                pinInitializer.GPIO_Mode = GPIOMode_TypeDef::GPIO_Mode_Out_OD;
            }
            else
            {
                pinInitializer.GPIO_Mode = GPIOMode_TypeDef::GPIO_Mode_Out_PP;
            }
        }

        if(outputType & OutputType::fastSpeed)
        {
            pinInitializer.GPIO_Speed = GPIOSpeed_TypeDef::GPIO_Speed_50MHz;
        }
        else
        {
            pinInitializer.GPIO_Speed = GPIOSpeed_TypeDef::GPIO_Speed_2MHz;
        }
        GPIO_Init(port, &pinInitializer);
    }

private:
    GPIO_TypeDef* port;
};

class StmGpio : public GpioBase<StmGpio, StmPort, 4>
{
public:
    StmGpio();

protected:
    friend GpioBase<StmGpio, StmPort, 4>;
};
}
