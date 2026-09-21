#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "work_cach.hpp"

namespace
{
using Cache = Multi_Level_Cach<int, int, Cach_Mode::Exclusive, true>;
using Big_Data = std::unordered_map<int, int>;

struct Test_Case
{
    std::string name;
    std::size_t l1_capacity;
    std::size_t l2_capacity;
    std::vector<int> keys;
    std::vector<bool> expected_hits;
};

Cache_name_size make_level(const std::string& name, std::size_t capacity)
{
    std::size_t hir_capacity = 0;

    if(name == "LIRS")
        hir_capacity = capacity == 0 ? 0 : 1;

    return Cache_name_size{name, capacity, hir_capacity};
}

Big_Data make_data()
{
    Big_Data data;

    for(int key = 1; key <= 100; ++key)
        data.emplace(key, key * 100);

    return data;
}

bool run_trace(
    const std::string& l1,
    const std::string& l2,
    const Test_Case& test,
    const Big_Data& data)
{
    std::vector<Cache_name_size> parameters
    {
        make_level(l1, test.l1_capacity),
        make_level(l2, test.l2_capacity)
    };

    Cache cache(parameters, data);

    if(test.keys.size() != test.expected_hits.size())
    {
        std::cerr << "[TEST ERROR] " << test.name
                  << ": keys.size() != expected_hits.size()\n";
        return false;
    }

    for(std::size_t i = 0; i < test.keys.size(); ++i)
    {
        const int key = test.keys[i];
        Public_Access_Result<int> result;

        try
        {
            result = cache.access(key);
        }
        catch(const std::exception& error)
        {
            std::cerr << "[FAIL] " << l1 << " -> " << l2
                      << " | " << test.name
                      << " | request #" << (i + 1)
                      << " key=" << key
                      << " | unexpected exception: "
                      << error.what() << '\n';
            return false;
        }

        const int expected_value = key * 100;

        if(result.sought_element != expected_value)
        {
            std::cerr << "[FAIL] " << l1 << " -> " << l2
                      << " | " << test.name
                      << " | request #" << (i + 1)
                      << " key=" << key
                      << " | expected value=" << expected_value
                      << ", actual value=" << result.sought_element
                      << '\n';
            return false;
        }

        if(result.hit != test.expected_hits[i])
        {
            std::cerr << "[FAIL] " << l1 << " -> " << l2
                      << " | " << test.name
                      << " | request #" << (i + 1)
                      << " key=" << key
                      << " | expected hit=" << test.expected_hits[i]
                      << ", actual hit=" << result.hit
                      << '\n';
            return false;
        }
    }

    return true;
}

bool run_invalid_key_test(
    const std::string& l1,
    const std::string& l2,
    const Big_Data& data)
{
    std::vector<Cache_name_size> parameters
    {
        make_level(l1, 2),
        make_level(l2, 3)
    };

    Cache cache(parameters, data);

    try
    {
        (void)cache.access(1000);
    }
    catch(const std::runtime_error&)
    {
        return true;
    }
    catch(const std::exception& error)
    {
        std::cerr << "[FAIL] " << l1 << " -> " << l2
                  << " | invalid key | wrong exception type: "
                  << error.what() << '\n';
        return false;
    }

    std::cerr << "[FAIL] " << l1 << " -> " << l2
              << " | invalid key | exception was not thrown\n";
    return false;
}
}

int main()
{
    const std::vector<std::string> algorithms
    {
        "LRU", "LFU", "LIRS", "ARC", "2Q"
    };

    const std::vector<Test_Case> tests
    {
        {
            "same key stays cached",
            2, 3,
            {1, 1, 1, 1},
            {false, true, true, true}
        },
        {
            // Exclusive: 2 + 3 = 5 unique resident pages.
            // После cold-fill ключами 1..5 все пять должны оставаться
            // где-то в hierarchy. Второй проход обязан быть полностью hit.
            "exclusive uses sum of level capacities",
            2, 3,
            {1, 2, 3, 4, 5, 1, 2, 3, 4, 5},
            {false, false, false, false, false,
             true,  true,  true,  true,  true}
        },
        {
            // Проверяет promotion вверх и push_down вытесненных страниц вниз.
            "promotion and push-down in reverse order",
            2, 3,
            {1, 2, 3, 4, 5, 5, 4, 3, 2, 1},
            {false, false, false, false, false,
             true,  true,  true,  true,  true}
        },
        {
            // Несколько оборотов рабочего набора хорошо ловят resident-дубли
            // между уровнями и потерю страниц при promotion.
            "repeated exclusive rotation",
            2, 3,
            {1, 2, 3, 4, 5,
             1, 2, 3, 4, 5,
             1, 2, 3, 4, 5},
            {false, false, false, false, false,
             true,  true,  true,  true,  true,
             true,  true,  true,  true,  true}
        },
        {
            // Какой старый key уйдет из hierarchy, зависит от алгоритма.
            // Поэтому проверяем только то, что новый 6 — miss, а повторный 6 — hit.
            "one page over total capacity",
            2, 3,
            {1, 2, 3, 4, 5, 6, 6},
            {false, false, false, false, false, false, true}
        },
        {
            "seven unique pages fit across two levels",
            3, 4,
            {1, 2, 3, 4, 5, 6, 7,
             7, 6, 5, 4, 3, 2, 1},
            {false, false, false, false, false, false, false,
             true,  true,  true,  true,  true,  true,  true}
        }
    };

    const Big_Data data = make_data();

    std::size_t pair_count = 0;
    std::size_t passed_pairs = 0;
    std::size_t failed_checks = 0;

    for(const std::string& l1 : algorithms)
    {
        for(const std::string& l2 : algorithms)
        {
            ++pair_count;
            bool pair_ok = true;

            for(const Test_Case& test : tests)
            {
                if(!run_trace(l1, l2, test, data))
                {
                    pair_ok = false;
                    ++failed_checks;
                }
            }

            if(!run_invalid_key_test(l1, l2, data))
            {
                pair_ok = false;
                ++failed_checks;
            }

            if(pair_ok)
            {
                ++passed_pairs;
                std::cout << "[PASS] " << l1 << " -> " << l2 << '\n';
            }
            else
            {
                std::cout << "[PAIR FAILED] "
                          << l1 << " -> " << l2 << '\n';
            }
        }
    }

    std::cout << "\n=====================================\n"
              << "Exclusive pairs tested: " << pair_count << '\n'
              << "Pairs fully passed: " << passed_pairs << '\n'
              << "Failed checks: " << failed_checks << '\n'
              << "=====================================\n";

    return failed_checks == 0 ? 0 : 1;
}
