#include <gtest/gtest.h>
#include "logger/Logger.h"
#include <thread>
#include <immintrin.h>

using namespace lob;

class LoggerTest : public ::testing::Test {
protected:
    
};




TEST_F(LoggerTest, AddThenLaunchThenCheack) {
    
    const int test_size = 16;
    TradeEventsQueue queue;
    WALQueue wal("logger_test.bin");

    Logger logger(queue, wal);

    std::vector<TradeEvent> events;
    std::vector<TradeEvent> result;
    events.reserve(test_size);
    result.reserve(test_size);

    for (int i = 0; i < test_size; i++) {
        events.push_back({});
        events.back().trade_id = i;
    }

    for(auto& el : events)
        queue.try_push(el);
    std::jthread jt1([&logger](std::stop_token st) {
        logger.process(st);
        });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    jt1.request_stop();
    jt1.join();

    TradeEvent el;
    while (wal.try_pop_object(el)) {
        result.push_back(el);
    }

    for (int i = 0; i < test_size; i++) {
        EXPECT_EQ(events[i].trade_id, result[i].trade_id);
    }


}