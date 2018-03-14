#include <gtest/gtest.h>

#include "../morpheus/core/vec.h"

TEST(CoreTest, TestVec) {
    EXPECT_EQ(VINT(1,2,3) * 2, VINT(2,4,6));
}
