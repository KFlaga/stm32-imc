#pragma once

#include <type_traits>
#include <tuple>
#include <initializer_list>

/// Expands any expression that contains variadic parameter
///
/// Abuses initializer list unfolding to call expr for each argument in variadic parameter pack
/// in correct order (as evaluation of expressions inside initializer list is defined).
#define EXPAND_VARIADIC_EXPRESSION(expr) (void)std::initializer_list<int>{((expr), 0)...};

namespace mp
{

template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template<typename T>
using get_type = typename T::type;

template<typename From, template<typename...> class To>
struct copy_args;

template<template<typename...> class From, template<typename...> class To, typename... T1>
struct copy_args<From<T1...>, To>
{
    using type = To<T1...>;
};

/// Copies template arguments from type From to template type To.
///
/// If we have an instantiated template type From<T1, T2>
/// and an uninstantiated template type To, copy_args_t<From<T1, T2>, To>
/// defines type To<T1, T2>.
template<typename From, template<typename...> class To>
using copy_args_t = typename copy_args<From, To>::type;


template<template<typename> class Modifier, typename T>
struct apply_on_args;

template<template<typename> class Modifier, template<typename...> class T, typename... Ts>
struct apply_on_args<Modifier, T<Ts...>>
{
    using type = T<get_type<Modifier<Ts>>...>;
};

/// Applies a Modifier to each type on template type T.
///
/// If we have some type instantiated template type T<T1, T2> and
/// an uninstantiated template type Modifier, apply_on_args_t<Modifier, T<T1, T2>>
/// defines type T<Modifer<T1>, Modifier<T2>>.
template<template<typename> class Modifier, typename T>
using apply_on_args_t = get_type<apply_on_args<Modifier, T>>;


template<typename T>
struct identity
{
    using type = T;
};

template<typename... Ts>
struct parameter_pack {};


/// Returns number of types in template pack or in template type
template<typename... Ts>
struct parameters_count
{
    static constexpr auto value = sizeof...(Ts);
};

template<template<typename...> class T, typename... Ts>
struct parameters_count<T<Ts...>>
{
    static constexpr auto value = sizeof...(Ts);
};

namespace detail
{
template<typename Expr, typename... Args>
constexpr auto is_valid_expression_helper(Expr&& e, parameter_pack<Args...>) -> decltype(e(std::declval<Args>()...), bool())
{
    return true;
}

constexpr bool is_valid_expression_helper(...)
{
    return false;
}
}

/// Returns true if e(t...) is compilable, where t... is list of references of type Args....
template<typename... Args, typename Expr>
constexpr bool is_valid_expression(Expr&& e)
{
    return detail::is_valid_expression_helper(std::forward<Expr>(e), parameter_pack<Args...>{});
}

namespace detail
{
template<typename T, typename... Args>
constexpr auto is_valid_initialization_helper(identity<T>, parameter_pack<Args...>) -> decltype(T{std::declval<Args>()...}, bool())
{
    return true;
}

constexpr bool is_valid_initialization_helper(...)
{
    return false;
}
}

/// Returns true if T{Args...} is compilable.
template<typename T, typename... Args>
constexpr bool is_valid_initialization()
{
    return detail::is_valid_initialization_helper(identity<T>{}, parameter_pack<Args...>{});
}


/// Has static member value indicating whether type T is instantiation of template type Tmp.
template<template<typename...> class Tmp, typename T>
struct is_instantiation_of : std::false_type {};

template<template<typename...> class Tmp1, typename... Ts>
struct is_instantiation_of<Tmp1, Tmp1<Ts...>> : std::true_type {};

namespace detail
{
template <class F, class Tuple, std::size_t... I>
constexpr decltype(auto) apply_impl(F&& f, Tuple&& t, std::index_sequence<I...>)
{
    EXPAND_VARIADIC_EXPRESSION(f(std::get<I>(std::forward<Tuple>(t))));
}
}

/// Calls function f on each element of tuple t in order.
/// f must be generic lambda or overload set that accepts all types in tuple.
template <class F, class Tuple>
constexpr decltype(auto) apply(F&& f, Tuple&& t)
{
    return detail::apply_impl(
        std::forward<F>(f), std::forward<Tuple>(t),
        std::make_index_sequence<std::tuple_size<std::remove_reference_t<Tuple>>::value>{});
}

/// Evaluates function f(args) if value == true.
/// If value == false, this call is not compiled at all.
template<bool value, typename Func, typename... Args>
constexpr void eval_if(Func&& f, [[maybe_unused]] Args&&... args)
{
    if constexpr(value)
    {
        f(std::forward<Args>(args)...);
    }
    else
    {
    }
}

/// Evaluates function f(args) if value == true or g(args) if value == false.
/// Only call that is evaluated is compiled.
template<bool value, typename If, typename Else, typename... Args>
constexpr void eval_if_else(If&& f, Else&& g, Args&&... args)
{
    if constexpr(value)
    {
        f(std::forward<Args>(args)...);
    }
    else
    {
        g(std::forward<Args>(args)...);
    }
}

/// Evaluates function f(args) if types T and U are same.
template<typename T, typename U, typename Func>
constexpr void eval_if_is(U&& t, Func&& f)
{
    using rawU = std::remove_cv_t<std::remove_reference_t<U>>;
    eval_if<std::is_base_of<T, rawU>::value>(std::forward<Func>(f), std::forward<U>(t));
}

/// Evaluates function f(args) if types T and U are same.
template<typename T, typename U, typename If, typename Else>
constexpr void eval_if_is_else(U&& t, If&& f, Else&& g)
{
    using rawU = std::remove_cv_t<std::remove_reference_t<U>>;
    eval_if_else<std::is_base_of<T, rawU>::value>(std::forward<If>(f), std::forward<Else>(g), std::forward<U>(t));
}

/// Returns true if U is in parameter pack T.
template<typename U, typename... T>
constexpr bool contains()
{
    // U is in T we got sth like
    // integer_sequence<bool, false, false, true, false>
    // integer_sequence<bool, false, false, false, true>
    // So sequences are not same. Otherwise both are full of false.
    return not std::is_same<
        std::integer_sequence<bool, false, std::is_same<U, T>::value...>,
        std::integer_sequence<bool, std::is_same<U, T>::value..., false>
    >::value;
}

template<typename, typename>
struct pack_contains {};

/// Has static member value indicating whether T is in pack Ts.
template<typename T, typename... Ts>
struct pack_contains<T, mp::parameter_pack<Ts...>>
{
    static constexpr bool value = mp::contains<T, Ts...>();
};

template<typename F>
struct make_tag
{
};

namespace detail
{
template<int N, typename T, typename Pack>
struct pack_position_helper {};

template<int N, typename T, typename F, typename... Fs>
struct pack_position_helper<N, T, mp::parameter_pack<F, Fs...>>
{
    static constexpr int value = std::is_same<remove_cvref_t<T>, remove_cvref_t<F>>::value ?
            N :
            pack_position_helper<N + 1, T, mp::parameter_pack<Fs...>>::value;
};

template<int N, typename T>
struct pack_position_helper<N, T, mp::parameter_pack<>>
{
    static constexpr int value = -1;
};
}

/// Has static member value indicating on which position in parameter pack is T.
/// Pack should be an instantiated template type.
template<typename T, typename Pack>
struct pack_position
{
    static constexpr int value = detail::pack_position_helper<0, T, Pack>::value;
};

namespace detail
{
template<int current, int desired, typename T, typename... Ts>
struct type_at_helper
{
	static_assert(desired - current <= sizeof...(Ts), "type_at: index must be smaller than number of types in variadic pack");
	using type = typename type_at_helper<current+1, desired, Ts...>::type;
};

template<int desired, typename T, typename... Ts>
struct type_at_helper<desired, desired, T, Ts...>
{
	using type = T;
};
}

/// Defines i-th type in pack Ts..
template<int i, typename...Ts>
using type_at = typename detail::type_at_helper<0, i, Ts...>::type;

/// Has static member value indicating whether T is the only type in Ts
template<typename T, typename... Ts>
struct is_only_type_in_pack
{
    static constexpr bool value = sizeof...(Ts) == 1 && pack_position<T, parameter_pack<Ts...>>::value == 0;
};

}
