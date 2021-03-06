#include <misc/Meta.hpp>
#include <tests/framework.hpp>

using namespace mp;

struct T1 { constexpr double t1() { return 0.0; } };
struct T2 {};
struct T3 {};

template<typename... Ts>
struct PackA {};

template<typename... Ts>
struct PackB {};

template<typename T>
struct PackC {};

template<typename>
struct ChangeToInt
{
    using type = int;
};

static_assert(std::is_same<PackB<>, copy_args_t<PackA<>, PackB>>::value,
        "copy_args - copy empty");
static_assert(std::is_same<PackB<T1>, copy_args_t<PackA<T1>, PackB>>::value,
        "copy_args - copy one");
static_assert(std::is_same<PackB<T1, T2, T3>, copy_args_t<PackA<T1, T2, T3>, PackB>>::value,
        "copy_args - copy three");

static_assert(std::is_same<PackB<>, apply_on_args_t<identity, PackB<>>>::value,
        "apply_on_args - identity empty");
static_assert(std::is_same<PackB<T1>, apply_on_args_t<identity, PackB<T1>>>::value,
        "apply_on_args - identity one");
static_assert(std::is_same<PackB<T1, T2, T3>, apply_on_args_t<identity, PackB<T1, T2, T3>>>::value,
        "apply_on_args - identity three");

static_assert(std::is_same<PackB<>, apply_on_args_t<ChangeToInt, PackB<>>>::value,
        "apply_on_args - change empty");
static_assert(std::is_same<PackB<int>, apply_on_args_t<ChangeToInt, PackB<T1>>>::value,
        "apply_on_args - change one");
static_assert(std::is_same<PackB<int, int, int>, apply_on_args_t<ChangeToInt, PackB<T1, T2, T3>>>::value,
        "apply_on_args - change three");

static_assert(parameters_count<>::value == 0,
        "parameters_count<>");
static_assert(parameters_count<int>::value == 1,
        "parameters_count<int>");
static_assert(parameters_count<int, T1>::value == 2,
        "parameters_count<int, T1>");
static_assert(parameters_count<PackB<>>::value == 0,
        "parameters_count<PackB<>>");
static_assert(parameters_count<PackB<int>>::value == 1,
        "parameters_count<PackB<int>>");
static_assert(parameters_count<PackB<PackA<>>>::value == 1,
        "parameters_count<PackB<PackA<>>>");

static_assert(is_instantiation_of<PackA, PackA<>>::value,
        "is_instantiation_of OK 1");
static_assert(is_instantiation_of<PackA, PackA<T1>>::value,
        "is_instantiation_of OK 2");

static_assert(not is_instantiation_of<PackA, PackB<>>::value,
        "is_instantiation_of NOK 1");
static_assert(not is_instantiation_of<PackA, PackB<T1>>::value,
        "is_instantiation_of NOK 2");

static_assert(0 == pack_position<int, parameter_pack<int, float>>::value,
        "pack_position 0");
static_assert(1 == pack_position<float, parameter_pack<int, float>>::value,
        "pack_position 1");
static_assert(-1 == pack_position<char, parameter_pack<int, float>>::value,
        "pack_position 2");

static_assert(not is_only_type_in_pack<char>::value,
        "is_only_type_in_pack<char>");
static_assert(is_only_type_in_pack<char, char>::value,
        "is_only_type_in_pack<char, char>");
static_assert(not is_only_type_in_pack<char, char, int>::value,
        "is_only_type_in_pack<char, char, int>");
static_assert(not is_only_type_in_pack<char, int>::value,
        "is_only_type_in_pack<char, int>");


void f1(int) {}
void f2() {}
void f3(int, T1) {}

static_assert(is_valid_expression<int>(f1),
        "is_valid_expression<int>(void(int))");
static_assert(not is_valid_expression<int>(f2),
        "is_valid_expression<int>(void())");
static_assert(is_valid_expression<int, T1>(f3),
        "is_valid_expression<int, T1>(void(int, T1))");

static_assert(is_valid_expression<T1>([](auto&& x) -> decltype(x.t1()) {}),
        "is_valid_expression T1");
static_assert(not is_valid_expression<T2>([](auto&& x) -> decltype(x.t1()) {}),
        "is_valid_expression T2");

struct I1 {};
struct I2 { int x; };

static_assert(is_valid_initialization<I1>(),
        "is_valid_initialization<I1>()");
static_assert(not is_valid_initialization<I1, int>(),
        "is_valid_initialization<I1, int>()");
static_assert(is_valid_initialization<I2>(),
        "is_valid_initialization<I2>()");
static_assert(is_valid_initialization<I2, int>(),
        "is_valid_initialization<I2, int>()");
static_assert(not is_valid_initialization<I2, int, int>(),
        "is_valid_initialization<I2, int, int>()");

static_assert(std::is_same_v<int, type_at<0, int, double>>,
        "std::is_same_v<int, type_at<0, int, double>>");
static_assert(std::is_same_v<double, type_at<1, int, double>>,
        "std::is_same_v<int, type_at<1, int, double>>");
static_assert(std::is_same_v<char, type_at<4, int, double, I1, I2, char>>,
        "std::is_same_v<char, type_at<5, int, double, I1, I2, char>>");
