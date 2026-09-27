#include "tests/support/cache_checks.hpp"

TEST(Arc, CommonContract)
{
    for(std::size_t c : {0, 1, 4})
    {
        Arc_cach<int, int> cache(c);
        common_contract(cache);
    }
}
TEST(Arc, DuplicateDoesNotPromote)
{
    Arc_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);
    const auto before = cache.snapshot();
    cache.insert_value(1, 999);
    EXPECT_EQ(cache.snapshot().queues, before.queues);
    auto victim = cache.insert_value(3, 303);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 1);
}
TEST(Arc, AdaptiveGhostTransitions)
{
    Arc_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);
    request(cache, 1);
    request(cache, 3);
    EXPECT_EQ(queue_keys(cache.snapshot(), "B1"), std::vector<int>({2}));
    EXPECT_EQ(cache.look_up(2), nullptr);
    EXPECT_FALSE(cache.extract_entry(2));
    cache.insert_value(2, 222);
    EXPECT_EQ(cache.snapshot().target, 1u);
    EXPECT_EQ(queue_keys(cache.snapshot(), "B2"), std::vector<int>({1}));
    auto victim = cache.insert_value(1, 111); // B2: p=0, непустой T1 отдаёт 3.
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 3);
    EXPECT_EQ(cache.snapshot().target, 0u);
    const auto state = cache.snapshot();
    const auto found = std::find_if(state.resident.begin(), state.resident.end(),
                                   [](const auto& entry) { return entry.key == 1; });
    ASSERT_NE(found, state.resident.end());
    EXPECT_EQ(found->value, 111);
    check_state(cache);
}
TEST(Arc, EmptyT1Regression)
{
    for(std::size_t c : {1, 2})
    {
        Arc_cach<int, int> cache(c);
        auto keys = c == 1 ? std::vector<int>{1, 1, 2, 2, 1} : std::vector<int>{1, 2, 1, 3, 2, 3, 1};
        for(int k : keys)
        {
            request(cache, k);
            check_state(cache);
        }
    }
}
TEST(Arc, ExtractCreatesGhostEraseDoesNot)
{
    Arc_cach<int, int> cache(3);
    request(cache, 1);
    request(cache, 2);
    request(cache, 2);
    cache.extract_entry(1);
    cache.extract_entry(2);
    EXPECT_EQ(queue_keys(cache.snapshot(), "B1"), std::vector<int>({1}));
    EXPECT_EQ(queue_keys(cache.snapshot(), "B2"), std::vector<int>({2}));
    EXPECT_FALSE(cache.erase_key(1));
    cache.insert_value(1, 111);
    cache.erase_key(1);
    EXPECT_TRUE(queue_keys(cache.snapshot(), "B1").empty());
    check_state(cache);
}
TEST(Arc, MixedOperations)
{
    for(std::size_t c : {1, 2, 4})
    {
        Arc_cach<int, int> cache(c);
        mixed_operations(cache, 2026);
    }
}
TEST(Arc, StringKeyAndValue)
{
    Arc_cach<std::string, std::string> cache(1);
    cache.insert_value("a", "alpha");
    auto result = cache.extract_entry("a");
    ASSERT_TRUE(result);
    EXPECT_EQ(result->value, "alpha");
    EXPECT_EQ(cache.look_up("a"), nullptr);
    cache.insert_value("a", "new");
    const auto state = cache.snapshot();
    ASSERT_EQ(state.resident.size(), 1u);
    EXPECT_EQ(state.resident.front().key, "a");
    EXPECT_EQ(state.resident.front().value, "new");
}
TEST(Arc, B2ReturnAtEqualPositiveTarget)
{
    Arc_cach<int, int> cache(3);
    for(int key : {1, 1, 2, 3, 4, 2, 3})
    {
        request(cache, key);
    }
    ASSERT_EQ(cache.snapshot().target, 2u);
    ASSERT_EQ(queue_keys(cache.snapshot(), "T1"), std::vector<int>({4}));
    ASSERT_EQ(queue_keys(cache.snapshot(), "B2"), std::vector<int>({1}));
    // Возврат 1 из B2 уменьшает p до 1. При T1=p жертва берётся из T1.
    auto victim = cache.insert_value(1, 111);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 4);
    EXPECT_EQ(victim->value, 404);
    EXPECT_EQ(cache.snapshot().target, 1u);
    EXPECT_EQ(queue_keys(cache.snapshot(), "T2").front(), 1);
    check_state(cache);
}
TEST(Arc, FailedGhostLoadKeepsBoundedHistory)
{
    Arc_cach<int, Throwing_Value> cache(2);
    for(int key : {1, 2, 1, 3, 2, 3, 4, 1, 2, 5, 2, 6})
    {
        if(!cache.look_up(key))
        {
            cache.insert_value(key, Throwing_Value(key * 101));
        }
    }
    Throwing_Value::copies_left = 1;
    EXPECT_THROW(cache.insert_value(1, Throwing_Value(111)), std::runtime_error);
    Throwing_Value::copies_left = -1;
    EXPECT_TRUE(cache.snapshot().consistent);
    for(int key : {1, 2, 3, 4})
    {
        if(!cache.look_up(key))
        {
            cache.insert_value(key, Throwing_Value(key * 101));
        }
        EXPECT_TRUE(cache.snapshot().consistent);
    }
}
