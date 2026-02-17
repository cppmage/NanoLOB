

#include <benchmark/benchmark.h>
#include "store/OrderStore.h"
#include "OrderBook/OrderBook.h"
#include <random>

static void BM_OrderStoreAddCancel(benchmark::State& state) {
    lob::OrderStore<100, 1000, 1> store;
    uint64_t id = 1;

    for (auto _ : state) {
        state.PauseTiming(); 
        int64_t price = 500;
        state.ResumeTiming();

        store.add(id, price, 100);
        auto* order = store.get(id);
        benchmark::DoNotOptimize(order); 
        store.cancel(id);

        id++;
    }
    state.SetItemsProcessed(state.iterations());
}

static lob::OrderBook<0, 1000000, 100> book;
static void BM_OrderBookMatching(benchmark::State& state) {
    

    // 1. Пре-генерация данных (чтобы не тратить время в цикле)
    const size_t test_size = 100000;
    std::vector<int64_t> prices(test_size);
    std::mt19937 gen(42);
    std::uniform_int_distribution<int64_t> dist(1, 90000); // Чтобы не вылетать за границы
    for (auto& p : prices) p = dist(gen);

    uint64_t id = 1;
    size_t i = 0;

    for (auto _ : state) {
        // Берем заранее подготовленные данные
        int64_t p = prices[i % test_size];

        // Чередуем BUY и SELL для создания матчинга
        if (i % 2 == 0) {
            book.limit_buy(id++, p, 10);
            book.cancel_buy(id - 1);
        }
        else {
            book.limit_sell(id++, p, 10);
            book.cancel_sell(id - 1);
        }

        i++;

    }
}


// Запуск теста
//BENCHMARK(BM_OrderStoreAddCancel);

//BENCHMARK(BM_OrderBookMatching);