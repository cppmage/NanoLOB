// Behavioural suite shared by every free-list flavour (FIFO today, LIFO next).
//
// Ordering is deliberately NOT asserted: the only contract checked here is that
// the list is a faithful container -- everything pushed comes back out exactly
// once, and nothing else does. That keeps one suite valid for both flavours.
//
// Occupancy is managed by the caller, so no test ever pushes past the usable
// capacity or pops from an empty list.
//
// NOTE on capacity: FIFO_FreeList encodes "empty" as tail == head, so the ring
// can hold at most size - 1 elements -- pushing `size` of them wraps tail onto
// head and the full list reports itself empty. Config::kCapacity below is that
// usable maximum, and "fill completely" throughout this file means filling to
// it.

#include <gtest/gtest.h>

#include <lob/allocators/FIFO_FreeList.hpp>

#include <cstddef>
#include <random>
#include <set>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Distinct values per index, so a lost or duplicated element cannot hide behind
// an equal one.
// ---------------------------------------------------------------------------
struct Node {
    int payload;
};

constexpr std::size_t kMaxValues = 1u << 16;

Node* NodeAt(std::size_t index) {
    // Fixed size on purpose: growing it would invalidate pointers already
    // handed to a list under test.
    static std::vector<Node> storage(kMaxValues);
    if (index >= storage.size()) {
        ADD_FAILURE() << "test asked for more distinct values than the node pool holds";
        return nullptr;
    }
    return storage.data() + index;
}

template <class T>
struct ValueFactory;

template <>
struct ValueFactory<int> {
    static int Make(std::size_t index) { return static_cast<int>(index) + 1; }
};

template <>
struct ValueFactory<Node*> {
    static Node* Make(std::size_t index) { return NodeAt(index); }
};

// ---------------------------------------------------------------------------
// ADAPT HERE -- the only place that knows a concrete free-list API.
// A LIFO_FreeList with different spelling (free/allocate, a ctor argument
// instead of a template parameter, `bool pop(T&)`, a different usable
// capacity...) needs its own config struct like this one; every test below is
// written against these five members and needs no change.
// ---------------------------------------------------------------------------
template <typename T, std::size_t Size>
struct FifoConfig {
    using Value = T;
    using List = lob::FIFO_FreeList<T, Size>;

    // One slot is spent distinguishing full from empty -- see the note above.
    static constexpr std::size_t kCapacity = Size - 1;

    static List Make() { return List(); }
    static void Push(List& list, const Value& v) { list.push(v); }
    static Value Pop(List& list) { return list.pop(); }
    static bool Empty(const List& list) { return list.empty(); }
};

// ---------------------------------------------------------------------------
// The suite.
// ---------------------------------------------------------------------------
template <class Config>
class FreeListTest : public ::testing::Test {
protected:
    using List = typename Config::List;
    using Value = typename Config::Value;
    using Factory = ValueFactory<Value>;

    static constexpr std::size_t kCapacity = Config::kCapacity;

    // Pushes `count` fresh values and returns what was pushed.
    std::multiset<Value> PushFresh(List& list, std::size_t count) {
        std::multiset<Value> pushed;
        for (std::size_t i = 0; i < count; ++i) {
            const Value v = Factory::Make(next_value_++);
            Config::Push(list, v);
            pushed.insert(v);
        }
        return pushed;
    }

    // Pops exactly `count` elements and returns them.
    std::multiset<Value> PopExactly(List& list, std::size_t count) {
        std::multiset<Value> popped;
        for (std::size_t i = 0; i < count; ++i) {
            popped.insert(Config::Pop(list));
        }
        return popped;
    }

    // Moves the internal read/write positions forward by `offset` without
    // changing the occupancy -- this is what sets up a wrap-around.
    void Rotate(List& list, std::size_t offset) {
        for (std::size_t i = 0; i < offset; ++i) {
            Config::Push(list, Factory::Make(next_value_++));
            Config::Pop(list);
        }
    }

    // A fresh value index per test, so values never repeat within a test.
    std::size_t next_value_ = 0;
};

// Add LIFO_FreeList configs here and the whole file runs against them too.
using FreeListTypes = ::testing::Types<
    FifoConfig<int, 4>,     // smallest useful ring: holds exactly one element
    FifoConfig<Node*, 64>
    // , LifoConfig<int, 64>, LifoConfig<Node*, 64>
    >;

TYPED_TEST_SUITE(FreeListTest, FreeListTypes);

TYPED_TEST(FreeListTest, StartsEmpty) {
    auto list = TypeParam::Make();
    EXPECT_TRUE(TypeParam::Empty(list));
}

TYPED_TEST(FreeListTest, SingleElementRoundTrip) {
    auto list = TypeParam::Make();
    const auto pushed = this->PushFresh(list, 1);

    EXPECT_FALSE(TypeParam::Empty(list));
    EXPECT_EQ(this->PopExactly(list, 1), pushed);
    EXPECT_TRUE(TypeParam::Empty(list));
}

