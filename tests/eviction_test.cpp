#include "crutch.hpp" // Проверяем собственный include <optional>.
#include "creat_cach.hpp"

#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
template<typename Cache>
void insert(Cache& cache, int key, int value, Erase_ELL<int, int> expected = {})
{
    static_assert(std::is_same_v<decltype(cache.insert_value(key, value)), Erase_ELL<int, int>>);
    auto erased = cache.insert_value(key, value);
    assert(erased.key_erase.has_value() == erased.value_erase.has_value());
    if(erased.key_erase != expected.key_erase || erased.value_erase != expected.value_erase)
    {
        std::cerr << "Unexpected eviction while inserting " << key << "=" << value << '\n';
        std::abort();
    }
    assert(cache.size() <= cache.capacity());
}

template<typename Cache>
void hit(Cache& cache, int key, int value)
{
    auto result = cache.look_up(key);
    assert(result.hit && result.found_ell && *result.found_ell == value);
}

void lru()
{
    Lru_cach<int, int> cache(2);
    insert(cache, 1, 100);
    insert(cache, 2, 200);
    insert(cache, 3, 300, {1, 100});
    assert(!cache.look_up(1).hit);
    insert(cache, 3, 999); // Дубликат не добавляет узел и не меняет Value.
    assert(cache.size() == 2);
    hit(cache, 3, 300);
    hit(cache, 2, 200); // head/MRU = 2, tail/LRU = 3.
    insert(cache, 4, 400, {3, 300});
    assert(cache.erase_key(2));
    assert(!cache.erase_key(2));
    insert(cache, 5, 500);
}

void lfu()
{
    Lfu_cach<int, int> cache(2);
    insert(cache, 1, 100);
    insert(cache, 2, 200);
    hit(cache, 1, 100); // Частоты 1:2 и 2:1 — однозначная жертва 2.
    insert(cache, 3, 300, {2, 200});
    insert(cache, 1, 999);
    hit(cache, 1, 100);
    assert(cache.erase_key(3));
    insert(cache, 4, 400);
    insert(cache, 5, 500, {4, 400});

    Lfu_cach<int, int> ties(2);
    insert(ties, 1, 100);
    insert(ties, 2, 200);
    insert(ties, 3, 300, {1, 100}); // Равные частоты: старейший в группе.
}

void two_q()
{
    Two_Q_Cach<int, int> cache(4); // Kin=1, Kout=2.
    for(int key = 1; key <= 4; ++key) insert(cache, key, key * 100);
    insert(cache, 5, 500, {1, 100}); // A1in -> A1out.
    assert(!cache.look_up(1).hit);
    assert(!cache.erase_key(1)); // Ghost не удаляется через erase_key.
    insert(cache, 1, 101, {2, 200}); // A1out -> Am, жертва из A1in.
    insert(cache, 2, 202, {3, 300});
    insert(cache, 3, 303, {4, 400});
    insert(cache, 4, 404, {1, 101}); // A1in == Kin: жертва теперь из Am.
    insert(cache, 4, 999);
    hit(cache, 4, 404);

    Two_Q_Cach<int, int> oldest_ghost(2); // Kout=1: не удалять найденный ghost до promotion.
    insert(oldest_ghost, 1, 100);
    insert(oldest_ghost, 2, 200);
    insert(oldest_ghost, 3, 300, {1, 100});
    insert(oldest_ghost, 1, 101, {2, 200});
    insert(oldest_ghost, 4, 400, {3, 300}); // Очистка A1out не заменяет resident-жертву.
    assert(oldest_ghost.erase_key(4));
    insert(oldest_ghost, 3, 303); // Возврат ghost в свободное место: eviction нет.
    hit(oldest_ghost, 3, 303);
}

void arc()
{
    Arc_cach<int, int> cache(2);
    insert(cache, 1, 100);
    insert(cache, 2, 200);
    hit(cache, 1, 100); // T2={1}, T1={2}.
    insert(cache, 3, 300, {2, 200}); // T1 -> B1.
    assert(!cache.look_up(2).hit);
    assert(!cache.erase_key(2));
    insert(cache, 2, 202, {1, 100}); // B1 hit, p=1; T2 -> B2.
    assert(!cache.look_up(1).hit);
    insert(cache, 1, 101, {3, 300}); // B2 hit, p=0; T1 -> B1.
    insert(cache, 4, 400, {2, 202});
    hit(cache, 4, 400); // T1 пуст, история B1={3}, B2={2}.
    insert(cache, 5, 500, {1, 101}); // Удаление B2 не должно вернуть {2,202}.
    insert(cache, 5, 999); // Resident T1 -> T2, eviction нет.
    insert(cache, 5, 999); // Resident T2 -> MRU, eviction нет.
    hit(cache, 5, 500);

    Arc_cach<int, int> direct(2);
    insert(direct, 1, 100);
    insert(direct, 2, 200);
    insert(direct, 3, 300, {1, 100}); // Полный T1: удаление без B1.

    Arc_cach<int, int> ghost_only(2);
    insert(ghost_only, 1, 100);
    insert(ghost_only, 2, 200);
    hit(ghost_only, 1, 100);
    insert(ghost_only, 3, 300, {2, 200});
    assert(ghost_only.erase_key(1));
    insert(ghost_only, 4, 400); // Только очистка B1={2}, resident-место уже свободно.
}

