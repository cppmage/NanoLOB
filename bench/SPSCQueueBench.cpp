#include <benchmark/benchmark.h>
#include "lockfree/SPSCQueue.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "other/Other.h"

using namespace lob;



using QueueSmall = SPSCQueue<int, 4>;   // 16 
using QueueMedium = SPSCQueue<int, 8>;  // 256 
using QueueLarge = SPSCQueue<int, 12>;  // 4096 
using QueueHuge = SPSCQueue<int, 16>;   // 65536 

// Sngle thread
template<typename QueueType>
void BM_Push(benchmark::State& state) {
    QueueType queue;
    int value = 42;

    for (auto _ : state) {
        queue.try_push(value);
        benchmark::DoNotOptimize(queue);
        ++value;
    }
    state.SetItemsProcessed(state.iterations());
}


template<typename QueueType>
void BM_Pop(benchmark::State& state) {
    QueueType queue;
    int value;

    for (int i = 0; i < 100000; ++i) {
        queue.try_push(i);
    }

    for (auto _ : state) {
        if (queue.try_pop(value)) {
            benchmark::DoNotOptimize(value);
            queue.try_push(value);
        }
    }
    state.SetItemsProcessed(state.iterations());
}

template<typename QueueType>
void BM_PushPop(benchmark::State& state) {
    QueueType queue;
    int value = 42;

    for (auto _ : state) {
        queue.try_push(value);
        queue.try_pop(value);
        benchmark::DoNotOptimize(value);
        ++value;
    }
    state.SetItemsProcessed(state.iterations());
}

template<typename QueueType>
void BM_PushPopWithWork(benchmark::State& state) {
    QueueType queue;
    int value = 42;
    int work_delay = state.range(0);

    for (auto _ : state) {
        queue.try_push(value);

        for (int i = 0; i < work_delay; ++i) {
            benchmark::DoNotOptimize(i);
        }

        queue.try_pop(value);
        benchmark::DoNotOptimize(value);
        ++value;
    }
    state.SetItemsProcessed(state.iterations());
    state.SetLabel(std::to_string(work_delay).c_str());
}

// Multi thread benchs
template<typename QueueType>
void BM_SPSC(benchmark::State& state) {
    const int items_per_iteration = 500000;

    for (auto _ : state) {
        QueueType queue;
        std::atomic<bool> start{ false };
        std::atomic<long long> sum_produced{ 0 };
        std::atomic<long long> sum_consumed{ 0 };

        std::jthread producer([&] {
            while (!start) {}

            long long local_sum = 0;
            for (int i = 0; i < items_per_iteration; ++i) {
                while (!queue.try_push(i)) {
                    smart_pause();
                }
                local_sum += i;
            }
            sum_produced += local_sum;
            });


        std::jthread consumer([&] {
            while (!start) {}

            long long local_sum = 0;
            int value;
            int consumed = 0;

            while (consumed < items_per_iteration) {
                if (queue.try_pop(value)) {
                    local_sum += value;
                    ++consumed;
                }
                else {
                    smart_pause();
                }
            }
            sum_consumed += local_sum;
            });

        start = true;

        producer.join();
        consumer.join();

        if (sum_produced.load() != sum_consumed.load()) {
            state.SkipWithError("Sum mismatch!");
        }
    }

    state.SetItemsProcessed(state.iterations() * items_per_iteration);
}

template<typename QueueType>
void BM_SPSC_Imbalanced(benchmark::State& state) {
    const int items_per_iteration = 200000;
    const int producer_delay = state.range(0);
    const int consumer_delay = state.range(1);

    for (auto _ : state) {
        QueueType queue;
        std::atomic<bool> start{ false };
        std::atomic<long long> sum_produced{ 0 };
        std::atomic<long long> sum_consumed{ 0 };

        std::jthread producer([&] {
            while (!start) {}

            long long local_sum = 0;
            for (int i = 0; i < items_per_iteration; ++i) {
                for (int d = 0; d < producer_delay; ++d) {
                    benchmark::DoNotOptimize(d);
                }

                while (!queue.try_push(i)) {
                    smart_pause();
                }
                local_sum += i;
            }
            sum_produced += local_sum;
            });

        std::jthread consumer([&] {
            while (!start) {}

            long long local_sum = 0;
            int value;
            int consumed = 0;

            while (consumed < items_per_iteration) {
                if (queue.try_pop(value)) {
                    for (int d = 0; d < consumer_delay; ++d) {
                        benchmark::DoNotOptimize(d);
                    }
                    local_sum += value;
                    ++consumed;
                }
                else {
                    smart_pause();
                }
            }
            sum_consumed += local_sum;
            });

        start = true;

        producer.join();
        consumer.join();

        if (sum_produced.load() != sum_consumed.load()) {
            state.SkipWithError("Sum mismatch!");
        }
    }

    state.SetItemsProcessed(state.iterations() * items_per_iteration);
}

