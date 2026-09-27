#include "tests/support/cache_checks.hpp"

TEST(Lirs, CommonContract)
{
    for(std::size_t c : {0, 1, 4})
    {
        Lirs_cach<int, int> cache(c, c ? 1 : 0);
        common_contract(cache);
    }
}
TEST(Lirs, Quotas)
{
    using Cache = Lirs_cach<int, int>;
    EXPECT_NO_THROW(Cache(0, 0));
    EXPECT_NO_THROW(Cache(1, 0));
    EXPECT_NO_THROW(Cache(1, 1));
    EXPECT_THROW(Cache(0, 1), std::invalid_argument);
    EXPECT_THROW(Cache(1, 2), std::invalid_argument);
    EXPECT_THROW(Cache(3, 0), std::invalid_argument);
    EXPECT_THROW(Cache(3, 3), std::invalid_argument);
}
TEST(Lirs, CapacityOneAndHistory)
{
    for(std::size_t h : {0, 1})
    {
        Lirs_cach<int, int> cache(1, h);
        int previous = -1;
        for(int key : {1, 1, 2, 1, 3, 2, 1, 1})
        {
            EXPECT_EQ(request(cache, key), previous == key);
            previous = key;
            EXPECT_TRUE(queue_keys(cache.snapshot(), "S").empty());
            check_state(cache);
        }
    }
}
TEST(Lirs, HirPromotionDemotionAndPruning)
{
    Lirs_cach<int, int> cache(3, 1);
    for(int k : {1, 2, 3})
    {
        request(cache, k);
    }
    EXPECT_EQ(queue_keys(cache.snapshot(), "Q"), std::vector<int>({3}));
    request(cache, 3); // HIR из S становится LIR; нижний LIR 1 становится HIR вне S.
    EXPECT_EQ(queue_keys(cache.snapshot(), "Q"), std::vector<int>({1}));
    EXPECT_EQ(queue_keys(cache.snapshot(), "S"), std::vector<int>({3, 2}));
    request(cache, 1); // HIR вне S добавляется в S, оставаясь в Q.
    EXPECT_EQ(queue_keys(cache.snapshot(), "Q"), std::vector<int>({1}));
    request(cache, 1); // Второй доступ повышает его в LIR.
    EXPECT_EQ(queue_keys(cache.snapshot(), "Q"), std::vector<int>({2}));
    check_state(cache);
}
TEST(Lirs, GhostReturnAndNewValue)
{
    Lirs_cach<int, int> cache(3, 1);
    for(int k : {1, 2, 3, 4})
    {
        request(cache, k);
    }
    EXPECT_EQ(cache.look_up(3), nullptr);
    EXPECT_FALSE(cache.extract_entry(3));
    EXPECT_FALSE(cache.erase_key(3));
    auto victim = cache.insert_value(3, 333);
    ASSERT_TRUE(victim);
    EXPECT_EQ(victim->key, 4);
    const auto state = cache.snapshot();
    const auto found = std::find_if(state.resident.begin(), state.resident.end(),
                                   [](const auto& entry) { return entry.key == 3; });
    ASSERT_NE(found, state.resident.end());
    EXPECT_EQ(found->value, 333);
    check_state(cache);
}
TEST(Lirs, ExternalRemovalAndRefill)
{
    Lirs_cach<int, int> cache(4, 1);
    for(int k : {1, 2, 3, 4})
    {
        request(cache, k);
    }
    EXPECT_TRUE(cache.erase_key(2));
    EXPECT_TRUE(cache.extract_entry(4));
    for(int k : {5, 2, 4, 3, 6, 2})
    {
        request(cache, k);
        check_state(cache);
    }
}
TEST(Lirs, MixedOperations)
{
    for(std::size_t c : {1, 2, 4, 8})
    {
        for(std::size_t h = 1; h < std::max<std::size_t>(2, c); ++h)
        {
            Lirs_cach<int, int> cache(c, h);
            mixed_operations(cache, 42);
        }
    }
}
TEST(Lirs, StringKeyAndValue)
{
    Lirs_cach<std::string, std::string> cache(2, 1);
    cache.insert_value("a", "alpha");
    auto value = cache.extract_entry("a");
    ASSERT_TRUE(value);
    EXPECT_EQ(value->value, "alpha");
}
TEST(Lirs, FailedInsertResetsToUsableState)
{
    Lirs_cach<int, Throwing_Value> cache(2, 1);
    Throwing_Value::copies_left = 0;
    EXPECT_THROW(cache.insert_value(1, Throwing_Value(101)), std::runtime_error);
    Throwing_Value::copies_left = -1;
    EXPECT_EQ(cache.size(), 0u);
    EXPECT_TRUE(cache.snapshot().consistent);
    EXPECT_NO_THROW(cache.insert_value(2, Throwing_Value(202)));
    const auto state = cache.snapshot();
    ASSERT_EQ(state.resident.size(), 1u);
    EXPECT_EQ(state.resident.front().key, 2);
    EXPECT_EQ(state.resident.front().value.value, 202);
}

TEST(Lirs, HitAfterRemovingLastLir)
{
    Lirs_cach<int, int> cache(3, 2);
    for(int key : {1, 2, 3})
    {
        request(cache, key);
    }
    ASSERT_TRUE(cache.erase_key(1));
    ASSERT_TRUE(queue_keys(cache.snapshot(), "S").empty());
    ASSERT_NE(cache.look_up(2), nullptr);
    EXPECT_TRUE(cache.snapshot().consistent);
    EXPECT_EQ(cache.snapshot().lir_count, 1u);
    EXPECT_EQ(cache.snapshot().hir_count, 1u);
}
