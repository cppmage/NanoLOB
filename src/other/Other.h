#pragma once
#include <chrono>


namespace lob {

	static uint64_t getCurrentTimeAsUint64() noexcept {
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
	}

}

#if defined(_MSC_VER)
#include <intrin.h>
#define CPU_PAUSE() _mm_pause()
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define CPU_PAUSE() _mm_pause()
#elif defined(__arm__) || defined(__aarch64__)
// Для ARM используем yield
#define CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define CPU_PAUSE() __asm__ __volatile__ ("" ::: "memory")
#endif
#else
#define CPU_PAUSE() do {} while (0)
#endif

// Использование в коде:
inline void smart_pause() noexcept {
    CPU_PAUSE();
}