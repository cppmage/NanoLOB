#pragma once

#include <store/OrderStore.h>
#include <TradeEvent/TradeEvent.h>
#include <time/Time.h>

namespace lob {

	template<
		size_t min_price, 
		size_t max_price, 
		size_t bucket_size
	>
	class OrderBook {
	private:
        static constexpr size_t arr_size = (max_price - min_price + bucket_size) / bucket_size;
		OrderStore<min_price, max_price, bucket_size, arr_size> asks, bids;
        TradeEventsQueue& trade_queue;

        unsigned int free_trade_id;

	public:
        OrderBook(TradeEventsQueue& queue_) : trade_queue(queue_), free_trade_id(0){

        }
		bool limit_buy(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match(price, quantity, id, asks, Side::Buy);
            if (remaining_qty == 0)return false;
			bids.add(id, price, remaining_qty);
            return true;
		}
        bool limit_sell(uint64_t id, int64_t price, uint32_t quantity) {

			uint32_t remaining_qty = try_match(price, quantity, id, bids, Side::Sell);
            if (remaining_qty == 0)return false;
			asks.add(id, price, remaining_qty);
            return true;
		}

        void cancel_buy(uint64_t id) {
            bids.cancel(id);
        }
        void cancel_sell(uint64_t id) {
            asks.cancel(id);
        }

	private:
        uint32_t try_match(int64_t price, uint32_t quantity, uint64_t id, auto& opposite_store, Side side) {


            while (quantity > 0) {
                uint64_t t_entry = get_ticks();

                Order* opposite = nullptr;
                if (side == Side::Buy) opposite = opposite_store.getCheapest();
                else opposite = opposite_store.getDearest();

                if (!opposite) break;

                bool price_match = (side == Side::Buy) ? (price >= opposite->price) : (price <= opposite->price);
                if (!price_match) break;

                uint64_t t_match = get_ticks();

                uint64_t taker_id = opposite->id;


                uint32_t match_qty = std::min(quantity, opposite->quantity);

               
                if (match_qty == opposite->quantity) {
                    opposite_store.cancel(opposite->id);
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

                    event_slot->fill(side, id, taker_id, price, match_qty, free_trade_id++, t_entry, dt_match, dt_queue);
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

