// OrderStore behaviour across several pool sizes and price layouts.
//
// getCheapest()/getDearest() resolve only to bucket granularity -- within a
// bucket the list is in insertion order, not price order -- so the tests assert
// the bucket of the returned order, not its exact price.
//
// get() is bounds- and epoch-checked, so ids that were never issued are safe
// to pass and must come back as nullptr.

#include <gtest/gtest.h>

#include <lob/parameters/parameters.hpp>
#include <lob/store/OrderStore.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <vector>

namespace {

template <lob::pool_size_t Orders, std::size_t MinPrice, std::size_t MaxPrice, std::size_t BucketSize>
struct StoreConfig {
    using Store = lob::OrderStore<Orders, MinPrice, MaxPrice, BucketSize>;

    static constexpr lob::pool_size_t kOrders = Orders;
    static constexpr std::int64_t kMinPrice = static_cast<std::int64_t>(MinPrice);
    static constexpr std::int64_t kMaxPrice = static_cast<std::int64_t>(MaxPrice);
    static constexpr std::size_t kBucketSize = BucketSize;

    static std::size_t BucketOf(std::int64_t price) {
        return (static_cast<std::size_t>(price) - MinPrice) / BucketSize;
    }
};

template <class Config>
class OrderStoreTest : public ::testing::Test {
protected:
    using Store = typename Config::Store;

    static constexpr lob::pool_size_t kOrders = Config::kOrders;
    static constexpr std::int64_t kMinPrice = Config::kMinPrice;
    static constexpr std::int64_t kMaxPrice = Config::kMaxPrice;

    void SetUp() override { store_ = std::make_unique<Store>(); }

    Store& store() { return *store_; }

    static std::size_t BucketOf(std::int64_t price) { return Config::BucketOf(price); }

    // A price inside the range, spread out so successive indices usually land
    // in different buckets.
    static std::int64_t PriceAt(std::size_t i) {
        const std::int64_t span = kMaxPrice - kMinPrice + 1;
        return kMinPrice + static_cast<std::int64_t>((i * 7) % static_cast<std::size_t>(span));
    }

