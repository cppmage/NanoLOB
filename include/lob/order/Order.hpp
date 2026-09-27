#pragma once
#include <boost/intrusive/list.hpp>
#include <lob/order/Order_ID_Pack.hpp>
#include <lob/parameters/parameters.hpp>

namespace lob {
	using order_hook = boost::intrusive::list_base_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>>;

	struct alignas(cache_line_size) Order : public order_hook {
	private:

	public:

		friend bool operator<(const Order& a, const Order& b) {
			if (a.price != b.price) return a.price < b.price;
			return a.id < b.id; 
		}
		friend bool operator>(const Order& a, const Order& b) {
			if (a.price != b.price) return a.price > b.price;
			return a.id > b.id; 
		}

		Order_ID_Pack id;
		int64_t price;
		uint32_t quantity;
		uint32_t executed_qty;
		uint64_t timestamp;
		bool is_active;

		uint8_t reserved[15];
		Order(uint64_t id_, int64_t price_, uint32_t quantity_, uint64_t timestamp_)
			: id(id_), price(price_), quantity(quantity_),
			executed_qty(0), timestamp(timestamp_), is_active(false)
		{

		}
		Order()
			: id(0), price(0), quantity(0),
			executed_qty(0), timestamp(0), is_active(false)
		{

		}
	};


	using OrderList = boost::intrusive::list<Order, boost::intrusive::constant_time_size<false>>;

	static_assert(sizeof(Order) == cache_line_size, "Order struct must fit cache line");
}
