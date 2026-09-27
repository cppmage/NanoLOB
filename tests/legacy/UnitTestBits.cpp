#include <bitset/Bitset.h>
#include <gtest/gtest.h>
#include <array>

using namespace lob;

class BitsTest : public ::testing::Test {
protected:
    Bitset<1290> bits;
};

TEST_F(BitsTest, BitsSetReset) {

    auto arr = { 1290, 1198,999,324,222,44,43,42,41,40,10,1,0 };

    for (size_t el : arr) {
        bits.set(el);
        EXPECT_EQ(bits.firstNotZeroBit(), el);
    }
    
    for (auto it = std::prev(arr.end()); ; it--) {
        if (it == arr.begin()) {
            break;
        }
        bits.reset(*it);
        EXPECT_EQ(bits.firstNotZeroBit(), *(it-1));
    }
    


}

TEST_F(BitsTest, BitsSetResetReverse) {

    auto arr = {0, 1,10,40,41,42,43,44,45,46,222,324,999,1198,1290};
    for (size_t el : arr) {
        bits.set(el);
        EXPECT_EQ(bits.lastNotZeroBit(), el);
    }

    for (auto it = std::prev(arr.end()); ; it--) {
        if (it == arr.begin()) {
            break;
        }
        bits.reset(*it);
        EXPECT_EQ(bits.lastNotZeroBit(), *(it - 1));
    }



}