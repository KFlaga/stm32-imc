#include <tests/framework.hpp>
#include <algorithm>
#include <sstream>

namespace test
{
namespace detail
{
Test dummyTest;
Test* currentTest = &dummyTest;
}

bool TestRunner::run(const std::string& filter)
{
    for(auto& t: allTests)
    {
        if(filter.size() > 0 && t.name.find(filter) == std::string::npos)
        {
            continue;
        }

        std::cout << "[" << t.name << "]\n";
        try
        {
            detail::currentTest = t.test.get();
            t.test->setup();
            t.test->execute();
            t.test->teardown();
        }
        catch(const TestAssertionError&)
        {
            t.test->failTest();
        }
        catch(const std::exception& ex)
        {
            t.test->failTest();
            std::cout << "Exception raised: " << ex.what() << "\n";
        }

        detail::currentTest = &detail::dummyTest;

        if(t.test->isSuccess())
        {
            std::cout << "[" << t.name << "] PASSED\n";
        }
        else
        {
            std::cout << "[" << t.name << "] FAILED\n";
        }
    }
    return std::any_of(allTests.begin(), allTests.end(), [](auto& t) { return !t.test->isSuccess(); });
}

}

void dyna_assert_impl(bool cond, const char* expr, const char* file, int line)
{
    std::stringstream fileLine{};
    fileLine << file << ":" << line;
    ::test::detail::currentTest->assertEqual(true, cond, expr, fileLine.str());
}