    // Fills the store to capacity and returns the ids handed out.
    std::vector<std::uint64_t> FillToCapacity() {
        std::vector<std::uint64_t> ids;
        ids.reserve(kOrders);
        for (std::size_t i = 0; i < kOrders; ++i) {
            const std::uint64_t id = store().add(PriceAt(i), 1);
            EXPECT_NE(id, lob::invalid_order_id) << "store exhausted early at i=" << i;
            ids.push_back(id);
        }
        return ids;
    }

private:
    std::unique_ptr<Store> store_;
};

using StoreTypes = ::testing::Types<
    StoreConfig<64, 0, 999, 1>       // one bucket per price
    >;

TYPED_TEST_SUITE(OrderStoreTest, StoreTypes);

TYPED_TEST(OrderStoreTest, StartsEmpty) {
    EXPECT_EQ(this->store().getCheapest(), nullptr);
    EXPECT_EQ(this->store().getDearest(), nullptr);
}

TYPED_TEST(OrderStoreTest, AddStoresTheOrderAndItsFields) {
    const std::int64_t price = TestFixture::kMinPrice;
    const std::uint64_t id = this->store().add(price, 42);
    ASSERT_NE(id, lob::invalid_order_id);

    lob::Order* order = this->store().get(id);
    ASSERT_NE(order, nullptr);

    EXPECT_EQ(order->price, price);
    EXPECT_EQ(order->quantity, 42u);
    EXPECT_EQ(order->executed_qty, 0u);
    EXPECT_TRUE(order->is_active);
    EXPECT_EQ(static_cast<std::uint64_t>(order->id), id);
}

TYPED_TEST(OrderStoreTest, RejectsPricesOutsideTheRange) {
    auto& store = this->store();

    EXPECT_EQ(store.add(TestFixture::kMinPrice - 1, 1), lob::invalid_order_id);
    EXPECT_EQ(store.add(TestFixture::kMaxPrice + 1, 1), lob::invalid_order_id);
    EXPECT_EQ(store.add(-1, 1), lob::invalid_order_id);
    EXPECT_EQ(store.add(INT64_MIN, 1), lob::invalid_order_id);
    EXPECT_EQ(store.add(INT64_MAX, 1), lob::invalid_order_id);

    // A rejected add must leave the book untouched.
    EXPECT_EQ(store.getCheapest(), nullptr);
    EXPECT_EQ(store.getDearest(), nullptr);
}

TYPED_TEST(OrderStoreTest, AcceptsBothEndsOfTheRange) {
    auto& store = this->store();

    const std::uint64_t low = store.add(TestFixture::kMinPrice, 1);
    ASSERT_NE(low, lob::invalid_order_id);

    lob::Order* cheapest = store.getCheapest();
    ASSERT_NE(cheapest, nullptr);
    EXPECT_EQ(TestFixture::BucketOf(cheapest->price), TestFixture::BucketOf(TestFixture::kMinPrice));

    if (TestFixture::kOrders < 2) {
        GTEST_SKIP() << "single-order pool cannot hold both ends at once";
    }

    const std::uint64_t high = store.add(TestFixture::kMaxPrice, 1);
    ASSERT_NE(high, lob::invalid_order_id);

    lob::Order* dearest = store.getDearest();
    ASSERT_NE(dearest, nullptr);
    EXPECT_EQ(TestFixture::BucketOf(dearest->price), TestFixture::BucketOf(TestFixture::kMaxPrice));
}

TYPED_TEST(OrderStoreTest, LiveIdsAreUniqueAndResolveIndependently) {
    const auto ids = this->FillToCapacity();

    const std::set<std::uint64_t> unique(ids.begin(), ids.end());
    EXPECT_EQ(unique.size(), ids.size()) << "the same id was handed out twice";

    std::set<lob::Order*> orders;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        lob::Order* order = this->store().get(ids[i]);
        ASSERT_NE(order, nullptr) << "i=" << i;
        EXPECT_EQ(order->price, TestFixture::PriceAt(i)) << "i=" << i;
        orders.insert(order);
    }
    EXPECT_EQ(orders.size(), ids.size()) << "two live ids resolve to the same order";
}

TYPED_TEST(OrderStoreTest, ExhaustsAtNumberOfOrders) {
    this->FillToCapacity();

    EXPECT_EQ(this->store().add(TestFixture::kMinPrice, 1), lob::invalid_order_id);
    EXPECT_EQ(this->store().add(TestFixture::kMinPrice, 1), lob::invalid_order_id)
        << "exhaustion must be stable, not one-shot";
}

TYPED_TEST(OrderStoreTest, RejectsIdsThatWereNeverIssued) {
    auto& store = this->store();

    const std::uint64_t live = store.add(TestFixture::kMinPrice, 1);
    ASSERT_NE(live, lob::invalid_order_id);

    EXPECT_EQ(store.get(lob::invalid_order_id), nullptr);
    EXPECT_FALSE(store.cancel(lob::invalid_order_id));

    // A position past the end of the pool must be refused, not dereferenced.
    const lob::Order_ID_Pack past_the_end(TestFixture::kOrders, 0);
    EXPECT_EQ(store.get(static_cast<std::uint64_t>(past_the_end)), nullptr);
    EXPECT_FALSE(store.cancel(static_cast<std::uint64_t>(past_the_end)));

    const lob::Order_ID_Pack way_past(lob::max_orders_per_lob, 12345);
    EXPECT_EQ(store.get(static_cast<std::uint64_t>(way_past)), nullptr);
    EXPECT_FALSE(store.cancel(static_cast<std::uint64_t>(way_past)));

    // An in-range position carrying the wrong epoch must be refused too.
    const lob::Order_ID_Pack wrong_epoch(lob::Order_ID_Pack(live).position, 999);
    EXPECT_EQ(store.get(static_cast<std::uint64_t>(wrong_epoch)), nullptr);
    EXPECT_FALSE(store.cancel(static_cast<std::uint64_t>(wrong_epoch)));

    // None of that may have disturbed the live order.
    lob::Order* order = store.get(live);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order->price, TestFixture::kMinPrice);
    EXPECT_TRUE(order->is_active);
}

