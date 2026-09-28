#pragma once

#include <algorithm>
#include <cstdint>
#include <lob/parameters/parameters.hpp>
#include <lob/store/OrderStore.hpp>
#include <lob/trade_event/TradeEvent.hpp>
#include <lob/time/Time.hpp>

namespace lob {

	template<
		size_t min_price,
		size_t max_price,
		size_t bucket_size
	>
	class OrderBook {
	private:
		using store_t = OrderStore<static_cast<pool_size_t>(max_orders_per_lob), min_price, max_price, bucket_size>;

		store_t asks, bids;
        TradeEventsQueue& trade_queue;

        unsigned int free_trade_id;

	public:
        OrderBook(TradeEventsQueue& queue_) : trade_queue(queue_), free_trade_id(0){

        }
		uint64_t limit_buy(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match(price, quantity, id, asks, Side::Buy);
            if (remaining_qty == 0)return invalid_order_id;
			return bids.add(price, remaining_qty);
		}
        uint64_t limit_sell(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match(price, quantity, id, bids, Side::Sell);
            if (remaining_qty == 0)return invalid_order_id;
			return asks.add(price, remaining_qty);
		}

        bool cancel_buy(uint64_t id) {
            return bids.cancel(id);
        }
        bool cancel_sell(uint64_t id) {
            return asks.cancel(id);
        }

	private:
        uint32_t try_match(int64_t price, uint32_t quantity, uint64_t id, store_t& opposite_store, Side side) {


            while (quantity > 0) {
                uint64_t t_entry = get_ticks();

                Order* opposite = nullptr;
                if (side == Side::Buy) opposite = opposite_store.getCheapest();
                else opposite = opposite_store.getDearest();

                if (!opposite) break;

                bool price_match = (side == Side::Buy) ? (price >= opposite->price) : (price <= opposite->price);
                if (!price_match) break;

                uint64_t t_match = get_ticks();

                uint64_t maker_id = static_cast<uint64_t>(opposite->id);


                uint32_t match_qty = std::min(quantity, opposite->quantity);


                if (match_qty == opposite->quantity) {
                    opposite_store.cancel(maker_id);
                }
                else {
                    opposite->quantity -= match_qty;
                }



                auto* event_slot = trade_queue.prepare_push();

                while (event_slot == nullptr)[[likely]] {
                    event_slot = trade_queue.prepare_push();
                }

                uint64_t t_queue = get_ticks();

                if (event_slot != nullptr) {

                    uint32_t dt_match = static_cast<uint32_t>(t_match - t_entry);
                    uint32_t dt_queue = static_cast<uint32_t>(t_queue - t_match);

                    event_slot->fill(side, maker_id, id, price, match_qty, free_trade_id++, t_entry, dt_match, dt_queue);
                    //event_slot->t_entry = t_entry;
                    //event_slot->dt_match = dt_match;
                    //event_slot->dt_queue = dt_queue;
                    trade_queue.commit_push();
                }
                // Вывод трейда (Lock-free очередь)
                
                quantity -= match_qty;
            }
            return quantity;
        }
	};

}
