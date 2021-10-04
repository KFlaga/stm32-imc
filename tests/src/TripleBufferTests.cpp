#include <tests/framework.hpp>
#include <containers/TripleBuffer.hpp>
#include <vector>

using namespace DynaSoft;

ADD_TEST(TripleBufferTests, initWithProperBuffers)
{
	TripleBuffer<std::vector<int>> tb
	{
		std::vector<int>{0},
		std::vector<int>{1, 1},
		std::vector<int>{2, 2, 2},
	};

	EXPECT_EQUAL(1u, tb.read().size());
	EXPECT_EQUAL(2u, tb.write().size());
}

ADD_TEST(TripleBufferTests, swaps)
{
	TripleBuffer<std::vector<int>> tb
	{
		std::vector<int>{0},
		std::vector<int>{1, 1},
		std::vector<int>{2, 2, 2},
	};

	tb.swapRead();

	EXPECT_EQUAL(3u, tb.read().size());
	EXPECT_EQUAL(2u, tb.write().size());

	tb.swapRead();

	EXPECT_EQUAL(1u, tb.read().size());
	EXPECT_EQUAL(2u, tb.write().size());

	tb.swapWrite();

	EXPECT_EQUAL(1u, tb.read().size());
	EXPECT_EQUAL(3u, tb.write().size());

	tb.swapRead();
	tb.swapWrite();

	EXPECT_EQUAL(2u, tb.read().size());
	EXPECT_EQUAL(1u, tb.write().size());
}

ADD_TEST(TripleBufferTests, swapsAtomic)
{
	TripleBuffer<std::vector<int>, true> tb
	{
		std::vector<int>{0},
		std::vector<int>{1, 1},
		std::vector<int>{2, 2, 2},
	};

	tb.swapRead();

	EXPECT_EQUAL(3u, tb.read().size());
	EXPECT_EQUAL(2u, tb.write().size());

	tb.swapRead();

	EXPECT_EQUAL(1u, tb.read().size());
	EXPECT_EQUAL(2u, tb.write().size());

	tb.swapWrite();

	EXPECT_EQUAL(1u, tb.read().size());
	EXPECT_EQUAL(3u, tb.write().size());

	tb.swapRead();
	tb.swapWrite();

	EXPECT_EQUAL(2u, tb.read().size());
	EXPECT_EQUAL(1u, tb.write().size());
}
