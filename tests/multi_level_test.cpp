#include "multi_level_cache.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>


using Data = std::unordered_map<int, int>;


template <typename Cache>
void check_access(
    Cache& cache,
    int key,
    bool expected_hit,
    int expected_value)
{
    auto result = cache.access(key);

    assert(result.hit == expected_hit);
    assert(result.sought_element == expected_value);
}


// ============================================================
// INCLUSIVE
// ============================================================

void inclusive_promotion_test()
{
    std::cout << "inclusive_promotion_test\n";

    Data data{
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500}
    };

    std::vector<Cache_name_size> levels{
        {"LRU", 2, 0},   // L1
        {"LRU", 3, 0}    // L2
    };

    Multi_Level_Cach<
        int,
        int,
        Cach_Mode::Inclusive
    > cache(levels, data);


    check_access(cache, 1, false, 100);
    check_access(cache, 2, false, 200);
    check_access(cache, 3, false, 300);

    /*
        После этого:

        L1: [3, 2]
        L2: [3, 2, 1]

        1 отсутствует в L1,
        но находится в L2.
    */

    check_access(cache, 1, true, 100);

    /*
        Hit должен произойти на L2.

        Inclusive policy должна после этого
        поднять 1 обратно в L1.
    */

    assert(cache.hits_level[0] == 0);
    assert(cache.hits_level[1] == 1);


    // Теперь 1 должен находиться уже в L1.
    check_access(cache, 1, true, 100);

    assert(cache.hits_level[0] == 1);
    assert(cache.hits_level[1] == 1);

    std::cout << "PASS inclusive promotion\n";
}


void inclusive_invalidation_test()
{
    std::cout << "inclusive_invalidation_test\n";

    Data data{
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500}
    };

    std::vector<Cache_name_size> levels{
        {"LRU", 2, 0},
        {"LRU", 3, 0}
    };

    Multi_Level_Cach<
        int,
        int,
        Cach_Mode::Inclusive
    > cache(levels, data);


    check_access(cache, 1, false, 100);
    check_access(cache, 2, false, 200);
    check_access(cache, 3, false, 300);

    /*
        L1: [3, 2]
        L2: [3, 2, 1]
    */


    // Делаем 2 свежим только в L1.
    check_access(cache, 2, true, 200);

    /*
        L1: [2, 3]

        L2 НЕ изменяется:
        [3, 2, 1]
    */


    check_access(cache, 4, false, 400);

    /*
        L2:
            вставляем 4
            вытесняется 1

        L1:
            вставляем 4
            вытесняется 3

        Получаем примерно:

        L1: [4, 2]
        L2: [4, 3, 2]
    */


    // Снова делаем 2 горячим только в L1.
    check_access(cache, 2, true, 200);


    /*
        Теперь вставляем 5.

        В L2 LRU = 2.

        Поэтому L2 вытесняет 2.

        Но Inclusive требует:

            если 2 исчез из L2,
            он обязан исчезнуть и из L1.
    */

    check_access(cache, 5, false, 500);


    // Поэтому теперь 2 обязан быть полным MISS.
    check_access(cache, 2, false, 200);


    assert(cache.hits_level[0] == 2);
    assert(cache.hits_level[1] == 0);

    std::cout << "PASS inclusive invalidation\n";
}


// ============================================================
// EXCLUSIVE
// ============================================================

void exclusive_promotion_test()
{
    std::cout << "exclusive_promotion_test\n";

    Data data{
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500}
    };

    std::vector<Cache_name_size> levels{
        {"LRU", 2, 0},
        {"LRU", 3, 0}
    };

    Multi_Level_Cach<
        int,
        int,
        Cach_Mode::Exclusive
    > cache(levels, data);


    check_access(cache, 1, false, 100);
    check_access(cache, 2, false, 200);

    /*
        Exclusive:

        L1: [2, 1]
        L2: []
    */


    check_access(cache, 3, false, 300);

    /*
        L1 overflow:

        1 переезжает L1 -> L2

        L1: [3, 2]
        L2: [1]
    */


    check_access(cache, 4, false, 400);

    /*
        L1: [4, 3]
        L2: [2, 1]
    */


    // 1 должен быть найден именно на L2.
    check_access(cache, 1, true, 100);

    assert(cache.hits_level[0] == 0);
    assert(cache.hits_level[1] == 1);


    /*
        После hit:

        1 удаляется из L2
        и перемещается в L1.

        Из L1 при этом вытесняется 3,
        который переезжает в L2.

        L1: [1, 4]
        L2: [3, 2]
    */


    // Теперь 1 должен находиться на L1.
    check_access(cache, 1, true, 100);

    assert(cache.hits_level[0] == 1);
    assert(cache.hits_level[1] == 1);

    std::cout << "PASS exclusive promotion\n";
}


void exclusive_three_levels_test()
{
    std::cout << "exclusive_three_levels_test\n";

    Data data{
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500},
        {6, 600}
    };

    std::vector<Cache_name_size> levels{
        {"LRU", 2, 0},   // L1
        {"LRU", 2, 0},   // L2
        {"LRU", 2, 0}    // L3
    };

    Multi_Level_Cach<
        int,
        int,
        Cach_Mode::Exclusive
    > cache(levels, data);


    check_access(cache, 1, false, 100);
    check_access(cache, 2, false, 200);
    check_access(cache, 3, false, 300);
    check_access(cache, 4, false, 400);
    check_access(cache, 5, false, 500);


    /*
        Должно получиться:

        L1: [5, 4]
        L2: [3, 2]
        L3: [1]

        То есть 1 находится ТОЛЬКО на третьем уровне.
    */


    check_access(cache, 1, true, 100);

    assert(cache.hits_level[0] == 0);
    assert(cache.hits_level[1] == 0);
    assert(cache.hits_level[2] == 1);


    /*
        Теперь начинается каскад:

            1: L3 -> L1

        L1 вытесняет 4
            4 -> L2

        L2 вытесняет 2
            2 -> L3

        В результате 1 существует только в L1.
    */


    check_access(cache, 1, true, 100);

    assert(cache.hits_level[0] == 1);
    assert(cache.hits_level[1] == 0);
    assert(cache.hits_level[2] == 1);

    std::cout << "PASS exclusive three levels\n";
}


// ============================================================
// ОБЩИЕ ГРАНИЧНЫЕ ТЕСТЫ
// ============================================================

template <Cach_Mode Mode>
void missing_key_test()
{
    Data data{
        {1, 100}
    };

    std::vector<Cache_name_size> levels{
        {"LRU", 2, 0},
        {"LRU", 3, 0}
    };

    Multi_Level_Cach<int, int, Mode> cache(levels, data);

    bool exception_caught = false;

    try
    {
        cache.access(999);
    }
    catch(const std::runtime_error&)
    {
        exception_caught = true;
    }

    assert(exception_caught);
}


int main()
{
    inclusive_promotion_test();
    inclusive_invalidation_test();

    exclusive_promotion_test();
    exclusive_three_levels_test();

    missing_key_test<Cach_Mode::Inclusive>();
    missing_key_test<Cach_Mode::Exclusive>();

    std::cout << "\nALL MULTI LEVEL TESTS PASSED\n";

    return 0;
}