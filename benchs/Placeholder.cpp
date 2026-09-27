// Placeholder while the library is being refactored.
// Real benchmarks live in benchs/legacy/ and are not compiled yet.
#include <benchmark/benchmark.h>

static void BM_Placeholder(benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(0);
    }
}
BENCHMARK(BM_Placeholder);
