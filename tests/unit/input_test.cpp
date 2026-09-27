#include <gtest/gtest.h>
#include "creat_cach.hpp"
#include "tests/request_patterns.hpp"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <sstream>

TEST(Input, ExactCountAndEmpty)
{
    std::istringstream input("4\n1 2\n3 10\n");
    EXPECT_EQ(read_requests<int>(input), std::vector<int>({1, 2, 3, 10}));
    std::istringstream empty("0\n");
    EXPECT_TRUE(read_requests<int>(empty).empty());
    for(const std::string text :
        {"-1", "2 1", "1 0", "1 -1", "1 11", "1 2x", "a", "1000001", "999999999999999999999999"})
    {
        SCOPED_TRACE(text);
        std::istringstream bad(text);
        EXPECT_THROW(read_requests<int>(bad), std::invalid_argument);
    }
}

TEST(Input, ConsoleDoesNotRequireEndOfStream)
{
    std::istringstream input("2 7 7 remaining");
    EXPECT_EQ(read_requests<int>(input), std::vector<int>({7, 7}));
    // После N ключей чтение завершено; интерактивный запуск не ожидает Ctrl+D.
    std::string next;
    input >> next;
    EXPECT_EQ(next, "remaining");

    std::istringstream empty("0 next");
    EXPECT_TRUE(read_requests<int>(empty).empty());
    empty >> next;
    EXPECT_EQ(next, "next");
}

TEST(Input, IntegralKeyTypes)
{
    std::istringstream wide("3 1 70000 1000000");
    EXPECT_EQ(read_requests<long long>(wide, MAX_PAGE_KEY),
              std::vector<long long>({1, 70000, 1000000}));

    std::istringstream positive("3 1 5 10");
    EXPECT_EQ(read_requests<unsigned int>(positive),
              std::vector<unsigned int>({1, 5, 10}));

    std::istringstream negative("1 -1");
    EXPECT_THROW(read_requests<unsigned int>(negative), std::invalid_argument);
}

TEST(Input, NarrowIntegralKeysCheckedBeforeConversion)
{
    // uint8_t читается как число, а не как символ; 256 не должно превратиться в 0.
    std::istringstream valid("2 1 255");
    EXPECT_EQ(read_requests<std::uint8_t>(valid, 300),
              std::vector<std::uint8_t>({1, 255}));

    std::istringstream overflow("1 256");
    EXPECT_THROW(read_requests<std::uint8_t>(overflow, 300), std::invalid_argument);
}

TEST(Input, StringKeysAndMissingRequest)
{
    std::istringstream input("3 alpha beta alpha remaining");
    EXPECT_EQ(read_requests<std::string>(input),
              std::vector<std::string>({"alpha", "beta", "alpha"}));
    std::string next;
    input >> next;
    EXPECT_EQ(next, "remaining");

    std::istringstream missing("2 alpha");
    EXPECT_THROW(read_requests<std::string>(missing), std::invalid_argument);

    std::istringstream empty("0 next");
    EXPECT_TRUE(read_requests<std::string>(empty).empty());
    empty >> next;
    EXPECT_EQ(next, "next");
}

TEST(Input, OldConfigurationAndConsoleOrder)
{
    std::istringstream config("2\nLRU\nLFU\n");
    std::istringstream input("2 4\n6\n1 2 1 3 2 1\n");
    const auto levels = parsing_cach_parametr(config, input);
    ASSERT_EQ(levels.size(), 2u);
    EXPECT_EQ(levels[0].name_cach, "LRU");
    EXPECT_EQ(levels[0].capacity, 2u);
    EXPECT_EQ(levels[1].name_cach, "LFU");
    EXPECT_EQ(levels[1].capacity, 4u);
    EXPECT_EQ(read_requests<int>(input), std::vector<int>({1, 2, 1, 3, 2, 1}));
}

TEST(Input, LirsParameterFollowsItsCapacity)
{
    std::istringstream config("3 LRU LIRS ARC");
    std::istringstream input("2 4 1 6 2 7 7");
    const auto levels = parsing_cach_parametr(config, input);
    ASSERT_EQ(levels.size(), 3u);
    EXPECT_EQ(levels[0].capacity, 2u);
    EXPECT_EQ(levels[1].capacity, 4u);
    EXPECT_EQ(levels[1].hir_capacity, 1u);
    EXPECT_EQ(levels[2].capacity, 6u);
    EXPECT_EQ(read_requests<int>(input), std::vector<int>({7, 7}));
}

TEST(Input, ConfigurationValidation)
{
    // Пары содержат отдельно config.txt и консольный ввод.
    for(const auto& [config_text, input_text] :
        std::vector<std::pair<std::string, std::string>>{
            {"0", ""}, {"-1", ""}, {"65", ""}, {"1 BAD", "2"},
            {"2 LRU", "2 4"}, {"1 LRU 2", "2"}, {"1 LRU extra", "2"},
            {"1 LRU", "-1"}, {"1 LRU", "1000001"}, {"1 LRU", ""},
            {"1 LIRS", "2 0"}, {"1 LIRS", "1 2"}, {"1 LIRS", "4"}})
    {
        SCOPED_TRACE(config_text + " / " + input_text);
        std::istringstream config(config_text);
        std::istringstream input(input_text);
        EXPECT_THROW(parsing_cach_parametr(config, input), std::invalid_argument);
    }
}

