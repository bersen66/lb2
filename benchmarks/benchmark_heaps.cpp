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

static void BenchmarkPairingHeapPush(benchmark::State& state)
{
    boost::heap::pairing_heap<int> heap;
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            heap.push(i);
        }
        while (!heap.empty())
        {
            heap.pop();
        }
    }
}

BENCHMARK(BenchmarkPairingHeapPush)->Range(1<<0, 1<<16);

static void BenchmarkFibonacciHeapPush(benchmark::State& state)
{
    boost::heap::fibonacci_heap<int> heap;
    for (auto _ : state)
    {
        for (int i = 0; i < state.range(0); ++i)
        {
            heap.push(i);
        }
        while (!heap.empty())
        {
            heap.pop();
        }
    }
}

BENCHMARK(BenchmarkFibonacciHeapPush)->Range(1<<0, 1<<16);