TYPED_TEST(OrderStoreTest, CancelRemovesTheOrderAndRetiresItsId) {
    const std::uint64_t id = this->store().add(TestFixture::kMinPrice, 5);
    ASSERT_NE(id, lob::invalid_order_id);

    EXPECT_TRUE(this->store().cancel(id));
    EXPECT_EQ(this->store().get(id), nullptr) << "a cancelled id must stop resolving";
    EXPECT_FALSE(this->store().cancel(id)) << "double cancel must be rejected";

    EXPECT_EQ(this->store().getCheapest(), nullptr);
    EXPECT_EQ(this->store().getDearest(), nullptr);
}

TYPED_TEST(OrderStoreTest, ReusedSlotGetsAFreshId) {
    // Exhaust the arena first: while untouched memory is left, add() hands that
    // out instead of recycling, and the recycling path would go untested.
    const auto ids = this->FillToCapacity();
    const std::uint64_t first = ids.front();

    // Dirty the slot so the reset below is actually observable.
    lob::Order* before = this->store().get(first);
    ASSERT_NE(before, nullptr);
    before->executed_qty = 123;
    before->timestamp = 456;

    ASSERT_TRUE(this->store().cancel(first));

    const std::uint64_t second = this->store().add(TestFixture::kMinPrice, 9);
    ASSERT_NE(second, lob::invalid_order_id);

    EXPECT_NE(second, first) << "the recycled slot reused its old id";
    EXPECT_EQ(this->store().get(first), nullptr) << "the stale id came back to life";

    lob::Order* order = this->store().get(second);
    ASSERT_NE(order, nullptr);
    EXPECT_EQ(order, before) << "the freed slot was not the one recycled";
    EXPECT_EQ(order->quantity, 9u);
    EXPECT_EQ(order->executed_qty, 0u) << "recycled order kept stale state";
    EXPECT_EQ(order->timestamp, 0u) << "recycled order kept stale state";
    EXPECT_TRUE(order->is_active);
}

// Filling and emptying the whole store repeatedly must not leak slots or
// resurrect retired ids.
TYPED_TEST(OrderStoreTest, RepeatedFullCycles) {
    constexpr int kCycles = 3;
    std::set<std::uint64_t> retired;

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        const auto ids = this->FillToCapacity();
        ASSERT_EQ(this->store().add(TestFixture::kMinPrice, 1), lob::invalid_order_id)
            << "cycle=" << cycle;

        for (const std::uint64_t id : ids) {
            EXPECT_EQ(retired.count(id), 0u) << "id reissued after retirement, cycle=" << cycle;
        }

        for (const std::uint64_t id : ids) {
            ASSERT_TRUE(this->store().cancel(id)) << "cycle=" << cycle;
            retired.insert(id);
        }

        EXPECT_EQ(this->store().getCheapest(), nullptr) << "cycle=" << cycle;
        EXPECT_EQ(this->store().getDearest(), nullptr) << "cycle=" << cycle;
    }
}

// Emptying a bucket must clear its bit, so the next query skips it.
TYPED_TEST(OrderStoreTest, EmptiedBucketStopsBeingReported) {
    if (TestFixture::kOrders < 3) {
        GTEST_SKIP() << "needs room for two buckets plus a spare";
    }
    auto& store = this->store();

    const std::uint64_t low = store.add(TestFixture::kMinPrice, 1);
    const std::uint64_t high = store.add(TestFixture::kMaxPrice, 1);
    ASSERT_NE(low, lob::invalid_order_id);
    ASSERT_NE(high, lob::invalid_order_id);

    const std::size_t low_bucket = TestFixture::BucketOf(TestFixture::kMinPrice);
    const std::size_t high_bucket = TestFixture::BucketOf(TestFixture::kMaxPrice);

    ASSERT_TRUE(store.cancel(low));

    lob::Order* cheapest = store.getCheapest();
    ASSERT_NE(cheapest, nullptr);
    EXPECT_EQ(TestFixture::BucketOf(cheapest->price), high_bucket)
        << "the emptied low bucket is still reported";

    ASSERT_TRUE(store.cancel(high));
    EXPECT_EQ(store.getCheapest(), nullptr);

    // And the bucket must come back once it is used again.
    const std::uint64_t again = store.add(TestFixture::kMinPrice, 1);
    ASSERT_NE(again, lob::invalid_order_id);
    cheapest = store.getCheapest();
    ASSERT_NE(cheapest, nullptr);
    EXPECT_EQ(TestFixture::BucketOf(cheapest->price), low_bucket);
}

