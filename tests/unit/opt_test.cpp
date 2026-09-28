#include "opt_cach.hpp"
#include <gtest/gtest.h>
#include <functional>
#include <set>
#include <string>

TEST(Opt, UnknownKeyDoesNotConsumeHistory)
{
    std::unordered_map<int, int> data{{1, 101}, {2, 202}};
    OptCache<int, int> cache(1, {1, 2}, data);

    EXPECT_THROW(cache.access(999), std::logic_error);
    EXPECT_THROW(cache.access(2), std::logic_error);
    EXPECT_FALSE(cache.access(1).hit);
    EXPECT_EQ(cache.access(2).sought_element, 202);
    EXPECT_THROW(cache.access(2), std::logic_error);
}
TEST(Opt, EmptyZeroAndValues)
{
    std::unordered_map<std::string, std::string> data{{"a", "alpha"}};
    OptCache<std::string, std::string> empty(2, {}, data);

    EXPECT_THROW(empty.access("a"), std::logic_error);

    OptCache<std::string, std::string> zero(0, {"a", "a"}, data);

    EXPECT_FALSE(zero.access("a").hit);
    EXPECT_EQ(zero.access("a").sought_element, "alpha");

    EXPECT_EQ(zero.size(), 0u);
    EXPECT_EQ(zero.hits(), 0u);
}
TEST(Opt, FarthestAndNoFutureUse)
{
    std::unordered_map<int, int> data{{1, 101}, {2, 202}, {3, 303}, {4, 404}};
    std::vector<int> keys{1, 2, 3, 1, 2, 4, 1};
    OptCache<int, int> cache(2, keys, data);
    std::vector<bool> expected{false, false, false, true, false, false, true};

    for(std::size_t i = 0; i < keys.size(); ++i)
    {
        EXPECT_EQ(cache.access(keys[i]).hit, expected[i]);

        EXPECT_TRUE(cache.snapshot().consistent);
    }

    EXPECT_EQ(cache.hits(), 2u);
}
static int best_hits(const std::vector<int>& keys, std::size_t index, std::set<int> resident,
                     std::size_t capacity)
{
    if(index == keys.size())
    {
        return 0;
    }
    const int key = keys[index];
    if(resident.count(key))
    {
        return 1 + best_hits(keys, index + 1, resident, capacity);
    }
    if(capacity == 0)
    {
        return best_hits(keys, index + 1, resident, capacity);
    }
    if(resident.size() < capacity)
    {
        resident.insert(key);
        return best_hits(keys, index + 1, resident, capacity);
    }

    int result = 0;

    for(int victim : resident)
    {
        auto next = resident;
        next.erase(victim);
        next.insert(key);
        result = std::max(result, best_hits(keys, index + 1, next, capacity));
    }
    return result;
}
TEST(Opt, ExhaustiveShortStreams)
{
    const std::unordered_map<int, int> data{{0, 17}, {1, 101}, {2, 202}};

    for(int code = 0; code < 729; ++code)
    {
        int number = code;
        std::vector<int> keys;

        for(int i = 0; i < 6; ++i)
        {
            keys.push_back(number % 3);
            number /= 3;
        }

        for(std::size_t c : {0, 1, 2, 3})
        {
            SCOPED_TRACE(::testing::Message() << "code=" << code << " capacity=" << c);
            OptCache<int, int> cache(c, keys, data);
            
            for(int key : keys)
            {
                EXPECT_EQ(cache.access(key).sought_element, data.at(key));
            }

            EXPECT_EQ(cache.hits(), static_cast<std::size_t>(best_hits(keys, 0, {}, c)));
        }
    }
}
