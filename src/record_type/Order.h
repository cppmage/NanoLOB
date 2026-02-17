#pragma once
#include <boost/intrusive/list.hpp>


using namespace boost;

namespace lob {
	//using order_hook =  intrusive::list_base_hook<intrusive::link_mode<intrusive::auto_unlink>>;
	using order_hook = intrusive::list_base_hook<intrusive::link_mode<intrusive::auto_unlink>>;

	struct alignas(64) Order : public order_hook {
	private:

	public:

		friend bool operator<(const Order& a, const Order& b) {
			if (a.price != b.price) return a.price < b.price;
			return a.id < b.id; 
		}
		friend bool operator<(const Order& a, const Order& b) {
			if (a.price != b.price) return a.price > b.price;
			return a.id > b.id; 
		}

		uint64_t id;
		int64_t price;
		uint32_t quantity;
		uint32_t executed_qty;
		uint64_t timestamp;
	
		uint8_t reserved[16];
		Order(uint64_t id_, int64_t price_, uint32_t quantity_, uint64_t timestamp_)
			: id(id_), price(price_), quantity(quantity_),
			executed_qty(0), timestamp(timestamp_) 
		{

		}
		Order()
			: id(0), price(0), quantity(0),
			executed_qty(0), timestamp(0)
		{

		}
	};


	using OrderList = intrusive::list<Order, intrusive::constant_time_size<false>>;

	static_assert(sizeof(Order) == 64, "Order struct must be exactly 64 bytes to fit cache line");
}
