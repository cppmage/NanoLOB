#include <gtest/gtest.h>
#include "wal/WALShared.h" 
#include <thread>
#include <vector>
#include "TradeEvent/TradeEvent.h"

using namespace lob;

struct TestPacket {
    uint64_t id;
    double value;
    char label[16];

    bool operator==(const TestPacket& other) const {
        return id == other.id && value == other.value && std::strcmp(label, other.label) == 0;
    }
};

class WALTest : public ::testing::Test {
protected:
     lob::WALQueue wal;
     WALTest() : wal("wal_test.bin"){

     }
};

TEST_F(WALTest, PushPopSingleObject) {

    TestPacket sent = { 42, 3.14, "Hello" };
    TestPacket received = { 0, 0.0, "" };

    wal.try_push_object(sent);
    bool success = wal.try_pop_object(received);

    EXPECT_TRUE(success);
    EXPECT_EQ(sent.id, received.id);
    EXPECT_STREQ(sent.label, received.label);
}

TEST_F(WALTest, PopEmptyReturnsFalse) {
    TestPacket received;
    EXPECT_FALSE(wal.try_pop_object(received));
}

TEST_F(WALTest, WrapAroundIntegrity) {
    TestPacket p;
    size_t packet_size = sizeof(TestPacket); 

    
    size_t iterations = (1024 / packet_size) - 1;
    for (size_t i = 0; i < iterations; ++i) {
        wal.try_push_object(TestPacket{ i, 0.0, "fill" });
    }

    for (size_t i = 0; i < iterations; ++i) {
        wal.try_pop_object(p);
    }

    TestPacket split_packet = { 999, 99.9, "split" };
    wal.try_push_object(split_packet);

    TestPacket recovered = { 0 };
    EXPECT_TRUE(wal.try_pop_object(recovered));
    EXPECT_EQ(split_packet.id, recovered.id);
    EXPECT_STREQ(split_packet.label, recovered.label);
}

TEST_F(WALTest, MultiThreadedSPSC) {
    const int count = 10000;
    std::thread producer([&]() {
        for (int i = 0; i < count; ++i) {
            TestPacket p = { static_cast<uint64_t>(i), 1.0, "data" };
            while (!wal.try_push_object(p)) {
                std::this_thread::yield();
            }
        }
        });

    std::thread consumer([&]() {
        int received_count = 0;
        while (received_count < count) {
            TestPacket p;
            if (wal.try_pop_object(p)) {
                EXPECT_EQ(p.id, received_count);
                received_count++;
            }
            else {
                std::this_thread::yield();
            }
        }
        EXPECT_EQ(received_count, count);
        });

    producer.join();
    consumer.join();
}