// Several orders in one bucket: the bucket stays reported until the last of
// them is gone, whatever order they are cancelled in.
TYPED_TEST(OrderStoreTest, BucketSurvivesUntilItsLastOrderIsCancelled) {
    const std::int64_t price = TestFixture::kMinPrice;
    const std::size_t count = std::min<std::size_t>(TestFixture::kOrders, 5);

    std::vector<std::uint64_t> ids;
    for (std::size_t i = 0; i < count; ++i) {
        const std::uint64_t id = this->store().add(price, 1);
        ASSERT_NE(id, lob::invalid_order_id) << "i=" << i;
        ids.push_back(id);
    }

    // Cancel back to front, leaving the first for last.
    for (std::size_t i = ids.size(); i-- > 1;) {
        ASSERT_TRUE(this->store().cancel(ids[i])) << "i=" << i;
        EXPECT_NE(this->store().getCheapest(), nullptr) << "bucket emptied too early, i=" << i;
    }

    ASSERT_TRUE(this->store().cancel(ids.front()));
    EXPECT_EQ(this->store().getCheapest(), nullptr);
}

// Randomised add/cancel against a model of the live book.
TYPED_TEST(OrderStoreTest, InterleavedAddCancelMatchesModel) {
    constexpr int kOps = 400;

    auto& store = this->store();
    std::map<std::uint64_t, std::int64_t> live;  // id -> price
    std::mt19937 rng(12345);
    std::bernoulli_distribution add_coin(0.6);

    for (int op = 0; op < kOps; ++op) {
        const bool can_add = live.size() < TestFixture::kOrders;
        const bool can_cancel = !live.empty();
        ASSERT_TRUE(can_add || can_cancel);

        if (can_add && (!can_cancel || add_coin(rng))) {
            const std::int64_t price = TestFixture::PriceAt(static_cast<std::size_t>(op));
            const std::uint64_t id = store.add(price, 1);
            ASSERT_NE(id, lob::invalid_order_id)
                << "exhausted with " << live.size() << " of " << TestFixture::kOrders
                << " live, op=" << op;
            ASSERT_EQ(live.count(id), 0u) << "reissued a live id, op=" << op;
            live.emplace(id, price);
        } else {
            std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
            auto it = live.begin();
            std::advance(it, pick(rng));
            ASSERT_TRUE(store.cancel(it->first)) << "op=" << op;
            live.erase(it);
        }

        // Every live id still resolves to its own order at its own price.
        for (const auto& [id, price] : live) {
            lob::Order* order = store.get(id);
            ASSERT_NE(order, nullptr) << "lost a live order, op=" << op;
            ASSERT_EQ(order->price, price) << "op=" << op;
        }

        lob::Order* cheapest = store.getCheapest();
        lob::Order* dearest = store.getDearest();
        if (live.empty()) {
            ASSERT_EQ(cheapest, nullptr) << "op=" << op;
            ASSERT_EQ(dearest, nullptr) << "op=" << op;
            continue;
        }

        std::size_t min_bucket = SIZE_MAX;
        std::size_t max_bucket = 0;
        for (const auto& [id, price] : live) {
            const std::size_t bucket = TestFixture::BucketOf(price);
            min_bucket = std::min(min_bucket, bucket);
            max_bucket = std::max(max_bucket, bucket);
        }

        ASSERT_NE(cheapest, nullptr) << "op=" << op;
        ASSERT_NE(dearest, nullptr) << "op=" << op;
        ASSERT_EQ(TestFixture::BucketOf(cheapest->price), min_bucket) << "op=" << op;
        ASSERT_EQ(TestFixture::BucketOf(dearest->price), max_bucket) << "op=" << op;
    }
}

}  // namespace
