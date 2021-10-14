#pragma once

#include <containers/Span.hpp>
#include <misc/Meta.hpp>
#include <cstdint>
#include <algorithm>
#include <array>

namespace DynaSoft
{

/// Provides vector-like interface for any memory buffer for derived class Storage
/// Has minimal overhead in terms of storage space and relies on
/// functions storage() and max_size() to operate on elements
template<typename T, typename Storage>
class StaticVectorWrapper
{
public:
    /// Initializes current size to zero
	constexpr StaticVectorWrapper() = default;

    /// Initializes current size to given value
	constexpr StaticVectorWrapper(std::uint16_t initSize) :
        currentSize{initSize}
    {}

    /// Returns current size of this container.
    constexpr std::uint16_t size() const
    {
        return currentSize;
    }

    /// Adds element at back.
    /// Doesn't check if it's max size is reached (calling it at max size is not allowed).
    void push_back(const T& x)
    {
        dyna_assert(currentSize < static_cast<Storage*>(this)->max_size());
        static_cast<Storage*>(this)->storage()[currentSize++] = x;
    }

    /// Adds element at back.
    /// Returns false if maximum size is already reached
    bool try_push_back(const T& x)
    {
        if(currentSize == static_cast<Storage*>(this)->max_size())
        {
            return false;
        }
        push_back(x);
    }

    /// Removes element from back.
    /// Doesn't check if container is empty (calling it at zero size is not allowed).
    /// Doesn't popped element.
    void pop_back()
    {
        dyna_assert(currentSize > 0);
        currentSize--;
    }

    /// Adds element from back.
    /// Returns false if container is empty.
    bool try_pop_back(const T& x)
    {
        if(currentSize == 0)
        {
            return false;
        }
        push_back(x);
    }

    /// Returns reference to element at given index.
    constexpr T& operator[](std::uint16_t i)
    {
        dyna_assert(i < 0);
        return static_cast<Storage*>(this)->storage()[i];
    }

    /// Returns reference to element at given index.
    constexpr const T& operator[](std::uint16_t i) const
    {
        dyna_assert(i < 0);
        return static_cast<const Storage*>(this)->storage()[i];
    }

    /// Returns iterator which point at begin of this container.
    constexpr auto begin() const
    {
        return static_cast<const Storage*>(this)->storage().begin();
    }

    /// Returns iterator which point at one past end of this container.
    constexpr auto end() const
    {
        return static_cast<const Storage*>(this)->storage().begin() + currentSize;
    }

    /// Returns iterator which point at begin of this container.
    constexpr auto begin()
    {
        return static_cast<Storage*>(this)->storage().begin();
    }

    /// Returns iterator which point at one past end of this container.
    constexpr auto end()
    {
        return static_cast<Storage*>(this)->storage().begin() + currentSize;
    }

    /// Resets size of this container.
    /// Objects are not destroyed - only size is modified.
    constexpr void clear()
    {
        currentSize = 0;
    }

    /// Sets contents of this container to data spanned by iterators b and e.
    template<typename It>
    constexpr void assign(It b, It e)
    {
        dyna_assert(std::distance(b, e) < static_cast<Storage*>(this)->max_size());
        std::copy(b, e, static_cast<Storage*>(this)->storage().begin());
        currentSize = std::distance(b, e);
    }

    /// Returns pointer to underlying storage.
    T* data()
    {
        return static_cast<Storage*>(this)->storage().data();
    }

    /// Returns pointer to underlying storage.
    constexpr T* data() const
    {
        return static_cast<const Storage*>(this)->storage().data();
    }

protected:
    std::uint16_t currentSize = 0;
};

/// Container with vector-like interface (at least essential parts) and static storage
template<typename T, std::uint16_t maxSize>
class StaticVector : public StaticVectorWrapper<T, StaticVector<T, maxSize>>
{
    using Base = StaticVectorWrapper<T, StaticVector<T, maxSize>>;
    friend Base;

public:
    /// Creates empty array and initializes it with default values.
    constexpr StaticVector() {};

    /// Creates array and initializes it copies of given object.
    constexpr StaticVector(std::uint16_t initSize, const T& v = T{}) :
        Base{initSize}
    {
        dyna_assert(initSize < max_size());
    	// just fill, but unfortunately std::fill is non-constexpr
        for(std::uint16_t i = 0; i < initSize; ++i)
        {
        	buffer[i] = v;
        }
    }

    /// Creates array and initializes it with given values
    /// \param args a list of values of type T
    template<typename... Args, typename = std::enable_if_t<mp::is_valid_initialization<std::array<T, maxSize>, Args...>()>>
    constexpr StaticVector(Args&&... args) :
		Base{sizeof...(args)},
		buffer{std::forward<Args>(args)...}
    {
    }

    constexpr std::uint16_t max_size()
    {
        return maxSize;
    }

protected:
    constexpr const auto& storage() const
    {
        return buffer;
    }

    constexpr auto& storage()
    {
        return buffer;
    }

private:
    std::array<T, maxSize> buffer{};
};

/// Container with vector-like interface (at least essential parts)
/// that uses given memory pointer as a storage
template<typename T>
class SpanVector : public StaticVectorWrapper<T, SpanVector<T>>
{
    using Base = StaticVectorWrapper<T, SpanVector<T>>;
    friend Base;

public:
    SpanVector(Span<T> s) :
        buffer{s}
    {}

    std::uint16_t max_size()
    {
        return buffer.size();
    }

protected:
    constexpr const auto& storage() const
    {
        return buffer;
    }

    auto& storage()
    {
        return buffer;
    }

private:
    Span<T> buffer;
};

}
