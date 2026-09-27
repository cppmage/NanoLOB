#include <gtest/gtest.h>

#include <lob/order/Order_ID_Pack.hpp>
#include <lob/parameters/parameters.hpp>

#include <cstdint>
#include <random>
#include <type_traits>
#include <vector>

namespace {

static_assert(!std::is_convertible_v<lob::Order_ID_Pack, std::uint64_t>,
              "Order_ID_Pack must not convert to uint64_t implicitly");
static_assert(std::is_constructible_v<std::uint64_t, lob::Order_ID_Pack>,
              "Order_ID_Pack must convert to uint64_t explicitly");

constexpr std::uint64_t kMaxPosition = (1ULL << lob::order_position_bits) - 1;
constexpr std::uint64_t kMaxEpoch = (1ULL << lob::order_epoch_bits) - 1;

std::vector<std::uint64_t> InterestingIds() {
    std::vector<std::uint64_t> values = {
        0,
        1,
        kMaxPosition,                                          // position full, epoch zero
        kMaxPosition + 1,                                      // lowest id with epoch 1
        kMaxEpoch << lob::order_position_bits,                 // epoch full, position zero
        (kMaxEpoch << lob::order_position_bits) | kMaxPosition,  // every bit set
        UINT64_MAX,
    };

    std::mt19937_64 rng(12345);
    for (int i = 0; i < 64; ++i) {
        values.push_back(rng());
    }
    return values;
}

TEST(OrderIdPack, RoundTripsFullValueAndSplitsAtTheRightBit) {
    EXPECT_EQ(lob::order_position_bits + lob::order_epoch_bits, lob::id_size);

    const lob::Order_ID_Pack zero;
    EXPECT_EQ(zero.position, 0u);
    EXPECT_EQ(zero.epoch, 0u);

    for (const std::uint64_t value : InterestingIds()) {
        const lob::Order_ID_Pack id(value);

        EXPECT_EQ(id.position, value & kMaxPosition) << "value=" << value;
        EXPECT_EQ(id.epoch, (value >> lob::order_position_bits) & kMaxEpoch)
            << "value=" << value;
        EXPECT_EQ(static_cast<std::uint64_t>(id), value) << "value=" << value;
        EXPECT_EQ(id, lob::Order_ID_Pack(value)) << "value=" << value;
    }
}

TEST(OrderIdPack, FieldConstructorAgreesWithPackedConstructor) {
    const std::uint64_t positions[] = {0, 1, 42, kMaxPosition - 1, kMaxPosition};
    const std::uint64_t epochs[] = {0, 1, 42, kMaxEpoch - 1, kMaxEpoch};

    for (const std::uint64_t pos : positions) {
        for (const std::uint64_t ep : epochs) {
            const lob::Order_ID_Pack from_fields(pos, ep);
            const lob::Order_ID_Pack from_packed((ep << lob::order_position_bits) | pos);

            EXPECT_EQ(from_fields.position, pos) << "pos=" << pos << " ep=" << ep;
            EXPECT_EQ(from_fields.epoch, ep) << "pos=" << pos << " ep=" << ep;
            EXPECT_EQ(from_fields, from_packed) << "pos=" << pos << " ep=" << ep;
            EXPECT_EQ(static_cast<std::uint64_t>(from_fields), static_cast<std::uint64_t>(from_packed))
                << "pos=" << pos << " ep=" << ep;

            // Equality must separate ids that differ in only one field.
            EXPECT_NE(from_fields, lob::Order_ID_Pack(pos ^ 1u, ep))
                << "pos=" << pos << " ep=" << ep;
            EXPECT_NE(from_fields, lob::Order_ID_Pack(pos, ep ^ 1u))
                << "pos=" << pos << " ep=" << ep;
        }
    }
}

// Epoch dominates: a slot reused in a later epoch outranks any position from an
// earlier one. This is what makes a stale id compare as older.
TEST(OrderIdPack, OrdersByEpochThenPosition) {
    const lob::Order_ID_Pack old_epoch_high_position(kMaxPosition, 1);
    const lob::Order_ID_Pack new_epoch_low_position(0, 2);

    EXPECT_TRUE(old_epoch_high_position < new_epoch_low_position);
    EXPECT_TRUE(new_epoch_low_position > old_epoch_high_position);

    // Within one epoch, position breaks the tie.
    const lob::Order_ID_Pack same_epoch_low(7, 5);
    const lob::Order_ID_Pack same_epoch_high(8, 5);

    EXPECT_TRUE(same_epoch_low < same_epoch_high);
    EXPECT_TRUE(same_epoch_high > same_epoch_low);
}

TEST(OrderIdPack, ComparisonIsAStrictOrder) {
    std::vector<lob::Order_ID_Pack> ids;
    for (const std::uint64_t value : InterestingIds()) {
        ids.emplace_back(value);
    }

    for (const auto& a : ids) {
        EXPECT_FALSE(a < a) << "irreflexive: " << static_cast<std::uint64_t>(a);
        EXPECT_FALSE(a > a) << "irreflexive: " << static_cast<std::uint64_t>(a);

        for (const auto& b : ids) {
            // Ordering the packed values must agree with ordering the structs,
            // since epoch occupies the high bits.
            const std::uint64_t av = static_cast<std::uint64_t>(a);
            const std::uint64_t bv = static_cast<std::uint64_t>(b);

            EXPECT_EQ(a < b, av < bv) << av << " vs " << bv;
            EXPECT_EQ(a > b, av > bv) << av << " vs " << bv;
            EXPECT_EQ(a == b, av == bv) << av << " vs " << bv;
        }
    }
}

}  // namespace
