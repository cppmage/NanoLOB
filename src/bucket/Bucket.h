#pragma once
#include "record_type/Order.h"

namespace lob {

	struct Bucket {
		lob::OrderList list;
		Bucket(){}
		void add(Order& order) {
			auto price = order.price;
			auto it = list.begin();

			while (it != list.end() and it->price <= price) {
				it++;
			}
			list.insert(it, order);
		}
		void unlink(Order& order) {
			order.unlink();
		}
		bool empty() const {
			return list.cbegin()==list.cend();
		}
		Order& getBestOrder() {
			return list.front();
		}
		Order& getWorstOrder() {
			return list.back();
		}
	};
}