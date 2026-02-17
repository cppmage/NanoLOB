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
}

static void BM_OrderBookRealMatchingV2(benchmark::State& state) {
    lob::OrderBook<0, 1000000, 100> book;
    uint64_t id = 1;
    // Заполняем обе стороны
    for (int i = 0; i < 10000; i++) {
        book.limit_sell(id++, 500 + rand() % 100, 10);
        book.limit_buy(id++, 500 - rand() % 100, 10);
    }

    for (auto _ : state) {
        // Покупаем по рыночной цене (должен матчиться)
        state.PauseTiming();
        int64_t price = 400 + rand() % 100; // центральная цена
        state.ResumeTiming();
        book.limit_buy(id++,  price,5); // купить 5 лотов

        state.PauseTiming();
        if (id++ % 15 == 0) {
            state.ResumeTiming();
            book.limit_sell(id++, 501, 10);
            book.limit_buy(id++, 499, 10);
            state.PauseTiming();
        }
        state.ResumeTiming();
    }
}

//BENCHMARK(BM_OrderBookRealMatchingV1);
//BENCHMARK(BM_OrderBookRealMatchingV2);