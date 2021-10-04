#pragma once

#include <utility>
#include <type_traits>
#include <functional>

namespace DynaSoft
{

using CallbackContext = void*;

/// Wrapper for callback function with context.
/// \tparam C Function or function pointer signature. Should accept CallbackContext as its first argument.
template<typename C>
struct Callback
{
	using CallbackType = std::conditional_t<std::is_function_v<C>, C*, C>;
	struct ResetFunc {};

	CallbackType func;
    CallbackContext context;

    /// Calls func with context and given args if func is set.
    /// If func is not set returns default value of return type.
    template<typename... Args>
    auto operator()(Args&&... args)
    {
    	return invoke(func, std::forward<Args>(args)...);
    }

    /// Calls func with context and given args if func is set.
    /// If func is not set returns default value of return type.
    /// Callback function is reset to nullptr before old function is called.
    template<typename... Args>
	auto callAndReset(Args&&... args)
    {
    	auto f = func;
    	func = nullptr;
    	return invoke(f, std::forward<Args>(args)...);
    }

    bool isSet() const
    {
    	return func != nullptr;
    }

    operator bool()
	{
    	return isSet();
	}

private:
    template<typename... Args>
	auto invoke(CallbackType f, Args&&... args)
	{
		using Ret = decltype(f(context, std::forward<Args>(args)...));
		if(f != nullptr)
		{
			return std::invoke(f, context, std::forward<Args>(args)...);
		}

		if constexpr (!std::is_void_v<Ret>)
		{
			return Ret{};
		}
		else
		{
			return;
		}
	}
};

}
