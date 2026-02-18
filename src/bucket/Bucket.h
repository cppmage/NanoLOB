#pragma once
#include "record_type/Order.h"

namespace lob {

	struct Bucket {
		lob::OrderList list;
		Bucket(){}
		void add(Order& order) {
			list.push_back(order);
		}
		void unlink(Order& order) {
			order.unlink();
		}
		bool empty() const noexcept {
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