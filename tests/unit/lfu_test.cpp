#include "tests/support/cache_checks.hpp"

TEST(Lfu, CommonContract)
{
    for(std::size_t c : {0, 1, 4})
    {
        Lfu_cach<int, int> cache(c);

        common_contract(cache);
    }
}
TEST(Lfu, FrequencyAndLruTie)
{
    Lfu_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);
    request(cache, 1);

    auto victim = cache.insert_value(3, 303);

    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 2);

    request(cache, 3); // Частоты 1 и 3 равны, раньше обращались к 1.

    victim = cache.insert_value(4, 404);

    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 1);
}
TEST(Lfu, GapsAndMinimumAfterErasing)
{
    Lfu_cach<int, int> cache(3);
    request(cache, 1);
    request(cache, 2);
    request(cache, 3);

    for(int i = 0; i < 8; ++i)
    {
        request(cache, 1);
    }
    
    for(int i = 0; i < 3; ++i)
    {
        request(cache, 2);
    }

    EXPECT_TRUE(cache.erase_key(3));

    check_state(cache);

    cache.insert_value(4, 404);
    auto victim = cache.insert_value(5, 505);

    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 4);

    cache.erase_key(1);
    cache.erase_key(2);
    cache.erase_key(5);
    request(cache, 9);

    check_state(cache);
}
TEST(Lfu, DuplicateDoesNotIncreaseFrequency)
{
    Lfu_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);

    cache.insert_value(1, 999);
    auto victim = cache.insert_value(3, 303);

    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 1);
}
TEST(Lfu, ReferenceModel)
{
    for(std::size_t c : {0, 1, 2, 7})
    {
        Lfu_cach<int, int> cache(c);

        compare_simple_model(cache, true, 2026);
    }
}
TEST(Lfu, MixedOperations)
{
    Lfu_cach<int, int> cache(4);

    mixed_operations(cache, 42);
}
TEST(Lfu, StringKeyAndValue)
{
    Lfu_cach<std::string, std::string> cache(1);
    cache.insert_value("a", "alpha");

    auto victim = cache.insert_value("b", "beta");

    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->value, "alpha");
}
TEST(Lfu, CopyFailureLeavesOriginal)
{
    Lfu_cach<int, Throwing_Value> cache(1);
    cache.insert_value(1, Throwing_Value(101));
    Throwing_Value::copies_left = 0;

    EXPECT_THROW(cache.insert_value(2, Throwing_Value(202)), std::runtime_error);

    Throwing_Value::copies_left = -1;

    const auto state = cache.snapshot();
    ASSERT_EQ(state.resident.size(), 1u);
    EXPECT_EQ(state.resident.front().key, 1);
    EXPECT_EQ(state.resident.front().value.value, 101);
    EXPECT_TRUE(cache.snapshot().consistent);
}
