#pragma once
#include <cstdint>
#include <TradeEvent/TradeEvent.h>
#include <print>
#include <time/Time.h>
#include <cstdio>

namespace lob {
    static constexpr const char* common_output_stats_sample = "Factor: {}, CPU GHz: {:.6f}\n"
        "--- Performance Report (last {} trades) ---\n"
        "Avg Match:   {:>6} ns\n"
        "Avg Stall:   {:>6} ns (Queue Wait)\n"
        "Avg E2E:     {:>6} ns (Total Path)\n"
        "Avg Jitter:  {:>6} ns\n"
        "-------------------------------------------\n";

    class CommonStats {

    private:
        


        uint64_t total_match_dt = 0;
        uint64_t total_queue_dt = 0;
        uint64_t total_e2e_dt = 0;
        uint64_t count = 0;

        uint64_t last_e2e = 0;
        uint64_t total_jitter = 0;


    public:

        void update(const TradeEvent& e, uint64_t t_log) {
            total_match_dt += e.dt_match;
            total_queue_dt += e.dt_queue;

            uint64_t current_e2e = t_log - e.t_entry;
            total_e2e_dt += current_e2e;

            if (count > 0) {
                total_jitter += (current_e2e > last_e2e) ? (current_e2e - last_e2e) : (last_e2e - current_e2e);
            }

            last_e2e = current_e2e;
            count++;
        }

        void print_report(const FastTime& time, FILE* file) {
            if (count == 0)return;
            uint64_t avg_match_ticks = total_match_dt / count;
            uint64_t avg_stall_ticks = total_queue_dt / count;
            uint64_t avg_e2e_ticks = total_e2e_dt / count;
            uint64_t avg_jitter_ticks = total_jitter / count;

            std::print(file,
                common_output_stats_sample,
                time.ns_per_tsc_factor,
                static_cast<double>(time.dt_tsc) / time.dt_ns,
                count,
                time.duration_to_ns(avg_match_ticks),
                time.duration_to_ns(avg_stall_ticks),
                time.duration_to_ns(avg_e2e_ticks),
                time.duration_to_ns(avg_jitter_ticks)
            );
        }
        void print_report(const FastTime& time) {
            if (count == 0)return;
            uint64_t avg_match_ticks = total_match_dt / count;
            uint64_t avg_stall_ticks = total_queue_dt / count;
            uint64_t avg_e2e_ticks = total_e2e_dt / count;
            uint64_t avg_jitter_ticks = total_jitter / count;

            std::print(
                common_output_stats_sample,
                time.ns_per_tsc_factor,
                static_cast<double>(time.dt_tsc) / time.dt_ns,
                count,
                time.duration_to_ns(avg_match_ticks),
                time.duration_to_ns(avg_stall_ticks),
                time.duration_to_ns(avg_e2e_ticks),
                time.duration_to_ns(avg_jitter_ticks)
            );
        }


        void reset() {
            total_match_dt = 0;
            total_queue_dt = 0;
            total_e2e_dt = 0;
            total_jitter = 0;
            count = 0;
            last_e2e = 0;
        }

    };
}