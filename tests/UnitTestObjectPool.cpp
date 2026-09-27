// ObjectPool behaviour, written against the free-list flavour as a parameter so
// the same suite covers FIFO today and LIFO later.
//
// The pool hands out `size` distinct objects, then recycles whatever was freed.
// Allocation order is NOT asserted beyond one rule the pool does promise:
// untouched arena memory is handed out before anything is recycled.

#include <gtest/gtest.h>

#include <lob/allocators/FIFO_FreeList.hpp>
#include <lob/allocators/ObjectPool.hpp>
#include <lob/parameters/parameters.hpp>

#include <cstddef>
#include <random>
#include <set>
#include <vector>

namespace {

struct Node {
    int payload;
};

// ---------------------------------------------------------------------------
// ADAPT HERE -- add a config per free-list flavour and every test below runs
// against it unchanged.
// ---------------------------------------------------------------------------
template <template <typename, lob::pool_size_t> typename FreeList, lob::pool_size_t Size>
struct PoolConfig {
    static constexpr lob::pool_size_t kSize = Size;
    using Pool = lob::ObjectPool<Node, Size, FreeList>;
};

template <class Config>
class ObjectPoolTest : public ::testing::Test {
protected:
    using Pool = typename Config::Pool;
    static constexpr lob::pool_size_t kSize = Config::kSize;

    // Allocates `count` objects, failing the test on an unexpected exhaustion.
    std::vector<Node*> AllocateMany(Pool& pool, std::size_t count) {
        std::vector<Node*> out;
        out.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            Node* p = pool.allocate();
            EXPECT_NE(p, nullptr) << "pool exhausted early at i=" << i;
            out.push_back(p);
        }
        return out;
    }

    static std::multiset<Node*> AsSet(const std::vector<Node*>& v) {
        return std::multiset<Node*>(v.begin(), v.end());
    }
};

using PoolTypes = ::testing::Types<
    PoolConfig<lob::FIFO_FreeList, 64>
    // , PoolConfig<lob::LIFO_FreeList, 64>
    >;

TYPED_TEST_SUITE(ObjectPoolTest, PoolTypes);

// Every object the pool hands out is a distinct, usable address.
TYPED_TEST(ObjectPoolTest, AllocatesDistinctObjectsUpToSize) {
    constexpr auto kSize = TestFixture::kSize;
    typename TestFixture::Pool pool;

    const auto all = this->AllocateMany(pool, kSize);
    const std::set<Node*> unique(all.begin(), all.end());

    EXPECT_EQ(unique.size(), kSize) << "pool handed out the same object twice";
}

// Allocated objects must not alias: writing one must not disturb another.
TYPED_TEST(ObjectPoolTest, AllocatedObjectsAreIndependentStorage) {
    constexpr auto kSize = TestFixture::kSize;
    typename TestFixture::Pool pool;

    const auto all = this->AllocateMany(pool, kSize);
    for (std::size_t i = 0; i < all.size(); ++i) {
        all[i]->payload = static_cast<int>(i) + 1;
    }
    for (std::size_t i = 0; i < all.size(); ++i) {
        EXPECT_EQ(all[i]->payload, static_cast<int>(i) + 1) << "objects alias at i=" << i;
    }
}

// Past `size` allocations with nothing freed, the pool reports exhaustion
// instead of handing out something it does not own.
TYPED_TEST(ObjectPoolTest, ReportsExhaustionWithNullptr) {
    constexpr auto kSize = TestFixture::kSize;
    typename TestFixture::Pool pool;

    this->AllocateMany(pool, kSize);

    EXPECT_EQ(pool.allocate(), nullptr);
    EXPECT_EQ(pool.allocate(), nullptr) << "exhaustion must be stable, not one-shot";
}

