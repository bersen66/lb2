#include <benchmark/benchmark.h>
#include <boost/heap/fibonacci_heap.hpp>
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

static void BenchmarkFibbanaciHeap(benchmark::State& state)
{

    for (auto _ : state)
    {
        boost::heap::fibonacci_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>> heap;
        auto handle1 = heap.push({nullptr, 0, 1});
        auto handle2 = heap.push({nullptr, 0, 2});
        (*handle1).counter++;
        heap.increase(handle1);
        heap.erase(handle2);
        heap.erase(handle1);
    }
}

BENCHMARK(BenchmarkFibbanaciHeap);

static void BenchmarkPairingHeap(benchmark::State& state)
{

    for (auto _ : state)
    {
        boost::heap::pairing_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>> heap;
        auto handle1 = heap.push({nullptr, 0, 1});
        auto handle2 = heap.push({nullptr, 0, 2});
        (*handle1).counter++;
        heap.increase(handle1);
        heap.erase(handle2);
        heap.erase(handle1);
    }
}

BENCHMARK(BenchmarkPairingHeap);



template<typename HeapType>
void RunIncreaseDecreaseOperations(HeapType& heap,
                                   std::vector<typename HeapType::handle_type>& handles)
{
    for (int i = 0; i < handles.size(); ++i)
    {
        (*handles[i]).counter++;
        heap.increase(handles[i]);
        std::size_t num = (i + 10) % handles.size();
        (*handles[num]).counter--;
        heap.decrease(handles[num]);
    }
}

static void BenchmarkFibID(benchmark::State& state)
{
    boost::heap::fibonacci_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>> heap;
    using HandleType = boost::heap::fibonacci_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>>::handle_type;

    std::vector<HandleType> handles;
    for (std::size_t i = 0; i < 1000; ++i)
    {
        handles.push_back(heap.push(CounterWrapper{.b = nullptr, .counter = 0, .id = i}));
    }

    for (auto _ : state)
    {
        RunIncreaseDecreaseOperations(heap, handles);
    }
}

BENCHMARK(BenchmarkFibID);

static void BenchmarkPairID(benchmark::State& state)
{
    boost::heap::pairing_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>> heap;
    using HandleType = boost::heap::pairing_heap<CounterWrapper, boost::heap::compare<ConnectionsCompare>>::handle_type;

    std::vector<HandleType> handles;
    for (std::size_t i = 0; i < 1000; ++i)
    {
        handles.push_back(heap.push(CounterWrapper{.b = nullptr, .counter = 0, .id = i}));
    }

    for (auto _ : state)
    {
        RunIncreaseDecreaseOperations(heap, handles);
    }
}

BENCHMARK(BenchmarkPairID);