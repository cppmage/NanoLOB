#pragma once
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <lob/parameters/parameters.hpp>
#include <lob/bucket/Bucket.hpp>
#include <lob/bitset/HierarchicalBitset.hpp>
#include <lob/allocators/ObjectPool.hpp>
#include <lob/allocators/FIFO_FreeList.hpp>

namespace lob {


	template<pool_size_t number_of_orders, std::size_t min_price, std::size_t max_price, std::size_t bucket_size>
	class OrderStore {
	private:
		static_assert(number_of_orders <= max_orders_per_lob, "too many orders");
		static_assert(bucket_size > 0, "bucket_size must be positive");
		static_assert(min_price <= max_price, "empty price range");

		static constexpr std::size_t arr_size = (max_price - min_price + bucket_size) / bucket_size;

		using pool_t = ObjectPool<Order, number_of_orders, FIFO_FreeList>;

		std::array<Bucket, arr_size> buckets;
		HierarchicalBitset<arr_size, 1> bitset;

		pool_t pool;

		static bool inRange(int64_t price) noexcept {
			return price >= static_cast<int64_t>(min_price) &&
			       price <= static_cast<int64_t>(max_price);
		}

		static std::size_t calculateBucket(int64_t price) noexcept {
			return (static_cast<std::size_t>(price) - min_price) / bucket_size;
		}
	public:
		OrderStore() = default;

		uint64_t add(int64_t price, uint32_t quantity) noexcept {
			if (!inRange(price)) {
				return invalid_order_id;
			}

			Order* order = pool.allocate();
			if (order == nullptr) {
				return invalid_order_id;
			}

			order->id.position = pool.index_of(order);
			order->is_active = true;
			order->price = price;
			order->quantity = quantity;
			order->executed_qty = 0;
			order->timestamp = 0;

			uint64_t id = static_cast<uint64_t>(order->id);

			std::size_t bucket_id = calculateBucket(price);

			buckets[bucket_id].add(*order);
			bitset.set(bucket_id);

			return id;
		}

		/*
		* 1. Find and get iterator
		* 2. Get ptr of Order
		* 3. Erase from map
		* 4. Unlink
		* 5. Cheack is bucket empty
		*/
		bool cancel(uint64_t id) noexcept {
			Order* order = get(id);
			if(order == nullptr){
				return false;
			}

			std::size_t bucket_id = calculateBucket(order->price);
			order->unlink();
			if (buckets[bucket_id].empty()) {
				bitset.reset(bucket_id);
			}

			order->is_active = false;
			order->id.epoch++;
			pool.free(order);

			return true;
		}
		Order* getCheapest() noexcept {
			std::size_t id = bitset.firstNotZeroBit();
			if (id == BITSET_EMPTY_FLAG_VALUE) {
				return nullptr;
			}
			return &buckets[id].getBestOrder();
		}
		Order* getDearest() noexcept {
			std::size_t id = bitset.lastNotZeroBit();
			if (id == BITSET_EMPTY_FLAG_VALUE) {
				return nullptr;
			}
			return &buckets[id].getWorstOrder();

		}
		Order* get(uint64_t id) const noexcept {
			Order_ID_Pack pack(id);
			if(pack.position >= pool_t::capacity()){
				return nullptr;
			}
			Order* order = pool.get(static_cast<pool_size_t>(pack.position));
			if(static_cast<uint64_t>(order->id) != id){
				return nullptr;
			}
			return order;
		}

		//~OrderStore() {}
	};
}
