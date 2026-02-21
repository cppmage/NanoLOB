#pragma once
#include <chrono>
#include <thread>
#include <intrin.h>


namespace lob {

    static inline uint64_t get_ticks() noexcept {
        unsigned int aux;
        return __rdtscp(&aux);
    }

    class FastTime {
    public:
        uint64_t ns_per_tsc_factor;
        uint64_t start_tsc;
        uint64_t start_ns;
        static constexpr int shift = 32;
        uint64_t dt_ns;
        uint64_t dt_tsc;

        void calibrateTSC() {
            auto t1 = std::chrono::system_clock::now();
            start_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1.time_since_epoch()).count();
            start_tsc = get_ticks();

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto t2 = std::chrono::system_clock::now();
            uint64_t r2 = get_ticks();

            dt_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
            dt_tsc = r2 - start_tsc;
            ns_per_tsc_factor = (dt_ns << shift) / dt_tsc;
        }

    public:
        FastTime() { calibrateTSC(); }

        inline uint64_t duration_to_ns(uint64_t ticks) const noexcept {
            return (ticks * ns_per_tsc_factor) >> shift;
        }

        inline uint64_t tsc_to_absolute_ns(uint64_t tsc) const noexcept {
            return start_ns + duration_to_ns(tsc - start_tsc);
        }
    };

}