void BM_MultipleSPSC(benchmark::State& state) {
    const int num_queues = state.range(0);
    const int items_per_queue = 100000;

    for (auto _ : state) {
        std::vector<QueueMedium> queues(num_queues);
        std::atomic<bool> start{ false };
        std::atomic<long long> total_produced{ 0 };
        std::atomic<long long> total_consumed{ 0 };

        std::vector<std::jthread> producers;
        std::vector<std::jthread> consumers;

        for (int q = 0; q < num_queues; ++q) {
            producers.emplace_back([&, q] {
                while (!start) {}

                long long local_sum = 0;
                for (int i = 0; i < items_per_queue; ++i) {
                    while (!queues[q].try_push(i)) {
                        smart_pause();
                    }
                    local_sum += i;
                }
                total_produced += local_sum;
                });

            consumers.emplace_back([&, q] {
                while (!start) {}

                long long local_sum = 0;
                int value;
                int consumed = 0;

                while (consumed < items_per_queue) {
                    if (queues[q].try_pop(value)) {
                        local_sum += value;
                        ++consumed;
                    }
                    else {
                        smart_pause();
                    }
                }
                total_consumed += local_sum;
                });
        }

        start = true;

        for (auto& p : producers) p.join();
        for (auto& c : consumers) c.join();

        if (total_produced.load() != total_consumed.load()) {
            state.SkipWithError("Sum mismatch!");
        }
    }

    state.SetItemsProcessed(state.iterations() * items_per_queue * num_queues);
}

// Latency

template<typename QueueType>
void BM_RoundTripLatency(benchmark::State& state) {
    QueueType queue;
    std::atomic<bool> done{ false };

    std::jthread consumer([&] {
        int value;
        while (!done) {
            if (queue.try_pop(value)) {
                while (!queue.try_push(value)) {
                    smart_pause();
                }
            }
        }
        });

    int value = 42;
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();

        while (!queue.try_push(value)) {
            smart_pause();
        }

        while (!queue.try_pop(value)) {
            smart_pause();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        state.SetIterationTime(elapsed.count() / 1e9);

        ++value;
    }

    done = true;
    consumer.join();
}

// Compare with MutexQueue

#include <mutex>
#include <queue>
#include <condition_variable>

template<typename T>
class MutexQueue {
    std::queue<T> queue;
    mutable std::mutex mtx;
public:
    bool try_push(const T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(value);
        return true;
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        value = queue.front();
        queue.pop();
        return true;
    }
};

void BM_CompareWithStdQueue(benchmark::State& state) {
    const int mode = state.range(0);  

    if (mode == 0) {
        // SPSCQueue
        QueueLarge queue;
        int value = 42;
        for (auto _ : state) {
            queue.try_push(value);
            queue.try_pop(value);
            ++value;
        }
    }
    else {
        // MutexQueue
        MutexQueue<int> queue;
        int value = 42;
        for (auto _ : state) {
            queue.try_push(value);
            queue.try_pop(value);
            ++value;
        }
    }

    state.SetItemsProcessed(state.iterations());
}

// Benchs registration
#ifdef queue_check


BENCHMARK_TEMPLATE(BM_Push, QueueSmall)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Push, QueueMedium)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Push, QueueLarge)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Push, QueueHuge)->Threads(1)->Iterations(1000000);

BENCHMARK_TEMPLATE(BM_Pop, QueueSmall)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Pop, QueueMedium)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Pop, QueueLarge)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_Pop, QueueHuge)->Threads(1)->Iterations(1000000);

BENCHMARK_TEMPLATE(BM_PushPop, QueueSmall)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_PushPop, QueueMedium)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_PushPop, QueueLarge)->Threads(1)->Iterations(1000000);
BENCHMARK_TEMPLATE(BM_PushPop, QueueHuge)->Threads(1)->Iterations(1000000);


BENCHMARK_TEMPLATE(BM_PushPopWithWork, QueueLarge)
->Args({ 0 })
->Args({ 10 })
->Args({ 100 })
->Args({ 1000 })
->Iterations(100000);


BENCHMARK_TEMPLATE(BM_SPSC, QueueMedium)->Iterations(5)->UseRealTime();
BENCHMARK_TEMPLATE(BM_SPSC, QueueLarge)->Iterations(5)->UseRealTime();
BENCHMARK_TEMPLATE(BM_SPSC, QueueHuge)->Iterations(5)->UseRealTime();


BENCHMARK_TEMPLATE(BM_SPSC_Imbalanced, QueueLarge)
->Args({ 0, 0 })      
->Args({ 10, 0 })     
->Args({ 0, 10 })     
->Args({ 100, 10 })   
->Iterations(3);

BENCHMARK(BM_MultipleSPSC)
->Arg(1)->Arg(2)->Arg(4)->Arg(8)->Arg(16)
->Iterations(3)->UseRealTime();

BENCHMARK_TEMPLATE(BM_RoundTripLatency, QueueMedium)
->Iterations(10000)
->UseManualTime();

BENCHMARK(BM_CompareWithStdQueue)
->Arg(0)  
->Arg(1)  
->Iterations(1000000);

BENCHMARK_MAIN();
#endif