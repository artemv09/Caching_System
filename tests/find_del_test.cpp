#include "creat_cach.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace
{
void expect(const Erase_ELL<int, int>& result, Erase_ELL<int, int> expected = {})
{
    assert(result.key_erase.has_value() == result.value_erase.has_value());
    assert(result.key_erase == expected.key_erase);
    assert(result.value_erase == expected.value_erase);
}

template<typename Cache>
void insert(Cache& cache, int key, Erase_ELL<int, int> expected = {})
{
    expect(cache.insert_value(key, key * 100), expected);
    assert(cache.size() <= cache.capacity());
}

template<typename Cache>
void extract(Cache& cache, int key, int value)
{
    static_assert(std::is_same_v<decltype(cache.find_del(key)), Erase_ELL<int, int>>);
    auto before = cache.size();
    expect(cache.find_del(key), {key, value});
    assert(cache.size() + 1 == before);
    assert(!cache.look_up(key).hit);
    expect(cache.find_del(key)); // Повтор не удаляет history и не меняет счётчики.
    assert(cache.size() + 1 == before);
}

void lfu()
{
    Lfu_cach<int, int> cache(3);
    expect(cache.find_del(99));
    insert(cache, 1);
    insert(cache, 2);
    insert(cache, 3);
    assert(cache.look_up(1).hit);
    assert(cache.look_up(1).hit);
    assert(cache.look_up(2).hit); // Частоты: 1:3, 2:2, 3:1.
    extract(cache, 3, 300); // Удаляется единственная запись минимальной частоты.
    assert(cache.look_up(2).hit); // Теперь min_frequency должна перейти с 2 на 3.
    insert(cache, 4);
    extract(cache, 4, 400);
    insert(cache, 5);
    assert(cache.look_up(5).hit);
    assert(cache.look_up(5).hit);
    insert(cache, 6, {1, 100}); // Все частоты равны 3: старейшая запись — 1.
    extract(cache, 2, 200);
    extract(cache, 5, 500);
    extract(cache, 6, 600);
    assert(cache.size() == 0);
    insert(cache, 7);
    extract(cache, 7, 700);

    Lfu_cach<int, int> middle(3);
    insert(middle, 1);
    insert(middle, 2);
    insert(middle, 3);
    extract(middle, 2, 200); // Удаление из середины частотной группы.
    insert(middle, 4);
    insert(middle, 5, {1, 100});
}

void two_q()
{
    Two_Q_Cach<int, int> cache(4);
    for(int key = 1; key <= 4; ++key) insert(cache, key);
    extract(cache, 3, 300); // Произвольный узел A1in, не хвост.
    expect(cache.insert_value(3, 303)); // Возврат A1out -> Am на свободное место.
    insert(cache, 5, {1, 100});
    insert(cache, 6, {2, 200});
    insert(cache, 7, {4, 400}); // Если история 3 потерялась, здесь жертвой была бы 3.
    extract(cache, 3, 303); // Из Am удаляется полностью.
    expect(cache.insert_value(3, 333)); // Новый ключ -> A1in, отдельной истории Am нет.
    insert(cache, 8, {5, 500});
    insert(cache, 9, {6, 600});
    insert(cache, 10, {7, 700});
    insert(cache, 11, {3, 333});

    Two_Q_Cach<int, int> bounded(2); // Kout=1.
    insert(bounded, 1);
    insert(bounded, 2);
    extract(bounded, 1, 100);
    extract(bounded, 2, 200); // Удаляется ghost 1; результат всё равно {2,200}.
    insert(bounded, 1); // Холодная загрузка в A1in.
    insert(bounded, 2); // Сохранённая история -> Am.
    insert(bounded, 3, {1, 100});

    Two_Q_Cach<int, int> zero_history(1); // Kout=0.
    insert(zero_history, 1);
    extract(zero_history, 1, 100);
    insert(zero_history, 2);
    insert(zero_history, 1, {2, 200});
}

void arc()
{
    Arc_cach<int, int> recent(3);
    insert(recent, 1);
    insert(recent, 2);
    insert(recent, 3);
    extract(recent, 2, 200); // T1 -> B1, узел из середины списка.
    insert(recent, 2); // B1 -> T2. Повторный find_del ghost не уничтожил историю.
    insert(recent, 4, {1, 100});
    insert(recent, 5, {3, 300});
    insert(recent, 6, {4, 400}); // Холодная вставка 2 привела бы к её вытеснению здесь.
    extract(recent, 2, 200);

    Arc_cach<int, int> frequent(3);
    insert(frequent, 1);
    insert(frequent, 2);
    insert(frequent, 3);
    assert(frequent.look_up(1).hit);
    assert(frequent.look_up(2).hit);
    extract(frequent, 2, 200); // T2 -> B2: удаляется MRU, а не хвост 1.
    insert(frequent, 4);
    insert(frequent, 2, {3, 300}); // B2 -> T2.
    insert(frequent, 5, {4, 400});
    insert(frequent, 6, {5, 500});
    extract(frequent, 2, 200);

    // При заполненной истории find_del должна ограничить B1+B2,
    // не сообщая удалённый ghost в качестве resident-результата.
    for(int key_to_extract : {4, 1})
    {
        Arc_cach<int, int> bounded(2);
        insert(bounded, 1);
        insert(bounded, 2);
        assert(bounded.look_up(1).hit);
        insert(bounded, 3, {2, 200});
        insert(bounded, 2, {1, 100});
        insert(bounded, 1, {3, 300});
        insert(bounded, 4, {2, 200}); // T1={4}, T2={1}, B1={3}, B2={2}, p=0.
        extract(bounded, key_to_extract, key_to_extract * 100);
        insert(bounded, 2); // Хвост B2={2} очищен: теперь это холодная вставка.
        if(key_to_extract == 4)
        {
            insert(bounded, 5, {2, 200});
        }
        else
        {
            insert(bounded, 5, {4, 400});
            insert(bounded, 6, {2, 200});
        }
    }
}

void lirs()
{
    Lirs_cach<int, int> hir(3, 1);
    insert(hir, 1);
    insert(hir, 2);
    insert(hir, 3);
    extract(hir, 3, 300); // HIR в S остаётся non-resident history.
    insert(hir, 3); // Возврат истории делает 3 LIR и понижает 1 в HIR.
    insert(hir, 4, {1, 100});

    Lirs_cach<int, int> lir(3, 1);
    insert(lir, 1);
    insert(lir, 2);
    insert(lir, 3);
    extract(lir, 2, 200); // LIR выше дна S: история пока сохраняется.
    insert(lir, 4); // Заполнение освободившейся LIR-квоты.
    insert(lir, 2, {3, 300});
    insert(lir, 5, {1, 100}); // Подтверждает возврат 2 из истории в LIR.

    Lirs_cach<int, int> pruned(3, 1);
    insert(pruned, 1);
    insert(pruned, 2);
    insert(pruned, 3);
    extract(pruned, 1, 100); // Нижняя LIR: prune_S удаляет её историю.
    insert(pruned, 4);
    insert(pruned, 1, {3, 300}); // Холодная загрузка как HIR.
    insert(pruned, 5, {1, 100});

    Lirs_cach<int, int> outside_s(3, 1);
    insert(outside_s, 1);
    insert(outside_s, 2);
    insert(outside_s, 3);
    assert(outside_s.look_up(3).hit); // 1 -> HIR вне S.
    extract(outside_s, 1, 100);
    insert(outside_s, 4);
    insert(outside_s, 1, {4, 400});
}

struct Payload
{
    std::string text;
    static inline bool throw_on_copy = false;
    explicit Payload(std::string value): text(std::move(value)) {}
    Payload(const Payload& other): text(other.text)
    {
        if(throw_on_copy) throw std::runtime_error("copy failed");
    }
    Payload(Payload&&) = default;
    Payload& operator=(const Payload&) = default;
    Payload& operator=(Payload&&) = default;
};

void common()
{
    for(const std::string name : {"LRU", "LFU", "2Q", "ARC", "LIRS"})
    {
        auto zero = create_cache_one_ell<int, int>({name, 0, 0});
        std::visit([](auto& cache) {
            expect(cache.find_del(1));
            insert(cache, 1, {1, 100});
            expect(cache.find_del(1));
            assert(cache.size() == 0);
        }, *zero);

        auto cache = create_cache_one_ell<std::string, Payload>({name, 3, 1});
        std::visit([](auto& policy) {
            auto result = policy.find_del("absent");
            assert(!result.key_erase && !result.value_erase);
            auto inserted = policy.insert_value("key", Payload{"original value"});
            assert(!inserted.key_erase && !inserted.value_erase);
        }, *cache);
        Payload::throw_on_copy = true;
        bool caught = false;
        try { std::visit([](auto& policy) { policy.find_del("key"); }, *cache); }
        catch(const std::runtime_error&) { caught = true; }
        Payload::throw_on_copy = false;
        assert(caught);

        auto result = std::visit([](auto& policy) -> Erase_ELL<std::string, Payload> {
            assert(policy.size() == 1);
            // Без look_up: он изменил бы T1 в T2 и не проверил бы прежний путь.
            return policy.find_del("key");
        }, *cache);
        std::visit([](auto& policy) {
            assert(policy.size() == 0 && !policy.look_up("key").hit);
            auto second = policy.find_del("key");
            assert(!second.key_erase && !second.value_erase);
        }, *cache);
        cache.reset();
        assert(result.key_erase.has_value() == result.value_erase.has_value());
        assert(result.key_erase == "key" && result.value_erase->text == "original value");
    }
}
} // namespace

int main()
{
    lfu();
    two_q();
    arc();
    lirs();
    common();
    std::cout << "PASS find_del: LFU, 2Q, ARC, LIRS; ghost history, limits, counters, "
                 "zero capacity, copy failures, owning values and std::visit\n";
}
