#include "tests/support/hierarchy_checks.hpp"

using Inclusive = Multi_Level_Cach<int, int, Cach_Mode::Inclusive>;

TEST(Inclusive, RejectEmptyAndZero)
{
    auto data = test_data();

    EXPECT_THROW(Inclusive({}, data), std::invalid_argument);
    EXPECT_THROW(Inclusive((std::vector<Cache_name_size>{{"LRU", 1, 0}, {"LRU", 0, 0}}), data),
                 std::invalid_argument);
    EXPECT_NO_THROW(Inclusive((std::vector<Cache_name_size>{{"LRU", 4, 0}, {"LRU", 2, 0}}), data));
}
TEST(Inclusive, L1L2L3Promotion)
{
    auto data = test_data();
    std::vector<Cache_name_size> p{{"LRU", 1, 0}, {"LFU", 2, 0}, {"ARC", 3, 0}};
    Inclusive cache(p, data);
    
    for(int k : {1, 2, 3})
    {
        EXPECT_EQ(hierarchy_step(cache, p, k), -1);
    }

    expect_residents(cache, {{3}, {2, 3}, {1, 2, 3}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 2);

    expect_residents(cache, {{1}, {1, 3}, {1, 2, 3}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 0);
    EXPECT_EQ(hierarchy_step(cache, p, 3), 1);

    expect_residents(cache, {{3}, {1, 3}, {1, 2, 3}});
    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 1, 1}));
    EXPECT_EQ(cache.reference_capacity(), 3u);
}
TEST(Inclusive, InvalidationOfAllUpperCopies)
{
    auto data = test_data();
    std::vector<Cache_name_size> p(3, {"LRU", 2, 0});
    Inclusive cache(p, data);

    for(int k : {1, 2, 1, 3})
    {
        hierarchy_step(cache, p, k);
    }

    expect_residents(cache, {{2, 3}, {2, 3}, {2, 3}});
    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 0, 0}));

    EXPECT_EQ(hierarchy_step(cache, p, 1), -1);
}
TEST(Inclusive, UnknownKeyDoesNotCount)
{
    auto data = test_data();
    Inclusive cache({{"ARC", 2, 0}}, data);

    EXPECT_THROW(cache.access(999), std::runtime_error);

    EXPECT_EQ(cache.hits_level[0], 0u);
}
TEST(Inclusive, AllPolicyPairsAndMixedTriples)
{
    all_combinations<Cach_Mode::Inclusive>();
}

TEST(Inclusive, SingleLevelExactTrace)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 2, 0}};
    Inclusive cache(p, data);

    for(const auto& [key, level] : std::vector<std::pair<int, int>>{{1, -1}, {2, -1}, {1, 0}, {3, -1}, {2, -1}})
    {
        EXPECT_EQ(hierarchy_step(cache, p, key), level);
    }

    expect_residents(cache, {{2, 3}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1}));
    EXPECT_EQ(cache.reference_capacity(), 2u);
}

TEST(Inclusive, TwoMixedLevelsKeepFrequentlyUsedPage)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 1, 0}, {"LFU", 2, 0}};
    Inclusive cache(p, data);

    EXPECT_EQ(hierarchy_step(cache, p, 1), -1);
    EXPECT_EQ(hierarchy_step(cache, p, 2), -1);
    EXPECT_EQ(hierarchy_step(cache, p, 1), 1);

    expect_residents(cache, {{1}, {1, 2}});

    EXPECT_EQ(hierarchy_step(cache, p, 3), -1); // LFU сохраняет 1 с частотой 2.

    expect_residents(cache, {{3}, {1, 3}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 1);
    EXPECT_EQ(hierarchy_step(cache, p, 1), 0);

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 2}));
    EXPECT_EQ(cache.reference_capacity(), 2u);
}
