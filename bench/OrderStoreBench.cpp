

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

BENCHMARK(BM_OrderStoreAddCancel);
