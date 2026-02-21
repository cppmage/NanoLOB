#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "OrderBook/OrderBook.h"
#include "lockfree/SPSCQueue.h"


using namespace lob;

class OrderBookTest : public ::testing::Test {
protected:
    
};

TEST_F(OrderBookTest, AddAndCancelOrders) {
    TradeEventsQueue queue;
    OrderBook<0, 100, 1> book(queue);

    book.limit_buy(1, 100, 4);
    book.limit_sell(7, 99, 4);

    EXPECT_NE(queue.prepare_pop(), nullptr);

}

TEST_F(OrderBookTest, PartialMatchAndQueueStress) {
    TradeEventsQueue queue; 
    OrderBook<0, 1000, 1> book(queue);

    for (int i = 0; i < 50; ++i) {
        book.limit_buy(100 + i, 500 - i, 1);
    }

    
    uint64_t seller_id = 999;
    book.limit_sell(seller_id, 1, 50); 

    int events_count = 0;
    uint32_t total_matched_qty = 0;

    while (true) {
        auto* event = queue.prepare_pop();
        if (!event) break;

        events_count++;
        queue.commit_pop();
    }

    EXPECT_EQ(events_count, 50)
        << "Missed deals";

}