#include "tests/support/cache_checks.hpp"

TEST(TwoQ, CommonContract)
{
    for(std::size_t c : {0, 1, 4})
    {
        Two_Q_Cach<int, int> cache(c);
        common_contract(cache);
    }
}
TEST(TwoQ, FifoHitAndWarmup)
{
    Two_Q_Cach<int, int> cache(4);
    for(int k : {1, 2, 3, 4})
    {
        request(cache, k);
    }
    EXPECT_EQ(queue_keys(cache.snapshot(), "A1in").size(), 4u); // Kin=1, но прогрев использует весь кеш.
    request(cache, 1);
    auto victim = cache.insert_value(5, 505);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 1);
    EXPECT_EQ(cache.look_up(1), nullptr);
    EXPECT_FALSE(cache.extract_entry(1));
    EXPECT_FALSE(cache.erase_key(1));
}
TEST(TwoQ, GhostReturnAndAmLru)
{
    Two_Q_Cach<int, int> cache(4);
    for(int k : {1, 2, 3, 4, 5})
    {
        request(cache, k);
    }
    cache.insert_value(1, 111);
    cache.insert_value(2, 222);
    cache.insert_value(3, 333);
    const auto* value = cache.look_up(1); // Единственное обращение делает 1 самым новым в Am.
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 111);
    auto victim = cache.insert_value(4, 444);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 2);
    EXPECT_EQ(victim->value, 222);
    check_state(cache);
}
TEST(TwoQ, HistoryLimitAndExtract)
{
    Two_Q_Cach<int, int> cache(2);
    for(int k : {1, 2, 3, 4})
    {
        request(cache, k);
    }
    EXPECT_EQ(queue_keys(cache.snapshot(), "A1out"), std::vector<int>({2}));
    auto extracted = cache.extract_entry(3);
    ASSERT_TRUE(extracted);
    EXPECT_EQ(queue_keys(cache.snapshot(), "A1out"), std::vector<int>({3}));
    cache.insert_value(3, 333);
    EXPECT_EQ(queue_keys(cache.snapshot(), "Am"), std::vector<int>({3}));
}
TEST(TwoQ, ZeroHistoryAndMixedOperations)
{
    for(std::size_t c : {1, 2, 4})
    {
        Two_Q_Cach<int, int> cache(c);
        mixed_operations(cache, 42);
    }
}
TEST(TwoQ, StringKeyAndValue)
{
    Two_Q_Cach<std::string, std::string> cache(1);
    cache.insert_value("a", "alpha");
    auto result = cache.insert_value("b", "beta");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value, "alpha");
}
TEST(TwoQ, FailedLoadTrimsGhost)
{
    Two_Q_Cach<int, Throwing_Value> cache(1);
    cache.insert_value(1, Throwing_Value(101));
    Throwing_Value::copies_left = 1;
    EXPECT_THROW(cache.insert_value(2, Throwing_Value(202)), std::runtime_error);
    Throwing_Value::copies_left = -1;
    EXPECT_TRUE(cache.snapshot().consistent);
    EXPECT_TRUE(queue_keys(cache.snapshot(), "A1out").empty());
    cache.insert_value(3, Throwing_Value(303));
    EXPECT_EQ(cache.size(), 1u);
}
