#ifndef LRU_CACH
#define LRU_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <cassert>
#include <optional>

#include "crutch.hpp"



template <typename Key, typename Value>
class Lru_cach
{
    private:
        using Key_List = std::list<Entry<Key, Value>>;
        using Iterator = typename Key_List::iterator;

        Key_List lru_cach; //сам кэш
        std::unordered_map<Key, Iterator> hash_table;
        std::size_t capacity_;
        
        void make_recent(Iterator position);// перенести существующий узел списка в head

        void evict_oldest();// удалить самый давний элемент из списка и хеш-таблицы
        
    public:
        Access_Result<Value> look_up(const Key& key);

        Erase_ELL<Key, Value> insert_value(const Key& key, const Value& value);

        bool erase_key(const Key& key);

        Erase_ELL<Key, Value> find_del(const Key& key);

        explicit Lru_cach();
        explicit Lru_cach(std::size_t capacity);

        Lru_cach(const Lru_cach&) = delete;
        Lru_cach& operator=(const Lru_cach&) = delete;

        std::size_t size() const noexcept
        {
            return lru_cach.size();
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }              
};

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lru_cach<Key, Value>::insert_value(const Key& key, const Value& value)
{

    if (capacity_ == 0)
    {
       return {key, value};
    }

    if(hash_table.find(key) != hash_table.end())
    {
        return {};
    }

    // Копируем жертву до изменения контейнеров: Value может бросить исключение.
    Erase_ELL<Key, Value> erased;
    if(lru_cach.size() == capacity_)
    {
        const auto& victim = lru_cach.back();
        erased = {victim.key, victim.value};
    }

    lru_cach.push_front(Entry<Key, Value>{key, value});

    try
    {
        if(!hash_table.emplace(key, lru_cach.begin()).second)
        {
            lru_cach.pop_front();
            return {};
        }
    }
    catch (...)
    {
        lru_cach.pop_front();
        throw;
    }

    if(erased.key_erase)
    {
        evict_oldest();
    }

    return erased;
}

template <typename Key, typename Value>
bool Lru_cach<Key, Value>::erase_key(const Key& key)
{
    auto it = hash_table.find(key);

    if (it == hash_table.end())
        return false;

    lru_cach.erase(it -> second);
    hash_table.erase(it);

    return true;
}

template <typename Key, typename Value>
Access_Result<Value> Lru_cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return {false, nullptr};
    }

    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return {false, nullptr};
    }

    make_recent(found -> second);

    Entry<Key, Value>& entry = *(found -> second);
    return {true, &entry.value};
}


template <typename Key, typename Value>
Erase_ELL<Key, Value> Lru_cach<Key, Value>::find_del(const Key& key)
{
    auto it = hash_table.find(key);

    if (it == hash_table.end())
    {
       return {std::nullopt, std::nullopt};
    }

    Erase_ELL<Key, Value> del_ell = {key, it -> second -> value};

    lru_cach.erase(it -> second);
    hash_table.erase(it);

    return del_ell;
}

template <typename Key, typename Value>
void Lru_cach<Key, Value>::make_recent(Iterator position)// перенести существующий узел списка в head
{
    lru_cach.splice(lru_cach.begin(), lru_cach, position);
}

template <typename Key, typename Value>
void Lru_cach<Key, Value>::evict_oldest()// удалить самый давний элемент из списка и хеш-таблицы
{
    hash_table.erase(lru_cach.back().key);
    lru_cach.pop_back();
}

template <typename Key, typename Value>
Lru_cach<Key, Value>::Lru_cach(): capacity_(0)
{
}

template <typename Key, typename Value>
Lru_cach<Key, Value>::Lru_cach(std::size_t capacity): capacity_(capacity)
{
}

#endif
