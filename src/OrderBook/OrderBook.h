#pragma once

#include <store/OrderStore.h>


namespace lob {

	enum class Side : bool {
		Sell,
		Buy
	};

	template<
		size_t min_price, 
		size_t max_price, 
		size_t bucket_size, 
		size_t arr_size = (max_price - min_price + bucket_size) / bucket_size
	>
	class OrderBook {
	private:
		OrderStore<min_price, max_price, bucket_size, arr_size> asks, bids;
	public:

		int limit_buy(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match<Side::Buy>(price, quantity, id);
            if (remaining_qty == 0)return 0;
			bids.add(id, price, remaining_qty);
            return remaining_qty;
		}
		void limit_sell(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match<Side::Sell>(price, quantity, id);
            if (remaining_qty == 0)return;
			asks.add(id, price, remaining_qty);
		}

        void cancel_buy(uint64_t id) {
            bids.cancel(id);
        }
        void cancel_sell(uint64_t id) {
            asks.cancel(id);
        }

	private:
        template<Side side>
        uint32_t try_match(int64_t price, uint32_t quantity, uint64_t id) {
            auto& opposite_store = (side == Side::Buy) ? asks : bids;

            while (quantity > 0) {
                Order* opposite = (side == Side::Buy) ? opposite_store.getCheapest() : opposite_store.getDearest();

                if (!opposite) break;

                bool price_match = (side == Side::Buy) ? (price >= opposite->price) : (price <= opposite->price);
                if (!price_match) break;

                uint32_t match_qty = std::min(quantity, opposite->quantity);

               
                if (match_qty == opposite->quantity) {
                    opposite_store.cancel(opposite->id);
                }
                else {
                    opposite->quantity -= match_qty; 
                }

                // Вывод трейда (Lock-free очередь)
                
                quantity -= match_qty;
            }
            return quantity;
        }
	};

}

