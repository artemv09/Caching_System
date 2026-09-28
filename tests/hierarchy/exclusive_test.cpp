#include "tests/support/hierarchy_checks.hpp"

using Exclusive = Multi_Level_Cach<int, int, Cach_Mode::Exclusive>;

TEST(Exclusive, RejectEmpty)
{
    auto data = test_data();

    EXPECT_THROW(Exclusive({}, data), std::invalid_argument);
}
TEST(Exclusive, ThreeLevelCascade)
{
    auto data = test_data();
    std::vector<Cache_name_size> p(3, {"LRU", 1, 0});
    Exclusive cache(p, data);

    for(int k : {1, 2, 3})
    {
        EXPECT_EQ(hierarchy_step(cache, p, k), -1);
    }

    expect_residents(cache, {{3}, {2}, {1}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 2);

    
    expect_residents(cache, {{1}, {3}, {2}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 0);
    EXPECT_EQ(hierarchy_step(cache, p, 3), 1);

    
    expect_residents(cache, {{3}, {1}, {2}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 1, 1}));
    EXPECT_EQ(cache.reference_capacity(), 3u);
}
TEST(Exclusive, ZeroLevelsPassThrough)
{
    auto data = test_data();

    for(auto p : {std::vector<Cache_name_size>{{"ARC", 0, 0}, {"LRU", 2, 0}},
                  std::vector<Cache_name_size>{{"LIRS", 0, 0}, {"2Q", 0, 0}}})
    {
        Exclusive cache(p, data);

        EXPECT_EQ(hierarchy_step(cache, p, 1), -1);
        EXPECT_EQ(hierarchy_step(cache, p, 1), p[1].capacity ? 1 : -1);
    }
}
TEST(Exclusive, UnknownKeyAndCapacityOverflow)
{
    auto data = test_data();
    Exclusive cache({{"LRU", 1, 0}}, data);

    EXPECT_THROW(cache.access(999), std::runtime_error);

    EXPECT_EQ(cache.hits_level[0], 0u);

    Exclusive huge({{"LRU", std::numeric_limits<std::size_t>::max(), 0}, {"LRU", 1, 0}}, data);

    EXPECT_THROW(huge.total_capacity(), std::overflow_error);
}
TEST(Exclusive, AllPolicyPairsAndMixedTriples)
{
    all_combinations<Cach_Mode::Exclusive>();
}
TEST(Exclusive, SingleLevelExactTrace)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 2, 0}};
    Exclusive cache(p, data);

    for(const auto& [key, level] : std::vector<std::pair<int, int>>{{1, -1}, {2, -1}, {1, 0}, {3, -1}, {2, -1}})
    {
        EXPECT_EQ(hierarchy_step(cache, p, key), level);
    }

    expect_residents(cache, {{2, 3}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1}));
    EXPECT_EQ(cache.reference_capacity(), 2u);
}
TEST(Exclusive, TwoMixedLevelsPromotionAndEviction)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 1, 0}, {"LFU", 2, 0}};
    Exclusive cache(p, data);

    for(int key : {1, 2, 3})
    {
        EXPECT_EQ(hierarchy_step(cache, p, key), -1);
    }

    expect_residents(cache, {{3}, {1, 2}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 1);

    expect_residents(cache, {{1}, {2, 3}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 0);
    EXPECT_EQ(hierarchy_step(cache, p, 4), -1);

    expect_residents(cache, {{4}, {1, 3}});

    EXPECT_EQ(hierarchy_step(cache, p, 2), -1);

    expect_residents(cache, {{2}, {1, 4}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 1}));
    EXPECT_EQ(cache.reference_capacity(), 3u);
}
TEST(Exclusive, ThreeMixedLevelsExactCascade)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 1, 0}, {"LFU", 2, 0}, {"ARC", 3, 0}};
    Exclusive cache(p, data);

    for(int key : {1, 2, 3, 4})
    {
        EXPECT_EQ(hierarchy_step(cache, p, key), -1);
    }

    expect_residents(cache, {{4}, {2, 3}, {1}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 2);

    expect_residents(cache, {{1}, {3, 4}, {2}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 0);
    EXPECT_EQ(hierarchy_step(cache, p, 3), 1);

    expect_residents(cache, {{3}, {1, 4}, {2}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({1, 1, 1}));
    EXPECT_EQ(cache.reference_capacity(), 6u);
}
TEST(Exclusive, ZeroMiddleLevelPassesCascade)
{
    const auto data = test_data();
    const std::vector<Cache_name_size> p{{"LRU", 1, 0}, {"ARC", 0, 0}, {"LFU", 1, 0}};
    Exclusive cache(p, data);

    EXPECT_EQ(hierarchy_step(cache, p, 1), -1);
    EXPECT_EQ(hierarchy_step(cache, p, 2), -1);

    expect_residents(cache, {{2}, {}, {1}});

    EXPECT_EQ(hierarchy_step(cache, p, 1), 2);

    expect_residents(cache, {{1}, {}, {2}});

    EXPECT_EQ(cache.hits_level, std::vector<std::size_t>({0, 0, 1}));
}
