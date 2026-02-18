#pragma once
#include <atomic>
#include <array>
#include <new>

namespace lob {
	template<typename T, size_t power_of_two>
	class SPSCQueue {
	private:
		static constexpr size_t size = (1ULL << power_of_two);
		static constexpr size_t mask = size - 1;

		alignas(std::hardware_destructive_interference_size)std::array<T, size> arr;
		alignas(std::hardware_destructive_interference_size)std::atomic<size_t> tail;
		alignas(std::hardware_destructive_interference_size)std::atomic<size_t> head;

	public:
		SPSCQueue() : tail(0), head(0){

		}
		bool try_push(const T& value) noexcept {
			const size_t t = tail.load(std::memory_order::acquire);

			if (t - head.load(std::memory_order::acquire) == size) [[unlikely]] {
				return false;
			}

			arr[t & mask] = value;
			tail.fetch_add(1, std::memory_order::release);

			return true;
		}

		bool try_pop(T& value) noexcept {
			const size_t h = head.load(std::memory_order::acquire);

			if (h == tail.load(std::memory_order::acquire)) [[unlikely]] {
				return false;
			}
			value = arr[h&mask];
			head.fetch_add(1, std::memory_order::release);

			return true;
		}

		bool empty() const noexcept {
			return tail.load(std::memory_order::acquire) == head.load(std::memory_order::acquire);
		}

	};
}