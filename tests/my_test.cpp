#include <gtest/gtest.h>
#include "lru_cach.hpp"

TEST(MyCache, RepeatedPage)
{
    Lru_cach<int, int> cache(2);

    EXPECT_EQ(cache.look_up(7), nullptr);

    cache.insert_value(7, 707);
    const auto* value = cache.look_up(7);

    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 707);
}

