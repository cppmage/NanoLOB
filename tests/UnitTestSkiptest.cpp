
#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include "skiplist/Skiplist.h"


class SkipListTest : public ::testing::Test {
protected:
    
};

TEST_F(SkipListTest, Test1) {
    IntrusiveSkiplist<std::greater<>> list;
    srand(time(0));

    std::vector<test_node> nodes;
    nodes.resize(1024);
    int mini = INT32_MAX;
    int maxi = INT32_MIN;
    int factor = 0;

    for (auto& el : nodes) {
        el.key = rand() % 100000 - factor;
        mini = std::min(mini, el.key);
        maxi = std::max(maxi, el.key);
        list.insert(&el);

        factor++;

        EXPECT_EQ(mini, list.front()->key);
        EXPECT_EQ(maxi, list.back()->key);
    }

}

TEST_F(SkipListTest, Test2Unlink) {
    IntrusiveSkiplist<std::greater<>> list;
    
    test_node node10, node20, node30, node40;
    node10.key = 10;
    node20.key = 20;
    node30.key = 30;
    node40.key = 40;

    list.insert(&node30);
    list.insert(&node10);
    list.insert(&node20);
    list.insert(&node40);

    EXPECT_EQ(10, list.front()->key);
    EXPECT_EQ(40, list.back()->key);

    node10.unlink();
    EXPECT_EQ(20, list.front()->key);
    EXPECT_EQ(40, list.back()->key);

    node40.unlink();
    EXPECT_EQ(20, list.front()->key);
    EXPECT_EQ(30, list.back()->key);

    node30.unlink();
    EXPECT_EQ(20, list.front()->key);
    EXPECT_EQ(20, list.back()->key);

    node40.unlink();
}