// The documented ordering rule: while untouched arena memory is left, a freed
// object is not recycled yet.
TYPED_TEST(ObjectPoolTest, PrefersFreshMemoryOverRecycling) {
    constexpr auto kSize = TestFixture::kSize;
    if (kSize < 2) {
        GTEST_SKIP() << "needs a second fresh slot to prefer";
    }

    typename TestFixture::Pool pool;

    Node* first = pool.allocate();
    ASSERT_NE(first, nullptr);
    pool.free(first);

    Node* second = pool.allocate();
    ASSERT_NE(second, nullptr);
    EXPECT_NE(second, first) << "recycled while fresh arena memory was still available";
}

// Once the arena is exhausted, a freed object comes back.
TYPED_TEST(ObjectPoolTest, RecyclesAfterExhaustion) {
    constexpr auto kSize = TestFixture::kSize;
    typename TestFixture::Pool pool;

    const auto all = this->AllocateMany(pool, kSize);
    ASSERT_EQ(pool.allocate(), nullptr);

    Node* freed = all.front();
    pool.free(freed);

    EXPECT_EQ(pool.allocate(), freed);
    EXPECT_EQ(pool.allocate(), nullptr) << "recycled more than was freed";
}

// The sizing guarantee: freeing the entire pool at once must not lose or
// duplicate a single pointer.
TYPED_TEST(ObjectPoolTest, FreeWholePoolThenReallocateAll) {
    constexpr auto kSize = TestFixture::kSize;
    typename TestFixture::Pool pool;

    const auto first_round = this->AllocateMany(pool, kSize);
    for (Node* p : first_round) {
        pool.free(p);
    }

    const auto second_round = this->AllocateMany(pool, kSize);

    EXPECT_EQ(TestFixture::AsSet(second_round), TestFixture::AsSet(first_round))
        << "recycled set differs from what was freed";
    EXPECT_EQ(pool.allocate(), nullptr);
}

// ...and it has to keep working round after round.
TYPED_TEST(ObjectPoolTest, RepeatedFullCycles) {
    constexpr auto kSize = TestFixture::kSize;
    constexpr int kCycles = 4;

    typename TestFixture::Pool pool;
    const auto owned = TestFixture::AsSet(this->AllocateMany(pool, kSize));

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        std::vector<Node*> live(owned.begin(), owned.end());
        for (Node* p : live) {
            pool.free(p);
        }

        const auto again = this->AllocateMany(pool, kSize);
        ASSERT_EQ(TestFixture::AsSet(again), owned) << "cycle=" << cycle;
        ASSERT_EQ(pool.allocate(), nullptr) << "cycle=" << cycle;
    }
}

// Randomised allocate/free mix against a model of which objects are live.
// Fixed seed keeps failures reproducible.
TYPED_TEST(ObjectPoolTest, InterleavedAllocateFreeNeverLeaksOrAliases) {
    constexpr auto kSize = TestFixture::kSize;
    constexpr int kOps = 2000;

    typename TestFixture::Pool pool;
    std::set<Node*> live;
    std::set<Node*> ever_seen;

    std::mt19937 rng(12345);
    std::bernoulli_distribution alloc_coin(0.5);

    for (int op = 0; op < kOps; ++op) {
        const bool can_alloc = live.size() < kSize;
        const bool can_free = !live.empty();
        ASSERT_TRUE(can_alloc || can_free);

        if (can_alloc && (!can_free || alloc_coin(rng))) {
            Node* p = pool.allocate();
            ASSERT_NE(p, nullptr) << "exhausted while " << live.size() << " of " << kSize
                                  << " were live, op=" << op;
            ASSERT_EQ(live.count(p), 0u)
                << "handed out an object that is already live, op=" << op;
            live.insert(p);
            ever_seen.insert(p);
        } else {
            // Free an arbitrary live object.
            std::uniform_int_distribution<std::size_t> pick(0, live.size() - 1);
            auto it = live.begin();
            std::advance(it, pick(rng));
            pool.free(*it);
            live.erase(it);
        }
    }

    EXPECT_LE(ever_seen.size(), kSize) << "pool handed out more objects than it owns";
}

}  // namespace
