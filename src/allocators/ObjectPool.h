#pragma once
#include <memory>
#include <vector>

namespace lob {
	template<typename T>
	class ObjectPool {

	private:
		std::unique_ptr<T[]> pool;
		std::vector<T*> free_list;
		size_t offset;
		size_t capacity;
	public:
		ObjectPool(size_t capacity_) : capacity(capacity_), offset(0) {
			pool = std::make_unique< T[]>(capacity);
			free_list.reserve(capacity);
		}
		T* allocate() {
			if (!free_list.empty()) {
				T* res = free_list.back();
				free_list.pop_back();
				return res;
			}
			if (offset >= capacity)return nullptr;
			return &pool[offset++];
		}
		void free(T* ptr) {
			free_list.push_back(ptr);
		}

	};
}