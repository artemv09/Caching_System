#ifndef HIERARCHY_CHECKS_HPP
#define HIERARCHY_CHECKS_HPP
#include "tests/support/cache_checks.hpp"
#include "multi_level_cache.hpp"
#include <numeric>

// Создаёт живущий до конца теста источник с отличимыми от ключей значениями.
inline std::unordered_map<int, int> test_data()
{
    std::unordered_map<int, int> result;
    for(int k = 1; k <= 20; ++k)
    {
        result.emplace(k, k * 101);
    }
    return result;
}

// Проверяет один access по снимку до запроса, затем все инварианты уровней.
template <Cach_Mode Mode>
int hierarchy_step(Multi_Level_Cach<int, int, Mode>& cache, const std::vector<Cache_name_size>& configuration, int key)
{
    const auto before = cache.snapshot();
    int expected_level = -1;
    for(std::size_t i = 0; i < before.size(); ++i)
    {
        for(const auto& e : before[i].resident)
        {
            if(e.key == key && expected_level == -1)
            {
                expected_level = static_cast<int>(i);
            }
        }
    }
    const auto old_hits = cache.hits_level;
    const auto result = cache.access(key);
    EXPECT_EQ(result.hit, expected_level >= 0);
    EXPECT_EQ(result.sought_element, key * 101);
    const auto after = cache.snapshot();
    std::vector<std::set<int>> sets(after.size());
    std::size_t delta = 0;
    for(std::size_t i = 0; i < after.size(); ++i)
    {
        EXPECT_TRUE(after[i].consistent);
        EXPECT_LE(after[i].resident.size(), configuration[i].capacity);
        EXPECT_EQ(cache.hits_level[i] - old_hits[i],
                  static_cast<std::size_t>(static_cast<int>(i) == expected_level));
        delta += cache.hits_level[i] - old_hits[i];
        for(const auto& e : after[i].resident)
        {
            EXPECT_EQ(e.value, e.key * 101);
            EXPECT_TRUE(sets[i].insert(e.key).second);
        }
    }
    EXPECT_EQ(delta, static_cast<std::size_t>(result.hit));
    for(std::size_t i = 0; i < sets.size(); ++i)
    {
        for(std::size_t j = i + 1; j < sets.size(); ++j)
        {
            for(int k : sets[i])
            {
                if constexpr(Mode == Cach_Mode::Inclusive)
                {
                    EXPECT_EQ(sets[j].count(k), 1u);
                }
                else
                {
                    EXPECT_EQ(sets[j].count(k), 0u);
                }
            }
        }
    }
    return expected_level;
}

// Сравнивает резидентные ключи каждого уровня с заранее рассчитанным состоянием.
template <Cach_Mode Mode>
void expect_residents(const Multi_Level_Cach<int, int, Mode>& cache,
                      const std::vector<std::set<int>>& expected)
{
    const auto states = cache.snapshot();
    ASSERT_EQ(states.size(), expected.size());
    for(std::size_t level = 0; level < states.size(); ++level)
    {
        SCOPED_TRACE(::testing::Message() << "level=" << level + 1);
        std::set<int> actual;
        for(const auto& entry : states[level].resident)
        {
            actual.insert(entry.key);
            EXPECT_EQ(entry.value, entry.key * 101);
        }
        EXPECT_EQ(actual, expected[level]);
    }
}

// Все 25 пар и три содержательные тройки; результат не подменяется поведением LRU.
template <Cach_Mode Mode> void all_combinations()
{
    const auto data = test_data();
    std::vector<std::vector<Cache_name_size>> configurations;
    for(const std::string a : {"LRU", "LFU", "2Q", "ARC", "LIRS"})
    {
        for(const std::string b : {"LRU", "LFU", "2Q", "ARC", "LIRS"})
        {
            configurations.push_back({{a, 2, 1}, {b, 4, 1}});
        }
    }
    configurations.push_back({{"LRU", 1, 0}, {"2Q", 3, 0}, {"ARC", 5, 0}});
    configurations.push_back({{"ARC", 2, 0}, {"LIRS", 3, 1}, {"2Q", 5, 0}});
    configurations.push_back({{"2Q", 2, 0}, {"ARC", 3, 0}, {"LIRS", 5, 2}});
    for(const auto& configuration : configurations)
    {
        SCOPED_TRACE(configuration[0].name_cach + "/" + configuration[1].name_cach);
        Multi_Level_Cach<int, int, Mode> cache(configuration, data);
        std::mt19937 rng(42);
        std::size_t hits = 0;

        for(int step = 0; step < 400; ++step)
        {
            const int key = 1 + static_cast<int>(rng() % 12);
            SCOPED_TRACE(::testing::Message() << "seed=42 step=" << step << " key=" << key);
            hits += hierarchy_step(cache, configuration, key) >= 0;
        }
        EXPECT_EQ(std::accumulate(cache.hits_level.begin(), cache.hits_level.end(), std::size_t{0}), hits);
    }
}
#endif
