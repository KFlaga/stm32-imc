# stm32-imc
Simple data exchange protocol between two MCUs based on UART.

Header-only library stm32-imc contains implementation of IMC (Inter-MCU Communication) protocol and hardware abstractions.
Library stm32-peripheral contains concrete implementations of abstract classes representing peripherals.
imc-example have main.cpp with example how to use libraries above to send messages between 2 devices.

Details of protocol are described in more detail in sources
([InterMcuCommunicationModule.hpp](stm32-imc/include/imc/InterMcuCommunicationModule.hpp) and [ImcProtocol.hpp](stm32-imc/include/imc/ImcProtocol.hpp)).

Ready to be built for stm32f103 using arm-gcc toolchain which supports C++17.
To generate out-of-source build files for project imc-example with cmake you can call it like:
```
cmake \
    --toolchain=../cmake/toolchain-stm32.cmake \
    -DBUILD_TARGET=stm32 \
    -DTARGET_TRIPLET=arm-none-eabi \
    -DTOOLCHAIN_PREFIX="c:/Program Files (x86)/Atollic/xpack-arm-none-eabi-gcc-9.2.1-1.1" \
    -DIMC_DEVICE_ROLE=Master \
    ..
```
To build uts, which run on PC, only required argument is `-DBUILD_TARGET=tests` (uses default toolchain).

Also contains projects for Atollic TrueStudio, which works after upgrading it's toolchain to more modern gcc.

It is part of bigger commercial project from DynaSim (dynasim.eu) that is used in DynaSim microcontrollers.
The IMC part is used for communication between two MCUs in driving wheel input device -
master device is in static base and slave device inside rotating wheel. As UART requires only 2 pins,
slave device may be connected using only 4-wire cable and cable thickness needed to be kept low.
