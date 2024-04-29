#include <boost/heap/pairing_heap.hpp>
#include <gtest/gtest.h>
#include <lb/logging.hpp>
#include <lb/tcp/selectors.hpp>

struct CounterWrapper {
    lb::tcp::Backend* b;
    std::size_t counter = 0;
    std::size_t id = 0;
};

struct ConnectionsCompare {
    bool operator()(const CounterWrapper& lhs, const CounterWrapper& rhs) const
    {
        return lhs.counter > rhs.counter;
    }
};

TEST(PairingHeap, learnHowItWorks)
{
    boost::heap::pairing_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>> heap;

    auto handle1 = heap.push({nullptr, 0, 1});
    ASSERT_TRUE(heap.top().counter == 0);
    ASSERT_TRUE(heap.top().id == 1);

    auto handle2 = heap.push({nullptr, 0, 2});
    ASSERT_TRUE(heap.top().counter == 0);
    ASSERT_TRUE(heap.top().id == 1);

    (*handle1).counter++;
    heap.increase(handle1);
    ASSERT_TRUE(heap.top().counter == 0);
    ASSERT_TRUE(heap.top().id == 2);

    heap.erase(handle2);
    heap.erase(handle1);
    ASSERT_TRUE(heap.empty());
}