TEST(Input, CapacityOneAndZeroLirsParameters)
{
    for(const auto& capacities : {"0 0", "1 0", "1 1"})
    {
        SCOPED_TRACE(capacities);
        std::istringstream config("1 LIRS");
        std::istringstream input(capacities);
        // Допустимость нулевого уровня для выбранной иерархии проверяет её конструктор.
        EXPECT_NO_THROW(parsing_cach_parametr(config, input));
    }
}

TEST(Patterns, FamiliesAndReproducibility)
{
    const auto patterns = request_patterns::make_patterns(73, 2, 4, 6, 2026);
    const auto repeated = request_patterns::make_patterns(73, 2, 4, 6, 2026);
    ASSERT_FALSE(patterns.empty());
    ASSERT_EQ(patterns.size(), repeated.size());
    for(std::size_t i = 0; i < patterns.size(); ++i)
    {
        SCOPED_TRACE(patterns[i].name);
        EXPECT_FALSE(patterns[i].name.empty());
        EXPECT_EQ(patterns[i].keys.size(), 73u);
        EXPECT_EQ(patterns[i].name, repeated[i].name);
        EXPECT_EQ(patterns[i].keys, repeated[i].keys);
        for(int key : patterns[i].keys)
        {
            EXPECT_GE(key, 1);
            EXPECT_LE(key, MAX_PAGE_KEY);
        }
    }
    for(const auto& pattern : request_patterns::make_patterns(0, 0, 0, 0))
    {
        EXPECT_TRUE(pattern.keys.empty());
    }
}

TEST(Patterns, SameFrequenciesDifferentOrder)
{
    EXPECT_EQ(request_patterns::grouped(6, 3), std::vector<int>({1, 1, 2, 2, 3, 3}));
    auto grouped = request_patterns::grouped(13, 3);
    auto interleaved = request_patterns::cycle(13, 3);
    EXPECT_NE(grouped, interleaved);
    std::sort(interleaved.begin(), interleaved.end());
    EXPECT_EQ(grouped, interleaved);
}

// Нулевые, единичные и убывающие ёмкости допустимы для генератора запросов.
TEST(Patterns, CapacityBoundariesKeepLengthAndKeyRange)
{
    struct Capacities
    {
        std::size_t first;
        std::size_t last;
        std::size_t total;
    };
    for(const auto& capacities : std::vector<Capacities>{
            {0, 0, 0}, {1, 1, 2}, {4, 1, 5},
            {MAX_PAGE_KEY, MAX_PAGE_KEY, 64 * static_cast<std::size_t>(MAX_PAGE_KEY)}})
    {
        for(std::size_t count : {0u, 17u})
        {
            SCOPED_TRACE(::testing::Message() << "N=" << count << ", first=" << capacities.first
                << ", last=" << capacities.last << ", total=" << capacities.total);
            const auto patterns = request_patterns::make_patterns(
                count, capacities.first, capacities.last, capacities.total);
            ASSERT_FALSE(patterns.empty());
            for(const auto& pattern : patterns)
            {
                SCOPED_TRACE(pattern.name);
                EXPECT_EQ(pattern.keys.size(), count);
                for(int key : pattern.keys)
                {
                    EXPECT_GE(key, 1);
                    EXPECT_LE(key, MAX_PAGE_KEY);
                }
            }
        }
    }
}

// Миллион запросов не означает миллион разных холодных ключей.
TEST(Patterns, HotScanAcceptsMaximumRequestCount)
{
    const auto keys = request_patterns::hot_scan(MAX_REQUESTS, 2, 2);
    ASSERT_EQ(keys.size(), MAX_REQUESTS);
    EXPECT_EQ(std::vector<int>(keys.begin(), keys.begin() + 18),
              std::vector<int>({1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 3, 4}));
    const auto [smallest, largest] = std::minmax_element(keys.begin(), keys.end());
    EXPECT_EQ(*smallest, 1);
    EXPECT_GT(*largest, 2);
    EXPECT_LE(*largest, MAX_PAGE_KEY);
}

TEST(Patterns, RejectInvalidSuiteCapacitiesAndCount)
{
    using request_patterns::make_patterns;
    EXPECT_THROW(make_patterns(17, 4, 1, 3), std::invalid_argument);
    EXPECT_THROW(make_patterns(17, 1, 4, 3), std::invalid_argument);
    EXPECT_THROW(make_patterns(17, MAX_PAGE_KEY + 1, 1, MAX_PAGE_KEY + 2), std::invalid_argument);
    EXPECT_THROW(make_patterns(17, 1, MAX_PAGE_KEY + 1, MAX_PAGE_KEY + 2), std::invalid_argument);
    EXPECT_THROW(make_patterns(MAX_REQUESTS + 1, 2, 4, 6), std::invalid_argument);
}

TEST(Patterns, RejectInvalidParameters)
{
    using namespace request_patterns;
    EXPECT_THROW(cycle(4, 0), std::invalid_argument);
    EXPECT_THROW(uniform(4, std::numeric_limits<std::size_t>::max(), 42), std::invalid_argument);
    EXPECT_THROW(hot_cold(4, 2, 3, 101, 42), std::invalid_argument);
    EXPECT_THROW(hot_cold(4, 2, 3, -1, 42), std::invalid_argument);
    EXPECT_THROW(hot_scan(4, 2, 0), std::invalid_argument);
    EXPECT_THROW(phases(100, 1000000, 1, false), std::invalid_argument);
    EXPECT_THROW(moving_window(100, 1000000, 1, 42), std::invalid_argument);
    EXPECT_THROW(zipf(4, 3, std::numeric_limits<double>::quiet_NaN(), 42), std::invalid_argument);
    EXPECT_THROW(zipf(4, 3, 0, 42), std::invalid_argument);
}
