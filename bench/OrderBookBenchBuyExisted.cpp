#include <benchmark/benchmark.h>
#include "store/OrderStore.h"
#include "OrderBook/OrderBook.h"
#include <random>


static void BM_OrderBookRealMatchingV1(benchmark::State& state) {
    lob::OrderBook<0, 1000000, 100> book;

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

static lob::OrderBook<0, 1000000, 1> book;
static void BM_OrderBookRealMatchingV2(benchmark::State& state) {


    const size_t N = 1000000;
    std::vector<int64_t> buy_prices(N);
    std::vector<int64_t> sell_prices(N);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int64_t> dist_buy(400, 500);
    std::uniform_int_distribution<int64_t> dist_sell(501, 600);

    for (size_t i = 0; i < N; ++i) {
        buy_prices[i] = dist_buy(gen);
        sell_prices[i] = dist_sell(gen);
    }

    uint64_t id = 1;
    for (int i = 0; i < 1000000; i++) {
        book.limit_sell(id++, 100000 + (i % 100000), 10);
        book.limit_buy(id++, 100000 - (i % 100000), 10);
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

    state.SetItemsProcessed(state.iterations());
}


BENCHMARK(BM_OrderBookRealMatchingV1)->Iterations(1000000);
BENCHMARK(BM_OrderBookRealMatchingV2)->Iterations(1000000);