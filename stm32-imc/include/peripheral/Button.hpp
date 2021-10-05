#pragma once

#include <peripheral/GpioBase.hpp>
#include <peripheral/Pins.hpp>

namespace DynaSoft
{

/// Wraps input pin and checks if its state is different than normal, what indicates a button press.
/// Filters noise that occurs during pressing by waiting 10ms after each signal level change for it to stabilize.
class Button
{
public:
    Button() {}

    Button(InputPin pin_) :
    	pin{pin_}
    {}

    template<typename Gpio>
    void update(Gpio& gpio, std::uint32_t loopUs)
    {
        auto state = gpio.port(pin.port).readInput(pin.pin);
        if(pin.inputType == InputType::DigitalNormalHigh)
        {
            state = !state;
        }

        if(state != lastStableState)
        {
            stateTimer += loopUs;
            if(stateTimer >= stateInterval)
            {
                lastStableState = state;
            }
        }
        else
        {
            stateTimer = 0;
        }
    }

    bool isPressed()
    {
        return lastStableState;
    }

private:
    InputPin pin;
    std::uint32_t stateTimer = 0;
    std::uint32_t stateInterval = 1000 * 10;
    bool lastStableState = false;
};


}
