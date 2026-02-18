#include <gtest/gtest.h>
#include "lockfree/SPSCQueue.h"
#include <thread>
#include <immintrin.h>

using namespace lob;

using test1_queue = SPSCQueue<int, 4>;
using test2_queue = SPSCQueue<int, 11>;

class SPSCQueueTest : public ::testing::Test {
protected:
    test1_queue queue1;
    test2_queue queue2;
};

template<typename queue_t>
void producer(std::stop_token stoken, queue_t& queue, int& sum) {

    for (int i = 0; i < 1000; i++) {
        if (queue.try_push(i)) {
            sum += i;
            //std::cout << i << std::endl;
        }
    }

}
template<typename queue_t>
void consumer(std::stop_token stoken, queue_t& queue, int& sum) {
    int value = 0;
    while (!stoken.stop_requested()) {

        while (!queue.try_pop(value)) {
            _mm_pause();
            if (stoken.stop_requested())return;
        }
        sum += value;
    }
}

TEST_F(SPSCQueueTest, TryPop_WhenEmpty_ReturnsFalse) {
    test1_queue queue;
    int value;
    EXPECT_FALSE(queue.try_pop(value));
}
TEST_F(SPSCQueueTest, TryPush_WhenFull_ReturnsFalse) {
    test1_queue queue; 

    for (int i = 0; i < 16; i++) {
        EXPECT_TRUE(queue.try_push(i));
    }

    EXPECT_FALSE(queue.try_push(16));

    int value;
    for (int i = 0; i < 16; i++) {
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }
}

TEST_F(SPSCQueueTest, TryPush_AND_TryPopSmallQueeu) {

    int sum1 = 0, sum2 = 0;
    std::jthread jt1(producer<test1_queue>, std::ref(queue1), std::ref(sum1));
    std::jthread jt2(consumer<test1_queue>, std::ref(queue1), std::ref(sum2));

    jt1.join();

    std::atomic_thread_fence(std::memory_order::seq_cst);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::atomic_thread_fence(std::memory_order::seq_cst);

    jt2.request_stop();
    jt2.join();

    std::atomic_thread_fence(std::memory_order::seq_cst);

    EXPECT_EQ(sum1, sum2);

}
TEST_F(SPSCQueueTest, TryPush_AND_TryPopBigQueeu) {

    int sum1 = 0, sum2 = 0;
    std::jthread jt1(producer<test2_queue>, std::ref(queue2), std::ref(sum1));
    std::jthread jt2(consumer<test2_queue>, std::ref(queue2), std::ref(sum2));

    jt1.join();

    std::atomic_thread_fence(std::memory_order::seq_cst);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::atomic_thread_fence(std::memory_order::seq_cst);

    jt2.request_stop();
    jt2.join();

    std::atomic_thread_fence(std::memory_order::seq_cst);

    EXPECT_EQ(sum1, sum2);

}

TEST_F(SPSCQueueTest, InterleavedPushPop) {
    test1_queue queue; // size 16

    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(queue.try_push(i));
    }

    int value;
    for (int i = 0; i < 3; i++) {
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }

    for (int i = 5; i < 10; i++) {
        EXPECT_TRUE(queue.try_push(i));
    }

    for (int i = 3; i < 10; i++) {
        EXPECT_TRUE(queue.try_pop(value));
        EXPECT_EQ(value, i);
    }
}

TEST_F(SPSCQueueTest, StressTest) {
    test2_queue queue; 

    constexpr int NUM_OPS = 1000000;
    std::atomic<int> sum1{ 0 }, sum2{ 0 };

    auto producer = [&](std::stop_token stoken) {
        for (int i = 0; i < NUM_OPS; i++) {
            while (!queue.try_push(i)) {
                _mm_pause();
            }
            sum1 += i;
        }
        };

    auto consumer = [&](std::stop_token stoken) {
        int value;
        int count = 0;
        while (count < NUM_OPS) {
            if (queue.try_pop(value)) {
                sum2 += value;
                count++;
            }
            else {
                _mm_pause();
            }
        }
        };

    std::jthread jt1(producer);
    std::jthread jt2(consumer);

    jt1.join();
    jt2.join();

    EXPECT_EQ(sum1.load(), sum2.load());
}

TEST_F(SPSCQueueTest, WrapAroundTest) {
    test1_queue queue; // size 16

    for (int cycle = 0; cycle < 10; cycle++) {
        for (int i = 0; i < 10; i++) {
            EXPECT_TRUE(queue.try_push(cycle * 100 + i));
        }
        int value;
        for (int i = 0; i < 10; i++) {
            EXPECT_TRUE(queue.try_pop(value));
            EXPECT_EQ(value, cycle * 100 + i);
        }
    }
}

TEST_F(SPSCQueueTest, MultithreadedWithDelays) {
    test2_queue queue;
    std::atomic<int> sum1{ 0 }, sum2{ 0 };
    std::atomic<bool> producer_done{ false };

    auto slow_producer = [&](std::stop_token stoken) {
        for (int i = 0; i < 100; i++) {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            while (!queue.try_push(i)) {
                _mm_pause();
            }
            sum1 += i;
        }
        producer_done = true;
        };

    auto fast_consumer = [&](std::stop_token stoken) {
        int value;
        while (!producer_done || !queue.empty()) {
            if (queue.try_pop(value)) {
                sum2 += value;
            }
            else {
                _mm_pause();
            }
        }
        };

    std::jthread jt1(slow_producer);
    std::jthread jt2(fast_consumer);

    jt1.join();
    jt2.request_stop();
    jt2.join();

    EXPECT_EQ(sum1.load(), sum2.load());
}