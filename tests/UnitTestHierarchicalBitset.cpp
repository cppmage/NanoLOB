#include <gtest/gtest.h>

#include <lob/bitset/HierarchicalBitset.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <random>
#include <set>
#include <vector>

namespace {

template <std::size_t Bits, std::size_t TopWords>
struct HierarchicalBitsetConfig {
    using Set = lob::HierarchicalBitset<Bits, TopWords>;
    static constexpr std::size_t kBits = Bits;
};

template <class Config>
class HierarchicalBitsetTest : public ::testing::Test {
protected:
    using Set = typename Config::Set;
    static constexpr std::size_t kBits = Config::kBits;

    void SetUp() override { set_ = std::make_unique<Set>(); }
    Set& bitset() { return *set_; }

    static std::vector<std::size_t> InterestingPositions() {
        std::vector<std::size_t> candidates = {
            0, 1, 63, 64, 65, 127, 128, 4095, 4096, 4097, kBits / 2, kBits - 2, kBits - 1,
        };
        std::vector<std::size_t> out;
        for (std::size_t p : candidates) {
            if (p < kBits) out.push_back(p);
        }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
        return out;
    }

    void ExpectMatches(const std::set<std::size_t>& model, const char* where) {
        if (model.empty()) {
            EXPECT_EQ(bitset().firstNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << where;
            EXPECT_EQ(bitset().lastNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << where;
        } else {
            EXPECT_EQ(bitset().firstNotZeroBit(), *model.begin()) << where;
            EXPECT_EQ(bitset().lastNotZeroBit(), *model.rbegin()) << where;
        }
    }

private:
    std::unique_ptr<Set> set_;
};

using HierarchicalBitsetTypes = ::testing::Types<
    HierarchicalBitsetConfig<1, 1>,
    HierarchicalBitsetConfig<64, 1>,
    HierarchicalBitsetConfig<65, 1>,
    HierarchicalBitsetConfig<100, 4>,
    HierarchicalBitsetConfig<4097, 1>,
    HierarchicalBitsetConfig<8192, 4>,
    HierarchicalBitsetConfig<300000, 1>
    >;

TYPED_TEST_SUITE(HierarchicalBitsetTest, HierarchicalBitsetTypes);

TYPED_TEST(HierarchicalBitsetTest, StartsEmpty) {
    this->ExpectMatches({}, "fresh");
}

TYPED_TEST(HierarchicalBitsetTest, SingleBitRoundTripsAtEveryInterestingPosition) {
    for (std::size_t pos : TestFixture::InterestingPositions()) {
        this->bitset().set(pos);
        EXPECT_EQ(this->bitset().firstNotZeroBit(), pos) << "pos=" << pos;
        EXPECT_EQ(this->bitset().lastNotZeroBit(), pos) << "pos=" << pos;

        this->bitset().reset(pos);
        EXPECT_EQ(this->bitset().firstNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << "pos=" << pos;
        EXPECT_EQ(this->bitset().lastNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << "pos=" << pos;
    }
}

TYPED_TEST(HierarchicalBitsetTest, ExtremesFollowSetsAndResets) {
    const auto positions = TestFixture::InterestingPositions();
    std::set<std::size_t> model;

    for (std::size_t pos : positions) {
        this->bitset().set(pos);
        model.insert(pos);
        this->ExpectMatches(model, "while setting");
    }
    for (std::size_t pos : positions) {
        this->bitset().reset(pos);
        model.erase(pos);
        this->ExpectMatches(model, "resetting from the low end");
    }

    for (std::size_t pos : positions) {
        this->bitset().set(pos);
        model.insert(pos);
    }
    for (auto it = positions.rbegin(); it != positions.rend(); ++it) {
        this->bitset().reset(*it);
        model.erase(*it);
        this->ExpectMatches(model, "resetting from the high end");
    }
}

TYPED_TEST(HierarchicalBitsetTest, SetIsIdempotentAndResetOfClearBitIsHarmless) {
    const std::size_t pos = TestFixture::kBits - 1;

    this->bitset().set(pos);
    this->bitset().set(pos);
    this->bitset().reset(pos);
    this->ExpectMatches({}, "a double set must be undone by one reset");

    this->bitset().reset(0);
    this->ExpectMatches({}, "resetting a clear bit");

    this->bitset().set(pos);
    this->bitset().reset(0);
    if (pos != 0) {
        this->ExpectMatches({pos}, "resetting a clear bit must not disturb a set one");
    }
}

TYPED_TEST(HierarchicalBitsetTest, RandomisedSetResetMatchesModel) {
    constexpr int kOps = 3000;
    constexpr std::size_t kWindow = 512;

    std::mt19937_64 rng(12345);
    std::bernoulli_distribution set_coin(0.55);
    std::bernoulli_distribution clustered(0.5);
    std::set<std::size_t> model;

    for (int op = 0; op < kOps; ++op) {
        // Half the ids crowd into a small window, so neighbours in one word
        // and one subtree are exercised, not just scattered singletons.
        const std::size_t range = clustered(rng) ? std::min(kWindow, TestFixture::kBits) : TestFixture::kBits;
        const std::size_t id = static_cast<std::size_t>(rng() % range);

        if (set_coin(rng)) {
            this->bitset().set(id);
            model.insert(id);
        } else {
            this->bitset().reset(id);
            model.erase(id);
        }

        if (model.empty()) {
            ASSERT_EQ(this->bitset().firstNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << "op=" << op;
            ASSERT_EQ(this->bitset().lastNotZeroBit(), lob::BITSET_EMPTY_FLAG_VALUE) << "op=" << op;
        } else {
            ASSERT_EQ(this->bitset().firstNotZeroBit(), *model.begin()) << "op=" << op;
            ASSERT_EQ(this->bitset().lastNotZeroBit(), *model.rbegin()) << "op=" << op;
        }
    }
}

}  // namespace
