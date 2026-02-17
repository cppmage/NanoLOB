#include <gtest/gtest.h>
#include "allocators/ObjectPool.h"
#include "record_type/Order.h"
#include <random>


using namespace lob;

class ObjectPoolTest : public ::testing::Test {
protected:
    
};

TEST_F(ObjectPoolTest, AllocateFree) {
    ObjectPool<Order> pool(4);
    Order* ptr1 = pool.allocate();
    Order* ptr2 = pool.allocate();
    Order* ptr3 = pool.allocate();
    Order* ptr4 = pool.allocate();
    Order* ptr5 = pool.allocate();

    EXPECT_FALSE(ptr1==ptr2);
    EXPECT_EQ(ptr1->id, ptr2->id);
    EXPECT_EQ(ptr5, nullptr);

    pool.free(ptr2);
    pool.free(ptr3);
    Order* ptr6 = pool.allocate();
    Order* ptr7 = pool.allocate();

    EXPECT_EQ(ptr6, ptr3);
    EXPECT_EQ(ptr7, ptr2);
}