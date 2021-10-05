#include <containers/UnorderedRingBuffer.hpp>
#include <tests/framework.hpp>
#include <vector>

ADD_TEST(UnorderedRingBufferTests, alwaysMaxSize)
{
    DynaSoft::UnorderedRingBuffer<int, 4> sut{0};

    EXPECT_EQUAL(4, sut.size());
}

ADD_TEST(UnorderedRingBufferTests, push_firstLastIndices)
{
    DynaSoft::UnorderedRingBuffer<int, 4> sut{0};

    EXPECT_EQUAL(0, sut.firstIndex());
    EXPECT_EQUAL(3, sut.lastIndex());

    sut.push(1);

    EXPECT_EQUAL(1, sut.firstIndex());
    EXPECT_EQUAL(0, sut.lastIndex());

    EXPECT_EQUAL(1, sut[0]);

    sut.push(2);
    sut.push(3);
    sut.push(4);

    EXPECT_EQUAL(0, sut.firstIndex());
    EXPECT_EQUAL(3, sut.lastIndex());

    EXPECT_EQUAL(1, sut[0]);
    EXPECT_EQUAL(2, sut[1]);
    EXPECT_EQUAL(3, sut[2]);
    EXPECT_EQUAL(4, sut[3]);

    sut.push(5);

    EXPECT_EQUAL(1, sut.firstIndex());
    EXPECT_EQUAL(0, sut.lastIndex());

    EXPECT_EQUAL(5, sut[0]);
    EXPECT_EQUAL(2, sut[1]);
    EXPECT_EQUAL(3, sut[2]);
    EXPECT_EQUAL(4, sut[3]);
}


ADD_TEST(UnorderedRingBufferTests, nextPrevIndex)
{
    DynaSoft::UnorderedRingBuffer<int, 4> sut{0};

    EXPECT_EQUAL(1, sut.nextIndex(0));
    EXPECT_EQUAL(2, sut.nextIndex(1));
    EXPECT_EQUAL(0, sut.nextIndex(3));

    EXPECT_EQUAL(3, sut.prevIndex(0));
    EXPECT_EQUAL(0, sut.prevIndex(1));
    EXPECT_EQUAL(2, sut.prevIndex(3));
}

ADD_TEST(UnorderedRingBufferTests, foreachOrdered)
{
    DynaSoft::UnorderedRingBuffer<int, 4> sut{0};

    sut.push(1);
    sut.push(2);

    std::vector<int> v{};
    sut.foreachOrdered([&](int x)
    {
        v.push_back(x);
    });

    ASSERT_EQUAL(4, (int)v.size());
    EXPECT_EQUAL(0, v[0]);
    EXPECT_EQUAL(0, v[1]);
    EXPECT_EQUAL(1, v[2]);
    EXPECT_EQUAL(2, v[3]);

    v = {};

    sut.push(3);
    sut.push(4);
    sut.push(5);

    sut.foreachOrdered([&](int x)
    {
        v.push_back(x);
    });

    ASSERT_EQUAL(4, (int)v.size());
    EXPECT_EQUAL(2, v[0]);
    EXPECT_EQUAL(3, v[1]);
    EXPECT_EQUAL(4, v[2]);
    EXPECT_EQUAL(5, v[3]);
}
