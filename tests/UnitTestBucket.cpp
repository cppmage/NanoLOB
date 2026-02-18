#include <bucket/Bucket.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace lob;

class BucketTest : public ::testing::Test {
protected:
    // Тут инициализируешь свой PMR аллокатор и стакан
    Bucket bucket;
};

TEST_F(BucketTest, AddAndCancelOrder) {
    

    int64_t min_price = INT64_MAX;
    
    std::vector<Order> orders = {
        {0, 10, 1, 1},
        {0, 20, 1, 1},
        {0, 30, 1, 1},
        {0, 25, 1, 1},
        {0, 6, 1, 1},
        {0, 15, 1, 1},
        {0, 1, 1, 1},
        {0, 100, 1, 1}
    };

    for (auto& el : orders) {
        bucket.add(el);
        min_price = std::min(min_price, el.price);
        //EXPECT_EQ(bucket.getBestOrder().price, min_price);
    }

    for (int i = 0; i < orders.size()-1; i++) {
        bucket.unlink(orders[i]);
        min_price = INT64_MAX;
        for (int j = i + 1; j < orders.size(); j++) {
            min_price = std::min(min_price, orders[j].price);
        }
        //EXPECT_EQ(bucket.getBestOrder().price, min_price);
    }


}
