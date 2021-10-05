#include <tests/framework.hpp>

int main(void)
{
	return test::TestRunner::instance().run() ? 0 : 1;
}
