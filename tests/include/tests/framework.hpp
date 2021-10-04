#pragma once

#include <vector>
#include <memory>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <cmath>
#include <string>
#include <type_traits>

#include <misc/Macro.hpp>

// Simple test framework

namespace test
{

struct TestAssertionError : std::exception
{
    using exception::exception;
};

struct Test
{
    virtual ~Test() {}
    virtual void execute() {};

    virtual void setup() {}
    virtual void teardown() {}

    template<typename T1, typename T2>
    bool expectEqual(T1&& expected, T2&& actual, const std::string& msg = "", const std::string& file = "")
    {
        if(!(expected == actual))
        {
            onFailedExpectation(expected, actual, msg, file);
            return false;
        }
        return true;
    }

    template<typename T1, typename T2>
    void assertEqual(T1&& expected, T2&& actual, const std::string& msg = "", const std::string& file = "")
    {
        if(!expectEqual(std::forward<T1>(expected), std::forward<T2>(actual), msg, file))
        {
            throw TestAssertionError{};
        }
    }

    template<typename T1, typename T2>
    bool expectNotEqual(T1&& expected, T2&& actual, const std::string& msg = "", const std::string& file = "")
    {
        if(expected == actual)
        {
            onFailedExpectation(expected, actual, msg, file);
            return false;
        }
        return true;
    }

    template<typename T1, typename T2>
    void assertNotEqual(T1&& expected, T2&& actual, const std::string& msg = "", const std::string& file = "")
    {
        if(!expectNotEqual(std::forward<T1>(expected), std::forward<T2>(actual), msg, file))
        {
            throw TestAssertionError{};
        }
    }

    bool isSuccess() const
    {
    	return testSuccessed;
    }

    void failTest()
    {
    	testSuccessed = false;
    }

private:
    template<typename T1, typename T2>
    void onFailedExpectation(const T1& expected, const T2& actual, const std::string& msg, const std::string& file)
    {
    	testSuccessed = false;
        if(file.size() > 0)
        {
            std::cout << "In " << file << "\n";
        }
        std::cout << "    Expectation not met " << msg << "\n";
        if constexpr(std::is_same_v<T1, std::uint8_t>)
		{
            std::cout << "    Expected: " << (int)expected << "\n";
		}
        else
        {
            std::cout << "    Expected: " << expected << "\n";
        }
        if constexpr(std::is_same_v<T2, std::uint8_t>)
		{
            std::cout << "    Actual: " << (int)actual << "\n";
		}
        else
        {
            std::cout << "    Actual: " << actual << "\n";
        }
    }

    bool testSuccessed = true;
};

namespace detail
{
extern Test* currentTest;
}

class TestRunner
{
public:
    static TestRunner& instance()
    {
        static TestRunner instance_{};
        return instance_;
    }

    template<typename TestT>
    int addTest(const std::string& name)
    {
        allTests.push_back({std::make_unique<TestT>(), name});
        return 0;
    }

    void run(const std::string& filter = "");

private:
    TestRunner() {}

    struct TestInfo
    {
        std::unique_ptr<Test> test;
        std::string name;
    };

    std::vector<TestInfo> allTests;
};

}


template<typename T>
struct greater_equal
{
    T value;
    greater_equal(T v) : value{v} {};
};

template<typename T>
bool operator==(const greater_equal<T>& ex, const T& x)
{
    return x >= ex.value;
}

template<typename T>
bool operator==(const T& x, const greater_equal<T>& ex)
{
    return x >= ex.value;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const greater_equal<T>& x)
{
    out << "x >= " << x.value;
    return out;
}

template<typename T>
struct with_accuracy_t
{
    T value;
    T accuracy;
    with_accuracy_t(T v, T a) : value{v}, accuracy{a} {};
};

template<typename T>
auto with_accuracy(T v, T a)
{
    return with_accuracy_t<T>{v, a};
}

template<typename T>
bool operator==(const with_accuracy_t<T>& ex, const T& x)
{
    return std::abs(ex.value - x) <= ex.accuracy;
}

template<typename T>
bool operator==(const T& x, const with_accuracy_t<T>& ex)
{
    return std::abs(ex.value - x) <= ex.accuracy;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const with_accuracy_t<T>& x)
{
    out << "|x - " << x.value << "| <= " << x.accuracy;
    return out;
}

inline std::ostream& operator<<(std::ostream& out, std::nullptr_t)
{
    return out << "nullptr";
}

#define GET_TEST_NAME(cls, tst) MACRO_CONCAT3(_class_, cls, tst)
#define GET_TOKEN_NAME(cls, tst) MACRO_CONCAT3(_token_, cls, tst)

#define ADD_TEST_IMPL(cls, tst, inherit) \
    struct GET_TEST_NAME(cls, tst) : inherit \
    { \
        void execute(); \
    }; \
    int GET_TOKEN_NAME(cls, tst) = ::test::TestRunner::instance().addTest<GET_TEST_NAME(cls, tst)>(#cls "::" #tst); \
    void GET_TEST_NAME(cls, tst)::execute()


#define ADD_TEST_F(fixture_class, test_name) ADD_TEST_IMPL(fixture_class, test_name, fixture_class)

#define ADD_TEST(test_group, test_name)  ADD_TEST_IMPL(test_group, test_name, ::test::Test)


#define STRINGIFY_1(x) #x
#define STRINGIFY_2(x) STRINGIFY_1(x)
#define FILE_LINE() __FILE__ ":" STRINGIFY_2(__LINE__)

#define EXPECT_EQUAL(a, b) ::test::detail::currentTest->expectEqual(a, b, #a " == " #b, FILE_LINE())
#define ASSERT_EQUAL(a, b) ::test::detail::currentTest->assertEqual(a, b, #a " == " #b, FILE_LINE())
#define EXPECT_NOT_EQUAL(a, b) ::test::detail::currentTest->expectNotEqual(a, b, #a " != " #b, FILE_LINE())
#define ASSERT_NOT_EQUAL(a, b) ::test::detail::currentTest->assertNotEqual(a, b, #a " != " #b, FILE_LINE())

#define EXPECT_TRUE(cond) ::test::detail::currentTest->expectEqual(true, cond, #cond " is true", FILE_LINE())
#define ASSERT_TRUE(cond) ::test::detail::currentTest->assertEqual(true, cond, #cond " is true", FILE_LINE())

#define EXPECT_FALSE(cond) ::test::detail::currentTest->expectEqual(false, cond, #cond " is false", FILE_LINE())
#define ASSERT_FALSE(cond) ::test::detail::currentTest->assertEqual(false, cond, #cond " is false", FILE_LINE())

#define EXPECT_ENUM(a, b) ::test::detail::currentTest->expectEqual((int)a, (int)b, #a " == " #b, FILE_LINE())
#define ASSERT_ENUM(a, b) ::test::detail::currentTest->assertEqual((int)a, (int)b, #a " == " #b, FILE_LINE())

#define EXPECT_EQUAL_EXT(a, b, FL) ::test::detail::currentTest->expectEqual(a, b, #a " == " #b, FL)
#define ASSERT_EQUAL_EXT(a, b, FL) ::test::detail::currentTest->assertEqual(a, b, #a " == " #b, FL)
