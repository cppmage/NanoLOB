#pragma once
#include <cstdint>
#include <array>
#include <print>
#include <new>
#include <cstdio>

namespace lob {
    static constexpr const char* percentile_output_sample = "--- Latency Percentiles (N={}) ---\n"
        "P50 (Median): {:>6} ns\n"
        "P90:          {:>6} ns\n"
        "P99:          {:>6} ns\n"
        "P99.9:        {:>6} ns\n"
        "Max (Limit):  {:>6} ns\n"
        "Overflowed:   {:>6} ts\n"
        "---------------------------------\n";

    class PercentileStats {

    private:
        
        static constexpr size_t MAX_NS = 40000; 
        alignas(std::hardware_constructive_interference_size) std::array<uint32_t, MAX_NS + 1> buckets;
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
            if (total_count == 0)return;
            uint64_t p50_ticks = get_percentile(0.50);
            uint64_t p50_ns = time.duration_to_ns(p50_ticks);

            uint64_t p90_ticks = get_percentile(0.90);
            uint64_t p90_ns = time.duration_to_ns(p90_ticks);

            uint64_t p99_ticks = get_percentile(0.99);
            uint64_t p99_ns = time.duration_to_ns(p99_ticks);

            uint64_t p999_ticks = get_percentile(0.999);
            uint64_t p999_ns = time.duration_to_ns(p999_ticks);

            uint64_t p100_ticks = get_percentile(1.0);
            uint64_t p100_ns = time.duration_to_ns(p100_ticks);
            
            std::print(
                percentile_output_sample,
                total_count,
                p50_ns,
                p90_ns,
                p99_ns,
                p999_ns,
                (overflow_count > 0 ? MAX_NS : p100_ns),
                overflow_count
            );
        }
        void print_report(FastTime& time, FILE* file) {
            if (total_count == 0)return;
            uint64_t p50_ticks = get_percentile(0.50);
            uint64_t p50_ns = time.duration_to_ns(p50_ticks);

            uint64_t p90_ticks = get_percentile(0.90);
            uint64_t p90_ns = time.duration_to_ns(p90_ticks);

            uint64_t p99_ticks = get_percentile(0.99);
            uint64_t p99_ns = time.duration_to_ns(p99_ticks);

            uint64_t p999_ticks = get_percentile(0.999);
            uint64_t p999_ns = time.duration_to_ns(p999_ticks);

            uint64_t p100_ticks = get_percentile(1.0);
            uint64_t p100_ns = time.duration_to_ns(p100_ticks);

            std::print(file,
                percentile_output_sample,
                total_count,
                p50_ns,
                p90_ns,
                p99_ns,
                p999_ns,
                (overflow_count > 0 ? MAX_NS : p100_ns),
                overflow_count
            );
        }

        void reset() {
            buckets.fill(0);
            overflow_count = 0;
            total_count = 0;
        }

    };
}