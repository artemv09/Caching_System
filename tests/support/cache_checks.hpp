#ifndef CACHE_CHECKS_HPP
#define CACHE_CHECKS_HPP
#include <gtest/gtest.h>
#include <map>
#include <set>
#include <random>
#include "creat_cach.hpp"

// Находит копию очереди в снимке; чтение не обновляет приоритет страниц.
template <typename K, typename V>
std::vector<K> queue_keys(const Cache_Snapshot<K, V>& state, const std::string& name)
{
    for(const auto& queue : state.queues)
    {
        if(queue.first == name)
        {
            return queue.second;
        }
    }
    return {};
}

// Проверяет согласованность диагностики, вместимость и уникальность резидентов.
template <typename Cache> void check_state(const Cache& cache)
{
    const auto state = cache.snapshot();
    EXPECT_TRUE(state.consistent);
    EXPECT_EQ(state.resident.size(), cache.size());
    EXPECT_LE(cache.size(), cache.capacity());
    std::set<int> keys;
    for(const auto& entry : state.resident)
    {
        EXPECT_TRUE(keys.insert(entry.key).second);
    }
    if(!queue_keys(state, "T1").empty() || !queue_keys(state, "T2").empty())
    {
        std::set<int> all;
        for(const auto& queue : state.queues)
        {
            for(int key : queue.second)
            {
                EXPECT_TRUE(all.insert(key).second);
            }
        }
    }
}

// Моделирует один запрос: lookup, затем загрузка только при промахе.
template <typename Cache> bool request(Cache& cache, int key)
{
    auto value = cache.look_up(key);
    if(value)
    {
        EXPECT_EQ(*value, key * 101);
        return true;
    }
    cache.insert_value(key, key * 101);
    return false;
}

// Проверяет общий контракт вставки/удаления без предположений о выборе жертвы.
template <typename Cache> void common_contract(Cache& cache)
{
    EXPECT_EQ(cache.look_up(1), nullptr);
    EXPECT_FALSE(cache.erase_key(1));
    EXPECT_FALSE(cache.extract_entry(1));
    auto result = cache.insert_value(1, 101);
    if(cache.capacity() == 0)
    {
        ASSERT_TRUE(result);
        EXPECT_EQ(result->key, 1);
        EXPECT_EQ(result->value, 101);
        EXPECT_EQ(cache.look_up(1), nullptr);
        check_state(cache);
        return;
    }
    EXPECT_FALSE(result);
    const auto before = cache.snapshot();
    EXPECT_FALSE(cache.insert_value(1, 999));
    EXPECT_EQ(before.queues, cache.snapshot().queues);
    auto found = cache.look_up(1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, 101);
    auto extracted = cache.extract_entry(1);
    ASSERT_TRUE(extracted);
    EXPECT_EQ(extracted->key, 1);
    EXPECT_EQ(extracted->value, 101);
    EXPECT_EQ(cache.look_up(1), nullptr);
    EXPECT_FALSE(cache.extract_entry(1));
    EXPECT_FALSE(cache.erase_key(1));
    cache.insert_value(1, 303);
    found = cache.look_up(1);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(*found, 303);
    EXPECT_TRUE(cache.erase_key(1));
    EXPECT_EQ(cache.size(), 0u);
    check_state(cache);
}

// Случайные операции сверяются с независимым учётом пар резидентов и жертв.
template <typename Cache> void mixed_operations(Cache& cache, unsigned seed)
{
    std::mt19937 rng(seed);
    std::map<int, int> resident;
    for(int step = 0; step < 600; ++step)
    {
        const int key = static_cast<int>(rng() % 13);
        const int operation = static_cast<int>(rng() % 4);
        SCOPED_TRACE(::testing::Message()
                     << "seed=" << seed << " step=" << step << " op=" << operation << " key=" << key);
        const bool exists = resident.count(key) != 0;
        if(operation == 0)
        {
            auto value = cache.look_up(key);
            ASSERT_EQ(value != nullptr, exists);
            if(value)
            {
                EXPECT_EQ(*value, resident.at(key));
            }
        }
        else if(operation == 1)
        {
            EXPECT_EQ(cache.erase_key(key), exists);
            resident.erase(key);
        }
        else if(operation == 2)
        {
            auto entry = cache.extract_entry(key);
            ASSERT_EQ(entry.has_value(), exists);
            if(entry)
            {
                EXPECT_EQ(entry->key, key);
                EXPECT_EQ(entry->value, resident.at(key));
            }
            resident.erase(key);
        }
        else
        {
            auto entry = cache.insert_value(key, step + 500);
            if(entry)
            {
                if(cache.capacity())
                {
                    ASSERT_TRUE(resident.count(entry->key));
                    EXPECT_EQ(entry->value, resident.at(entry->key));
                    resident.erase(entry->key);
                }
                else
                {
                    EXPECT_EQ(entry->value, step + 500);
                }
            }
            if(!exists && cache.capacity())
            {
                resident[key] = step + 500;
            }
        }
        check_state(cache);
        std::map<int, int> actual;
        for(const auto& entry : cache.snapshot().resident)
        {
            actual.emplace(entry.key, entry.value);
        }
        EXPECT_EQ(actual, resident);
    }
}

// Простая модель LRU/LFU: перебор resident-вектора с метками частоты/времени.
template <typename Cache> void compare_simple_model(Cache& cache, bool lfu, unsigned seed)
{
    struct Record
    {
        int key;
        int frequency;
        int time;
    };
    std::vector<Record> records;
    std::mt19937 rng(seed);
    for(int step = 0; step < 1000; ++step)
    {
        int key = static_cast<int>(rng() % 11);
        SCOPED_TRACE(::testing::Message() << "seed=" << seed << " step=" << step << " key=" << key);
        auto found =
            std::find_if(records.begin(), records.end(), [key](const Record& r) { return r.key == key; });
        bool hit = found != records.end();
        EXPECT_EQ(request(cache, key), hit);
        if(hit)
        {
            ++found -> frequency;
            found -> time = step;
        }
        else if(cache.capacity())
        { 
            if(records.size() == cache.capacity())
            {
                auto victim = std::min_element(records.begin(), records.end(),
                                               [lfu](const Record& a, const Record& b)
                                               {
                                                   if(lfu && a.frequency != b.frequency)
                                                   {
                                                       return a.frequency < b.frequency;
                                                   }
                                                   return a.time < b.time;
                                               });
                records.erase(victim);
            }
            records.push_back({key, 1, step});
        }
        std::set<int> expected, actual;
        for(const auto& record : records)
        {
            expected.insert(record.key);
        }
        for(const auto& entry : cache.snapshot().resident)
        {
            actual.insert(entry.key);
        }
        EXPECT_EQ(actual, expected);
        check_state(cache);
    }
}

// Управляемое исключение копирования позволяет проверять откат без нехватки RAM.
struct Throwing_Value
{
    int value;
    inline static int copies_left = -1;
    explicit Throwing_Value(int number) : value(number)
    {
    }
    Throwing_Value(const Throwing_Value& other) : value(other.value)
    {
        if(copies_left == 0)
        {
            throw std::runtime_error("test copy failure");
        }
        if(copies_left > 0)
        {
            copies_left--;
        }
    }
    Throwing_Value(Throwing_Value&&) noexcept = default;
    Throwing_Value& operator=(const Throwing_Value&) = default;
    Throwing_Value& operator=(Throwing_Value&&) noexcept = default;
};
#endif
