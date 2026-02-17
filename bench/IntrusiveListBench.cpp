#include <benchmark/benchmark.h>
#include "skiplist/Skiplist.h"
#include <set>

static void SkipList_Insert_Bench(benchmark::State& state) {
    const int N = 1120000;
    std::vector<test_node> nodes(N);
    for (int i = 0; i < N; ++i) nodes[i].key = rand();

    IntrusiveSkiplist<std::greater<>> list;
    int i = 0;
    
    for (auto _ : state) {
        
        list.insert(&nodes[i++]);
        if(i==200000)state.PauseTiming();
    }

    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(SkipList_Insert_Bench);