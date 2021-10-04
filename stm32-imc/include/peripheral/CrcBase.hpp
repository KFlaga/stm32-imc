#pragma once

#include <containers/Span.hpp>
#include <cstdint>
#include <type_traits>

namespace DynaSoft
{

/// Base class for CRC unit, that abstracts CRC hardware/algorithm.
///
/// As it may use global CRC hardware it's state should be treaded as shared
/// between CrcBase instances.
///
/// \tparam Derived actual implementation of CRC.
///
/// Class Derived needs to add CrcBase as friend.
template<typename Derived>
class CrcBase
{
public:
	/// Creates CRC unit interface with reset state
	CrcBase() {}

	/// Creates CRC unit interface and adds buffer to it
	/// \param buffer Span of unsigned integer type.
	template<typename T>
	CrcBase(Span<T> buffer)
	{
		add(buffer);
	}

	/// Adds next number to be CRCed.
    void add(std::uint32_t x)
    {
        static_cast<Derived*>(this)->_add(x);
    }

    /// Adds all elements of given buffer. Each element is casted to std::uint32_t and CRCed.
	/// \param buffer Span of unsigned integer type.
	template<typename T>
    void add(Span<T> buffer)
    {
		static_assert(std::is_integral<T>::value && std::is_unsigned<T>::value, "CrcBase::add requires buffer of integral unsigned type");
        for(std::uint8_t x: buffer)
        {
            add(static_cast<std::uint32_t>(x));
        }
    }

    /// Returns current CRC value.
    std::uint32_t get()
    {
        return static_cast<Derived*>(this)->_get();
    }

    /// Resets this CRC unit state.
    void reset()
    {
        static_cast<Derived*>(this)->_reset();
    }
};
}
