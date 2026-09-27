#pragma once
#include <memory>
#include <cstdint>
#include <bit>
#include <limits>
#include <lob/parameters/parameters.hpp>
#include <lob/allocators/FIFO_FreeList.hpp>

namespace lob {
	template<typename T, pool_size_t size, template <typename, pool_size_t> typename FreeList>
	class ObjectPool {
	private:
		static_assert(size >= 1, "an empty pool is not useful");
		static_assert(size < (pool_size_t{1} << (std::numeric_limits<pool_size_t>::digits - 1)),
		              "size + 1 rounded up to a power of two must fit in pool_size_t");

		static constexpr pool_size_t free_list_size =
			std::bit_ceil(static_cast<pool_size_t>(size + 1));

		using free_list_t = FreeList<T*, free_list_size>;

		std::unique_ptr<T[]> pool;
		std::unique_ptr<free_list_t> free_list;
		pool_size_t offset;
	public:
		ObjectPool()
			: pool(std::make_unique<T[]>(size)),
			  free_list(std::make_unique<free_list_t>()),
			  offset(0) {}

		T* allocate() {
			if (offset < size) return &pool[offset++];
			if (!free_list->empty()) return free_list->pop();
			return nullptr;
		}

		void free(T* ptr) {
			free_list->push(ptr);
		}

		T* get(pool_size_t id) const noexcept {
			return &pool[id];
		}

		pool_size_t index_of(const T* ptr) const noexcept {
			return static_cast<pool_size_t>(ptr - pool.get());
		}

		static constexpr pool_size_t capacity() noexcept { return size; }
	};
}
