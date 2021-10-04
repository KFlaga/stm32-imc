#pragma once

#include <type_traits>
#include <cmath>

namespace DynaSoft
{

/// Wraps 2 pointers that represents begin and end of some memory block.
template<typename T>
struct Span
{
    Span(T* begin_, T* end_) : from{begin_}, to{end_}
    {
    }

    T* begin()
    {
        return from;
    }

    const T* begin() const
    {
        return from;
    }

    T* end()
    {
        return to;
    }

    const T* end() const
    {
        return to;
    }

    std::size_t size() const
    {
        return std::abs(from - to);
    }

    T& operator[](int idx)
    {
        return *(from + idx);
    }

    const T& operator[](int idx) const
    {
        return *(from + idx);
    }

    T* data()
    {
        return from;
    }

    const T* data() const
    {
        return from;
    }

private:
    T* from;
    T* to;
};

template<typename T>
auto makeSpan(T* begin_, T* end_)
{
    return Span<T>(begin_, end_);
}

template<typename T>
auto makeSpan(T* begin_, std::size_t size_)
{
    return Span<T>(begin_, begin_ + size_);
}

}
