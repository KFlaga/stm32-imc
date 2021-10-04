# stm32-imc
Simple data exchange protocol between two MCUs based on UART.

Header-only library stm32-imc contains implementation of IMC (Inter-MCU Communication) protocol and hardware abstractions.
Library stm32-peripheral contains concrete implementations of abstract classes representing peripherals.
imc-example have main.cpp with example how to use libraries above to send messages between 2 devices.

Details of protocol are described in more detail in sources.

Ready to be build with Atollic TrueStudio for stm32f103 after upgrading it's toolchain to use GCC which supports C++17.
Probably CMake files will be prepared as well soon.

It is part of bigger commercial project from DynaSim (dynasim.eu) that is used in DynaSim microcontrollers.
The IMC part is used for communication between two MCUs in driving wheel input device -
master device is in static base and slave device inside rotating wheel. As UART requires only 2 pins,
slave device may be connected using only 4-wire cable and cable thickness needed to be kept low.