void lirs()
{
    Lirs_cach<int, int> cache(3, 1);
    insert(cache, 1, 100);
    insert(cache, 2, 200);
    insert(cache, 3, 300);
    insert(cache, 4, 400, {3, 300}); // HIR в S -> HIR_NO_RES.
    assert(!cache.look_up(3).hit);
    assert(!cache.erase_key(3));
    insert(cache, 3, 303, {4, 400}); // Ghost return: жертва HIR из Q.
    insert(cache, 4, 404, {1, 100}); // Жертва HIR вне S удаляется целиком.
    insert(cache, 4, 999);
    hit(cache, 4, 404);

    Lirs_cach<int, int> outside_s(3, 1);
    insert(outside_s, 1, 100);
    insert(outside_s, 2, 200);
    insert(outside_s, 3, 300);
    hit(outside_s, 3, 300); // 3 -> LIR, 1 -> HIR вне S; eviction нет.
    insert(outside_s, 4, 400, {1, 100});

    Lirs_cach<int, int> free_slot(3, 1);
    insert(free_slot, 1, 100);
    insert(free_slot, 2, 200);
    insert(free_slot, 3, 300);
    insert(free_slot, 4, 400, {3, 300});
    assert(free_slot.erase_key(2));
    insert(free_slot, 3, 303); // Возврат истории без вытеснения данных.
}

// Value без конструктора по умолчанию; ошибку копирования вызываем по содержимому,
// без зависимости от числа копирований или оптимизации возврата.
struct Payload
{
    int data;
    static inline int fail_on = -1;
    explicit Payload(int value): data(value) {}
    Payload(const Payload& other): data(other.data)
    {
        if(data == fail_on) throw std::runtime_error("Value copy failed");
    }
    Payload(Payload&&) = default;
    Payload& operator=(const Payload&) = default;
    Payload& operator=(Payload&&) = default;
};

template<typename Cache>
void copy_failure(Cache& cache)
{
    cache.insert_value(1, Payload{100});
    for(int fail_on : {100, 200}) // Сохранение жертвы / создание новой страницы.
    {
        Payload::fail_on = fail_on;
        bool caught = false;
        try { cache.insert_value(2, Payload{200}); }
        catch(const std::runtime_error&) { caught = true; }
        Payload::fail_on = -1;
        assert(caught && cache.size() == 1);
        assert(!cache.look_up(2).hit);
        assert(cache.look_up(1).found_ell->data == 100);
    }
    auto erased = cache.insert_value(2, Payload{200});
    assert(erased.key_erase == 1 && erased.value_erase && erased.value_erase->data == 100);
}

void variants()
{
    for(const std::string name : {"LRU", "LFU", "2Q", "ARC", "LIRS"})
    {
        auto zero = create_cache_one_ell<int, int>({name, 0, 0});
        std::visit([](auto& cache) {
            insert(cache, 5, 500, {5, 500});
            insert(cache, 5, 500, {5, 500});
            assert(cache.size() == 0 && !cache.look_up(5).hit);
        }, *zero);
        auto one = create_cache_one_ell<int, int>({name, 1, 1});
        std::visit([](auto& cache) {
            insert(cache, 5, 500);
            insert(cache, 8, 800, {5, 500});
            insert(cache, 8, 999);
            hit(cache, 8, 800);
        }, *one);

        auto strings = create_cache_one_ell<std::string, Payload>({name, 1, 1});
        auto erased = std::visit([](auto& cache) -> Erase_ELL<std::string, Payload> {
            auto first = cache.insert_value("old", Payload{500});
            assert(!first.key_erase && !first.value_erase);
            return cache.insert_value("new", Payload{800});
        }, *strings);
        strings.reset(); // Результат владеет значением и живёт после уничтожения кеша.
        assert(erased.key_erase == "old" && erased.value_erase->data == 500);
    }
}

// Независимый учёт resident-пар: не моделирует выбор жертвы алгоритмом.
// Значение меняется при каждой загрузке одного и того же ключа.
void random_values(const std::string& only)
{
    std::mt19937 rng(42);
    for(const std::string name : {"LRU", "LFU", "2Q", "ARC", "LIRS"})
    {
        if(!only.empty() && only != name)
        {
            rng.discard(5000);
            continue;
        }
        auto variant = create_cache_one_ell<int, int>({name, 8, 2});
        std::visit([&](auto& cache) {
            std::unordered_map<int, int> residents;
            for(int step = 1; step <= 5000; ++step)
            {
                int key = static_cast<int>(rng() % 24);
                auto found = residents.find(key);
                auto result = cache.look_up(key);
                assert(result.hit == (found != residents.end()));
                if(result.hit)
                {
                    assert(*result.found_ell == found->second);
                    continue;
                }
                auto erased = cache.insert_value(key, step);
                assert(erased.key_erase.has_value() == erased.value_erase.has_value());
                if(erased.key_erase)
                {
                    auto victim = residents.find(*erased.key_erase);
                    assert(victim != residents.end() && victim->second == *erased.value_erase);
                    residents.erase(victim);
                }
                residents.emplace(key, step);
                assert(cache.size() == residents.size() && cache.size() <= cache.capacity());
            }
        }, *variant);
        std::cout << "PASS 5000 requests: " << name << " (seed 42)" << std::endl;
    }
}
} // namespace

int main(int argc, char** argv)
{
    // Расширенный прогон отделён от ручных трасс: он воспроизводит прежний
    // дефект ARC REPLACE при пустом T1, B2 hit и p == 0 (см. README).
    if(argc >= 2 && std::string{argv[1]} == "--random")
    {
        random_values(argc == 3 ? argv[2] : "");
        return 0;
    }
    lru();
    lfu();
    two_q();
    arc();
    lirs();
    variants();
    Lru_cach<int, Payload> lru_throw(1);
    Lfu_cach<int, Payload> lfu_throw(1);
    copy_failure(lru_throw);
    copy_failure(lfu_throw);
    std::cout << "PASS eviction key/value: all 5 policies, ghost transitions, zero/one capacity, "
                 "duplicates, copy failures\n";
}
