#pragma once

#include <bucket/Bucket.h>
#include <bitset/Bitset.h>
#include <array>
#include "absl/container/flat_hash_map.h"
#include <allocators/ObjectPool.h>

namespace lob {
	template<size_t min_price, size_t max_price, size_t bucket_size, size_t arr_size = (max_price - min_price+ bucket_size) / bucket_size>
	class OrderStore {
	private:

		std::array<Bucket, arr_size> buckets;
		Bitset<arr_size> bitset;

		absl::flat_hash_map<uint64_t, Order*> orders_registry;
		
		ObjectPool<Order> pool;

		inline size_t calculateBucket(int64_t price) noexcept {
			return (static_cast<size_t>(price) - min_price) / bucket_size;
		}
	public:
		OrderStore(size_t capacity = 2100000) : pool(capacity){
			orders_registry.reserve(capacity);
		}
		void add(uint64_t id, int64_t price, uint32_t quantity) {

			
			assert(orders_registry.find(id) == orders_registry.end() && "Duplicate Order ID!");

			//Order* order = new Order(id, price, quantity, 0);
			Order* order = pool.allocate();
			order->id = id;
			order->price = price;
			order->quantity = quantity;
			order->timestamp = 0;

			orders_registry.insert({ id, order });

			size_t bucket_id = calculateBucket(price);

			buckets[bucket_id].add(*order);
			bitset.set(bucket_id);
		}

		/*
		* 1. Find and get iterator
		* 2. Get ptr of Order
		* 3. Erase from map
		* 4. Unlink
		* 5. Cheack is bucket empty
		*/
		void cancel(uint64_t id) {
			
			auto it = orders_registry.find(id);
			if (it == orders_registry.end())return;

			Order* order = it->second;

			orders_registry.erase(it);

			size_t bucket_id = calculateBucket(order->price);
			order->unlink();
			if (buckets[bucket_id].empty()) {
				bitset.reset(bucket_id);
			}
			//delete order;
			pool.free(order);
		}
		Order* getCheapest() {
			size_t id = bitset.firstNotZeroBit();
			if (id == BITSET_EMPTY_FLAG_VALUE) {
				return nullptr;
			}
			return &buckets[id].getBestOrder();

		}
		Order* getDearest() {
			size_t id = bitset.lastNotZeroBit();
			if (id == BITSET_EMPTY_FLAG_VALUE) {
				return nullptr;
			}
			return &buckets[id].getWorstOrder();

		}
		Order* get(uint64_t id) {
			auto it = orders_registry.find(id);
			if (it == orders_registry.end())return nullptr;
			return it->second;
		}
		

		
		~OrderStore() {
			for (auto& [id, order] : orders_registry) {
				order->unlink();
				//delete order;
			}
			orders_registry.clear();
		}
	};
}