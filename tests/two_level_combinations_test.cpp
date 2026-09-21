#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "work_cach.hpp"

namespace
{
using Cache = Multi_Level_Cach<int, int, Cach_Mode::Inclusive, true>;
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
    {
        if(capacity == 0)
        {
            hir_capacity = 0;
        }
        else
        {
            // Для capacity >= 2 получаем LIR_capacity = capacity - 1.
            // Для capacity == 1 конструктор LIRS также допускает HIR = 1.
            hir_capacity = 1;
        }
    }

    return Cache_name_size{name, capacity, hir_capacity};
}

Big_Data make_data()
{
    Big_Data data;

    for(int key = 1; key <= 20; ++key)
    {
        data.emplace(key, key * 100);
    }

    return data;
}

bool run_trace(
    const std::string& l1,
    const std::string& l2,
    const Test_Case& test,
    const Big_Data& data
)
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
            std::cerr
                << "[FAIL] " << l1 << " -> " << l2
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
            std::cerr
                << "[FAIL] " << l1 << " -> " << l2
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
            std::cerr
                << "[FAIL] " << l1 << " -> " << l2
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
    const Big_Data& data
)
{
    std::vector<Cache_name_size> parameters
    {
        make_level(l1, 3),
        make_level(l2, 5)
    };

    Cache cache(parameters, data);

    try
    {
        (void)cache.access(99);
    }
    catch(const std::runtime_error&)
    {
        return true;
    }
    catch(const std::exception& error)
    {
        std::cerr
            << "[FAIL] " << l1 << " -> " << l2
            << " | invalid key"
            << " | wrong exception type: "
            << error.what() << '\n';

        return false;
    }

    std::cerr
        << "[FAIL] " << l1 << " -> " << l2
        << " | invalid key"
        << " | exception was not thrown\n";

    return false;
}

} // namespace

int main()
{
    const std::vector<std::string> algorithms
    {
        "LRU",
        "LFU",
        "LIRS",
        "ARC",
        "2Q"
    };

    const std::vector<Test_Case> tests
    {
        {
            "same key",
            3,
            5,
            {1, 1, 1, 1},
            {false, true, true, true}
        },

        {
            "working set fits L1",
            3,
            5,
            {1, 2, 3, 1, 2, 3, 1, 2, 3},
            {
                false, false, false,
                true, true, true,
                true, true, true
            }
        },

        {
            // Ключ 1/2/3 не может целиком находиться в L1 capacity=2,
            // но все три обязаны оставаться в L2 capacity=5.
            // Этот тест ловит ошибку, когда cold miss заполняет только L1.
            "L2 is actually used",
            2,
            5,
            {1, 2, 3, 1, 2, 3},
            {false, false, false, true, true, true}
        },

        {
            "all cold requests",
            3,
            5,
            {1, 2, 3, 4, 5, 6, 7, 8, 9},
            {
                false, false, false,
                false, false, false,
                false, false, false
            }
        },

        {
            // L1 не может хранить 4 ключа, L2 может.
            "four-page working set through L2",
            2,
            5,
            {1, 2, 3, 4, 1, 2, 3, 4},
            {
                false, false, false, false,
                true, true, true, true
            }
        },

        {
            // Нулевой L1 не должен мешать L2 обслуживать повторные запросы.
            "zero-capacity L1",
            0,
            5,
            {1, 1, 2, 2, 3, 3},
            {false, true, false, true, false, true}
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
                const bool ok = run_trace(l1, l2, test, data);

                if(!ok)
                {
                    pair_ok = false;
                    ++failed_checks;
                }
            }

            const bool invalid_key_ok =
                run_invalid_key_test(l1, l2, data);

            if(!invalid_key_ok)
            {
                pair_ok = false;
                ++failed_checks;
            }

            if(pair_ok)
            {
                ++passed_pairs;

                std::cout
                    << "[PASS] "
                    << l1 << " -> " << l2
                    << '\n';
            }
            else
            {
                std::cout
                    << "[PAIR FAILED] "
                    << l1 << " -> " << l2
                    << '\n';
            }
        }
    }

    std::cout
        << "\n==============================\n"
        << "Pairs tested: " << pair_count << '\n'
        << "Pairs fully passed: " << passed_pairs << '\n'
        << "Failed checks: " << failed_checks << '\n'
        << "==============================\n";

    return failed_checks == 0 ? 0 : 1;
}
