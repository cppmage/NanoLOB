#pragma once

#include <cstdint>
#include <new>
#include <chrono>
#include <lockfree/SPSCQueue.h>
#include <other/Other.h>

namespace lob {
	enum class Side : bool {
		Sell,
		Buy
	};
	
	struct alignas(std::hardware_constructive_interference_size) TradeEvent {
		uint64_t timestamp;
		uint64_t maker_order_id;
		uint64_t taker_order_id;
		int64_t  price;
		uint32_t quantity;
		uint32_t trade_id;
		Side  taker_side;

		TradeEvent() :
			timestamp(0), maker_order_id(0), taker_order_id(0), price(0),
			quantity(0), trade_id(0), taker_side(Side::Sell)
		{

		}
		template<Side side>
		void fill(uint64_t maker_order_id_, uint64_t taker_order_id_, 
			int64_t  price_, uint32_t quantity_, uint32_t trade_id_) {
			timestamp = getCurrentTimeAsUint64();
			taker_side = side;

			maker_order_id = maker_order_id_;
			taker_order_id = taker_order_id_;

			price = price_;
			quantity = quantity_;

			trade_id = trade_id_;
		}
	};

	using TradeEventsQueue = SPSCQueue<TradeEvent, 9>;

}