// Fill to exactly the usable capacity, then drain to exactly empty.
TYPED_TEST(FreeListTest, FillToCapacityThenDrain) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;

    auto list = TypeParam::Make();

    const auto pushed = this->PushFresh(list, kCapacity);
    EXPECT_FALSE(TypeParam::Empty(list));

    EXPECT_EQ(this->PopExactly(list, kCapacity), pushed);
    EXPECT_TRUE(TypeParam::Empty(list));
}

// A drained list must be reusable, and must not resurrect stale elements.
TYPED_TEST(FreeListTest, RepeatedFillDrainCycles) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;
    constexpr int kCycles = 3;

    auto list = TypeParam::Make();

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        const auto pushed = this->PushFresh(list, kCapacity);
        EXPECT_EQ(this->PopExactly(list, kCapacity), pushed) << "cycle=" << cycle;
        EXPECT_TRUE(TypeParam::Empty(list)) << "cycle=" << cycle;
    }
}

// Wrap-around, the headline case for a power-of-two ring: shift the internal
// positions by a partial offset, then fill completely so the run of live
// elements straddles the end of the backing array.
TYPED_TEST(FreeListTest, WrapAroundAfterPartialRotation) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;

    // An offset that does not divide the capacity guarantees the next fill
    // crosses the array boundary mid-way.
    const std::size_t offset = (kCapacity / 2) + 1;

    auto list = TypeParam::Make();
    this->Rotate(list, offset);
    ASSERT_TRUE(TypeParam::Empty(list));

    const auto pushed = this->PushFresh(list, kCapacity);
    EXPECT_EQ(this->PopExactly(list, kCapacity), pushed) << "offset=" << offset;
    EXPECT_TRUE(TypeParam::Empty(list));
}

// The same, but exhaustively: every possible starting position inside the ring
// must survive a full fill/drain.
TYPED_TEST(FreeListTest, WrapAroundAtEveryOffset) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;

    // kCapacity + 1 is the backing array size, i.e. one full revolution.
    for (std::size_t offset = 0; offset <= kCapacity; ++offset) {
        auto list = TypeParam::Make();
        this->Rotate(list, offset);
        ASSERT_TRUE(TypeParam::Empty(list)) << "offset=" << offset;

        const auto pushed = this->PushFresh(list, kCapacity);
        EXPECT_EQ(this->PopExactly(list, kCapacity), pushed) << "offset=" << offset;
        EXPECT_TRUE(TypeParam::Empty(list)) << "offset=" << offset;
    }
}

// Wrap-around while the list stays occupied the whole time: the live window
// slides across the boundary instead of being emptied at it.
TYPED_TEST(FreeListTest, WrapAroundWhileHalfFull) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;
    using Value = typename TestFixture::Value;

    if (kCapacity < 2) {
        GTEST_SKIP() << "needs room to hold an element back while sliding";
    }

    auto list = TypeParam::Make();
    std::multiset<Value> live = this->PushFresh(list, kCapacity / 2);

    // Slide one element at a time well past the end of the backing array.
    for (std::size_t step = 0; step < (kCapacity + 1) * 2 + 1; ++step) {
        const auto pushed = this->PushFresh(list, 1);
        live.insert(pushed.begin(), pushed.end());

        const Value v = TypeParam::Pop(list);
        const auto it = live.find(v);
        ASSERT_NE(it, live.end())
            << "popped an element that was never pushed (or popped twice), step=" << step;
        live.erase(it);

        ASSERT_FALSE(TypeParam::Empty(list)) << "step=" << step;
    }

    EXPECT_EQ(this->PopExactly(list, live.size()), live);
    EXPECT_TRUE(TypeParam::Empty(list));
}

// Randomised push/pop mix checked against a model, staying strictly inside
// [0, kCapacity] occupancy. Fixed seed keeps failures reproducible.
TYPED_TEST(FreeListTest, InterleavedPushPopMatchesModel) {
    constexpr std::size_t kCapacity = TestFixture::kCapacity;
    using Value = typename TestFixture::Value;

    constexpr int kOps = 2000;

    auto list = TypeParam::Make();
    std::multiset<Value> model;

    std::mt19937 rng(12345);
    std::bernoulli_distribution push_coin(0.5);

    for (int op = 0; op < kOps; ++op) {
        const bool can_push = model.size() < kCapacity;
        const bool can_pop = !model.empty();
        ASSERT_TRUE(can_push || can_pop);

        const bool do_push = can_push && (!can_pop || push_coin(rng));

        if (do_push) {
            const auto pushed = this->PushFresh(list, 1);
            model.insert(pushed.begin(), pushed.end());
        } else {
            const Value v = TypeParam::Pop(list);
            const auto it = model.find(v);
            ASSERT_NE(it, model.end())
                << "popped an element that was never pushed (or popped twice), op=" << op;
            model.erase(it);
        }

        ASSERT_EQ(TypeParam::Empty(list), model.empty()) << "op=" << op;
    }

    // Whatever is still inside must come out intact.
    EXPECT_EQ(this->PopExactly(list, model.size()), model);
    EXPECT_TRUE(TypeParam::Empty(list));
}

}  // namespace
