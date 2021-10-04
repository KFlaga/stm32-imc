#pragma once

#include <cstdint>
#include <type_traits>
#include <atomic>

namespace DynaSoft
{

/// Holds three buffers that are supposed to be separately read from / write to.
///
/// After writing to is done write buffer may be swapped with intermediate buffer,
/// which in turn may be swapped with read buffer to allow reading from it.
/// Using three buffers allows to write to next buffer before previous one was processed by reader.
///
/// \tparam Buffer Type of buffers that are wrapped.
/// \tparam enableExtraSynchronization If true swapping is done in thread-safe manner.
template<typename Buffer, bool enableExtraSynchronization = false>
class TripleBuffer
{
	using InterIndex = std::conditional_t<enableExtraSynchronization, std::atomic<std::uint8_t>, std::uint8_t>;

public:
	/// Constructs object with all three buffers initialized to default.
	/// Disabled if Buffer is not default constructible.
    template<typename dummy = void, typename = std::enable_if_t<std::is_default_constructible<Buffer>::value, dummy>>
	TripleBuffer() :
		bs{}
    {
    }

	/// Constructs object with three buffers initialized with given values.
    TripleBuffer(const Buffer& bufferRead, const Buffer& bufferWrite, const Buffer& bufferInter) :
        bs{bufferRead, bufferWrite, bufferInter}
    {
    }

    /// Returns buffer that is assigned to reading from.
    Buffer& read()
    {
    	return bs[readBuffer];
    }

    /// Returns buffer that is assigned to writing to.
    Buffer& write()
    {
    	return bs[writeBuffer];
    }

    /// Swaps read buffer with intermediate one.
    void swapRead()
    {
    	swap(readBuffer);
    }

    /// Swaps write buffer with intermediate one.
    void swapWrite()
    {
    	swap(writeBuffer);
    }

private:
    void swap(std::uint8_t& b)
    {
    	if constexpr(enableExtraSynchronization)
		{
			// Allow concurrent call to both swapXXX()
			// If swapWrite swaps interBuffer after 'swapped = interBuffer' in swapRead() then
			// compare_exchange_weak fails and loops again, so it will read from previous write buffer
			// If swapRead swaps it faster, then it will read from interBuffer, and swapWrite
			// will write to previous readBuffer
			// std::atomic also ensures memory synchronization if modified inside interrupt
			std::uint8_t swapped = interBuffer.load(std::memory_order_relaxed);
			while(!interBuffer.compare_exchange_weak(swapped, b, std::memory_order_acq_rel)) {}
			b = swapped;
		}
    	else
    	{
        	std::swap(b, interBuffer);
    	}
    }

    std::array<Buffer, 3> bs{};

    std::uint8_t readBuffer = 0;
    std::uint8_t writeBuffer = 1;
    InterIndex interBuffer = 2;
};

}
