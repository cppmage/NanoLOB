#pragma once
#include <chrono>
#include <cstddef>
#define NOMINMAX

namespace lob {

	static uint64_t getCurrentTimeAsUint64_deprecated() noexcept {
		
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
#define CPU_PAUSE() __asm__ __volatile__("yield" ::: "memory")
#else
#define CPU_PAUSE() __asm__ __volatile__ ("" ::: "memory")
#endif
#else
#define CPU_PAUSE() do {} while (0)
#endif

inline void smart_pause() noexcept {
    CPU_PAUSE();
}


#include <thread>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

static void pin_thread_to_core(int core_id) {
#ifdef _WIN32
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << core_id);
    SetThreadAffinityMask(thread, mask);
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
#endif
}
