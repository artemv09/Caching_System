#include "tests/support/cache_checks.hpp"

TEST(Lru, CommonContract)
{
    for(std::size_t c : {0, 1, 4})
    {
        Lru_cach<int, int> cache(c);
        common_contract(cache);
    }
}
TEST(Lru, HitChangesVictimAndDeleteEnds)
{
    Lru_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);
    EXPECT_TRUE(request(cache, 1));
    auto victim = cache.insert_value(3, 303);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 2);
    EXPECT_EQ(victim->value, 202);
    EXPECT_TRUE(cache.erase_key(3));
    EXPECT_TRUE(cache.erase_key(1));
    EXPECT_EQ(cache.size(), 0u);
}
TEST(Lru, DuplicateDoesNotChangePriority)
{
    Lru_cach<int, int> cache(2);
    request(cache, 1);
    request(cache, 2);
    cache.insert_value(1, 999);
    auto victim = cache.insert_value(3, 303);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 1);
}
TEST(Lru, ReferenceModel)
{
    for(std::size_t c : {0, 1, 2, 7})
    {
        Lru_cach<int, int> cache(c);
        compare_simple_model(cache, false, 42);
    }
}
TEST(Lru, MixedOperations)
{
    Lru_cach<int, int> cache(4);
    mixed_operations(cache, 2026);
}
TEST(Lru, StringsAndOwnedExtraction)
{
    Erase_ELL<std::string, std::string> result;
    {
        Lru_cach<std::string, std::string> cache(1);
        cache.insert_value("key", "value");
        result = cache.extract_entry("key");
    }
    ASSERT_TRUE(result);
    EXPECT_EQ(result->key, "key");
    EXPECT_EQ(result->value, "value");
}
TEST(Lru, CopyFailureLeavesOriginal)
{
    Lru_cach<int, Throwing_Value> cache(1);
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
