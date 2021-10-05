#pragma once

#include <cstdint>
#include <array>
#include <algorithm>

namespace DynaSoft
{

/// An array which remembers its current size, but has static buffer and so limited capacity.
template<typename T, std::uint16_t maxSize>
struct DynamicArray
{
    /// Creates empty array and initializes it with default values.
    DynamicArray() {};

    /// Creates array and initializes it copies of given object.
    DynamicArray(std::uint16_t initSize, const T& v = T{}) :
        currentSize{initSize}
    {
        std::fill(begin(), end(), v);
    }

    /// Creates array and initializes it with given initializer list.
    DynamicArray(std::initializer_list<T> ts) :
        buffer{ts},
        currentSize{(std::uint16_t)ts.size()}
    {
    }

    /// Returns current size of this container.
    std::uint16_t size() const
    {
        return currentSize;
    }

    /// Adds element at back.
    /// Doesn't check if it's max size is reached (calling it at max size is not allowed).
    void push_back(const T& x)
    {
        buffer[currentSize++] = x;
    }

    /// Adds element at back.
    /// Returns false if maximum size is already reached
    bool try_push_back(const T& x)
    {
        if(currentSize == maxSize)
        {
            return false;
        }
        push_back(x);
    }

    /// Adds element from back.
    /// Doesn't check if container is empty (calling it at zero size is not allowed).
    void pop_back()
    {
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
    T& operator[](std::uint16_t i)
    {
        return buffer[i];
    }

    /// Returns reference to element at given index.
    const T& operator[](std::uint16_t i) const
    {
        return buffer[i];
    }

    /// Returns iterator which point at begin of this container.
    auto begin() const
    {
        return buffer.begin();
    }

    /// Returns iterator which point at one past end of this container.
    auto end() const
    {
        return buffer.begin() + currentSize;
    }

    /// Returns iterator which point at begin of this container.
    auto begin()
    {
        return buffer.begin();
    }

    /// Returns iterator which point at one past end of this container.
    auto end()
    {
        return buffer.begin() + currentSize;
    }

    /// Resets size of this container.
    /// Objects are not destroyed - only size is modified.
    void clear()
    {
        currentSize = 0;
    }

    /// Sets contents of this container to span of iterators b and e.
    template<typename It>
    void assign(It b, It e)
    {
        std::copy(b, e, buffer.begin());
        currentSize = std::distance(b, e);
    }

    /// Returns pointer to underlying storage.
    T* data()
    {
        return buffer.data();
    }

private:
    std::array<T, maxSize> buffer{};
    std::uint16_t currentSize = 0;
};

}
