#pragma once

#include <misc/Assert.hpp>
#include <cstdint>
#include <array>
#include <algorithm>

namespace DynaSoft
{

/// Stores data like RingBuffer - overwrites oldest element if maximum size is reached.
/// Has constant size (max) at construction and access unordered (unrelated to order of adding elements).
template<typename T, std::uint16_t maxSize>
class UnorderedRingBuffer
{
public:
    /// Creates UnorderedRingBuffer and initializes it's storage with default constructed objects.
    UnorderedRingBuffer() : data{} {};

    /// Creates UnorderedRingBuffer and initializes it's storage with copy of given object.
    UnorderedRingBuffer(const T& t)
    {
        std::fill(begin(), end(), t);
    };

    /// Returns size of container.
    /// It is always equal to its max size.
    constexpr std::uint16_t size() const
    {
        return maxSize;
    }

    /// Adds element to container.
    /// Overwrites oldest element.
    void push(const T& x)
    {
        data[next] = x;
        next = (next == size() - 1) ? 0 : next + 1;
    }

    /// Access element in container.
    /// Index specifies element place in memory, not in addition order
    auto operator[](int i) const
    {
        dyna_assert(i < maxSize);
        return data[i];
    }

    /// Returns index of first (oldest) element.
    std::uint16_t firstIndex() const
    {
        return next;
    }

    /// Returns index of last (newest) element.
    std::uint16_t lastIndex() const
    {
        return next > 0 ? next - 1 : maxSize - 1;
    }

    /// Returns next oldest element index after one at index i.
    std::uint16_t nextIndex(std::uint16_t i) const
    {
        dyna_assert(i < maxSize);
        return i >= maxSize - 1 ? 0 : i + 1;
    }

    /// Returns previous oldest element index after one at index i.
    std::uint16_t prevIndex(std::uint16_t i) const
    {
        dyna_assert(i < maxSize);
        return i == 0 ? maxSize - 1 : i - 1;
    }

    /// Returns iterator which point at begin of container.
    auto begin() const
    {
        return data.begin();
    }

    /// Returns iterator which point at one past end of container.
    auto end() const
    {
        return data.begin() + size();
    }

    /// Returns iterator which point at begin of container.
    auto begin()
    {
        return data.begin();
    }

    /// Returns iterator which point at one past end of container.
    auto end()
    {
        return data.begin() + size();
    }

    /// Calls function f on each element in order of adding.
    template<typename Func>
    void foreachOrdered(Func&& f)
    {
        for(std::uint16_t i = next; i < size(); ++i)
        {
            f(data[i]);
        }
        for(std::uint16_t i = 0; i < next; ++i)
        {
            f(data[i]);
        }
    }

    /// Returns underlying storage.
    std::array<T, maxSize>& storage()
    {
        return data;
    }

private:
    std::array<T, maxSize> data;
    std::uint16_t next = 0;
};

}
