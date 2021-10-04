#include <stm32/peripheral/Gpio.hpp>
#include <stm32/peripheral/Irq.h>

namespace DynaSoft
{

StmGpio::StmGpio() : GpioBase{GPIOA, GPIOB, GPIOC, GPIOD}
{
    RCC_APB2PeriphClockCmd(
            RCC_APB2Periph_GPIOA
          | RCC_APB2Periph_GPIOB
          | RCC_APB2Periph_GPIOC
          | RCC_APB2Periph_GPIOD
          | RCC_APB2Periph_AFIO
          , ENABLE);
}

}
