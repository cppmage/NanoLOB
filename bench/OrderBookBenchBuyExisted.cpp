#include <benchmark/benchmark.h>
#include "store/OrderStore.h"
#include "output/ConsoleOutput.h"
#include "logger/Logger.h"

#include "OrderBook/OrderBook.h"
#include "TradeEvent/TradeEvent.h"
#include <random>
#include <thread>



template<typename queue_t = lob::TradeEventsQueue>
void consumer(std::stop_token stoken, queue_t& queue) {
    int value = 0;
    lob::TradeEvent event;
    while (!stoken.stop_requested()) {
        queue.try_pop(event);
    }
}

lob::TradeEventsQueue queue;
static void BM_OrderBookRealMatchingV1(benchmark::State& state) {
    
    std::jthread jtc(consumer<lob::TradeEventsQueue>, std::ref(queue));

    lob::OrderBook<0, 1000000, 100> book(queue);

    const size_t N = 1000000;
    std::vector<int64_t> prices(N);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int64_t> dist(100, 999900);
    for (auto& p : prices) p = dist(gen);

    uint64_t id = 1;

    for (int i = 0; i < 10000; i++) {
        book.limit_sell(id++, dist(gen), 10);
    }

    size_t i = 0;
    for (auto _ : state) {
        int64_t p = prices[i % N];

        book.limit_buy(id, p, 5);

        book.cancel_buy(id);
        

        id++;
        i++;

        if (i % 10 == 0) {
            book.limit_sell(id++, dist(gen), 10);
        }
    }

    state.SetItemsProcessed(state.iterations());
}
    

#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

void pin_thread_to_core(int core_id) {
#ifdef _WIN32
    // Windows: устанавливаем маску (1 << core_id)
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << core_id);
    SetThreadAffinityMask(thread, mask);
#else
    // Linux: используем cpu_set_t
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
#endif
}





static lob::TradeEventsQueue queue1;

static lob::OrderBook<0, 1000000, 1> book(queue1);

static void BM_OrderBookRealMatchingV2(benchmark::State& state) {
    
    
    lob::WALQueue wal_queue("BENCHV2_WAL_EVENTS.bin");
    lob::WALSnapshotStatsQueue wal_snapshot_queue("BENCHV2_WAL_SNAPSHOT.bin");
    lob::Logger logger(queue1, wal_queue, wal_snapshot_queue);
    lob::ConsoleOutput output(wal_queue);

    std::jthread jt1([&logger](std::stop_token st) {
        pin_thread_to_core(2);
        logger.process(st);
        });
    std::jthread jt2([&output](std::stop_token st) {
        pin_thread_to_core(1);
        output.process(st);
        });

    pin_thread_to_core(0);

    const size_t N = 1000000;
    std::vector<int64_t> buy_prices(N);
    std::vector<int64_t> sell_prices(N);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int64_t> dist_buy(50, 500000);
    std::uniform_int_distribution<int64_t> dist_sell(500001, 99999);

    for (size_t i = 0; i < N; ++i) {
        buy_prices[i] = dist_buy(gen);
        sell_prices[i] = dist_sell(gen);
    }

    uint64_t id = 1;
    int base_price = 500000;
    for (int i = 0; i < 1000000; i++) {
        book.limit_sell(id++, base_price + (i % base_price), 10);
        book.limit_buy(id++, base_price - (i % base_price), 10);
    }

    size_t iter = 0;
    // 2. ГОРЯЧИЙ ЦИКЛ (Без пауз и рандома)
    for (auto _ : state) {
        size_t idx = iter % N;

        int num = book.limit_buy(id++, buy_prices[idx], 5);
        benchmark::DoNotOptimize(num);
        

        if (iter % 15 == 0) {   
            book.limit_sell(id++, 501, 10);
            book.limit_buy(id++, 499, 10);
        }

        iter++;
        
    }

    jt1.request_stop();
    jt2.request_stop();


    state.SetItemsProcessed(state.iterations());
    int i = 0;
}


//BENCHMARK(BM_OrderBookRealMatchingV1)->Iterations(1000000);
BENCHMARK(BM_OrderBookRealMatchingV2)->Iterations(1000000);