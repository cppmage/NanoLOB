#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "store/OrderStore.h"

using namespace lob;

class OrderStoreTest : public ::testing::Test {
protected:
    OrderStore<0, 100, 1> store;
};

TEST_F(OrderStoreTest, AddAndCancelOrders) {


    store.add(1, 99, 0);
    store.add(2, 50, 0);
    store.add(3, 75, 0);
    store.add(4, 25, 0);
    store.add(5, 4, 0);
    store.add(6, 12, 0);

    store.cancel(1);
    EXPECT_EQ(store.getCheapest()->price, 4);
    EXPECT_EQ(store.getDearest()->price, 75);

    store.cancel(5);
    EXPECT_EQ(store.getCheapest()->price, 12);
    EXPECT_EQ(store.getDearest()->price, 75);

    store.cancel(3);
    EXPECT_EQ(store.getCheapest()->price, 12);
    EXPECT_EQ(store.getDearest()->price, 50);
}

TEST_F(OrderStoreTest, MultipleOrdersAtSamePrice) {
    store.add(10, 50, 100);
    store.add(11, 50, 200); 

    EXPECT_EQ(store.getCheapest()->id, 10); 

    store.cancel(10);
    EXPECT_EQ(store.getCheapest()->id, 11);
    EXPECT_EQ(store.getCheapest()->price, 50);

    store.cancel(11);
    EXPECT_EQ(store.getCheapest(), nullptr);
}