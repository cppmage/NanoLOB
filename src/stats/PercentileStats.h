#pragma once
#include <cstdint>
#include <array>
#include <print>

namespace lob {
    class PercentileStats {

    private:
        static constexpr size_t MAX_NS = 40000; 
        std::array<uint32_t, MAX_NS + 1> buckets;     
        uint64_t overflow_count;            
        uint64_t total_count;

    public:
        PercentileStats() : overflow_count(0), total_count(0){
            buckets.fill(0);
        }

        void update(uint64_t latency_ns) {
            if (latency_ns <= MAX_NS) {
                buckets[latency_ns]++;
            }
            else {
                overflow_count++;
            }
            total_count++;
        }

        uint64_t get_percentile(double percentile) {
            if (total_count == 0) return 0;

            uint64_t target = static_cast<uint64_t>(total_count * percentile);
            uint64_t accumulated = 0;

            for (size_t i = 0; i <= MAX_NS; ++i) {
                accumulated += buckets[i];
                if (accumulated >= target) {
                    return i; 
                }
            }
            return MAX_NS; 
        }

        void print_report(FastTime& time) {
            if (total_count == 0) return;

            uint64_t p50_ticks = get_percentile(0.50);
            uint64_t p50_ns = time.duration_to_ns(p50_ticks);

            uint64_t p90_ticks = get_percentile(0.90);
            uint64_t p90_ns = time.duration_to_ns(p90_ticks);

            uint64_t p99_ticks = get_percentile(0.99);
            uint64_t p99_ns = time.duration_to_ns(p99_ticks);

            uint64_t p999_ticks = get_percentile(0.999);
            uint64_t p999_ns = time.duration_to_ns(p999_ticks);

            std::print("--- Latency Percentiles (N={}) ---\n", total_count);
            std::print("P50 (Median): {:>6} ns\n", p50_ns);
            std::print("P90:          {:>6} ns\n", p90_ns);
            std::print("P99:          {:>6} ns\n", p99_ns);
            std::print("P99.9:        {:>6} ns\n", p999_ns);
            //std::print("Max (Limit):  {:>6} ns\n", (overflow_count > 0 ? ">10000" : std::to_string(get_percentile(1.0))));
            std::print("---------------------------------\n");
        }

        void reset() {
            buckets.fill(0);
            overflow_count = 0;
            total_count = 0;
        }

    };
}