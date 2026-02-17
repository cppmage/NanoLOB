#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "OrderBook/OrderBook.h"


using namespace lob;

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook<0, 100, 10> book;
};

TEST_F(OrderBookTest, AddAndCancelOrders) {

    book.limit_buy(1, 100, 4);
    book.limit_sell(7, 99, 4);
}