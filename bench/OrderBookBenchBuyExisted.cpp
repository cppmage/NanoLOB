#include <benchmark/benchmark.h>
#include "store/OrderStore.h"
#include "output/ConsoleOutput.h"
#include "logger/Logger.h"

#include "OrderBook/OrderBook.h"
#include "TradeEvent/TradeEvent.h"
#include <random>
#include <thread>



template<size_t min_price, size_t max_price>
static void BM_OrderBookRealMatchingV2(benchmark::State& state) {


    //static lob::TradeEventsQueue trade_queue;
    //static lob::OrderBook<min_price, max_price, 1> book(trade_queue);

    std::unique_ptr<lob::TradeEventsQueue> trade_queue_ptr = std::make_unique< lob::TradeEventsQueue>();
    lob::TradeEventsQueue& trade_queue = *trade_queue_ptr;

    std::unique_ptr<lob::OrderBook<min_price, max_price, 1>> book_ptr = std::make_unique<lob::OrderBook<min_price, max_price, 1>>(trade_queue);
    lob::OrderBook<min_price, max_price, 1>& book = *book_ptr;

    lob::WALQueue wal_queue("BENCHV2_WAL_EVENTS.bin");
    lob::WALSnapshotStatsQueue wal_snapshot_queue("BENCHV2_WAL_SNAPSHOT.bin");
    lob::Logger logger(trade_queue, wal_queue, wal_snapshot_queue);
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

    constexpr size_t N = 1000000;
    std::vector<int64_t> buy_prices(N);
    std::vector<int64_t> sell_prices(N);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int64_t> dist_buy(min_price, 3 * max_price / 4);
    std::uniform_int_distribution<int64_t> dist_sell(max_price / 2, max_price - 1);

    for (size_t i = 0; i < N; ++i) {
        buy_prices[i] = dist_buy(gen);
        sell_prices[i] = dist_sell(gen);
    }

    uint64_t id = 1;
    size_t base_price = max_price / 2 - 1;
    for (size_t i = 0; i < N; i++) {
        book.limit_sell(id++, base_price + (i % base_price), 10);
        book.limit_buy(id++, base_price - (i % base_price), 10);
    }

    size_t iter = 0;
    size_t hot_objects = 0;

    for (auto _ : state) {
        size_t idx = iter % N;

        int num0 = book.limit_buy(id++, buy_prices[idx], 5);
        int num1 = book.limit_sell(id++, sell_prices[idx], 5);
        hot_objects += 2;
        benchmark::DoNotOptimize(num0);
        benchmark::DoNotOptimize(num1);

        if (iter % 15 == 0) {
            book.limit_sell(id++, base_price + 1, 10);
            book.limit_buy(id++, base_price - 1, 10);
            hot_objects += 2;
        }

        iter++;

    }

    jt1.request_stop();
    jt2.request_stop();


    state.SetItemsProcessed(hot_objects);
    int i = 0;
}



BENCHMARK(BM_OrderBookRealMatchingV2<1, 10000>)->Iterations(1000000);
BENCHMARK(BM_OrderBookRealMatchingV2<1, 1000000>)->Iterations(1000000);


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



// Deprecated
//BENCHMARK(BM_OrderBookRealMatchingV1)->Iterations(1000000);