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

#include "work_cach.hpp"


template <typename Key, typename Value>
class Lru_cach
{
    private:
        using Key_List = std::list<Value>;
        using Iterator = typename Key_List::iterator;

        Key_List lru_cach; //сам кэш
        std::unordered_map<Key, Iterator> hash_table;
        std::size_t capacity_;
        
        void make_recent(Iterator position);// перенести существующий узел списка в head
        void evict_oldest();// удалить самый давний элемент из списка и хеш-таблицы
        void insert_new(const Key& key);// добавить новый узел списка и соответствующую запись в хеш-таблицу

    public:
        //здесь располагаются новые функции
        Access_Result<Value> look_up(const Key& key);

        //конец
        explicit Lru_cach();
        explicit Lru_cach(std::size_t capacity);

        Lru_cach(const Lru_cach&) = delete;
        Lru_cach& operator=(const Lru_cach&) = delete;

        bool access(const Key& key);//функция для обединения всего в одну систему

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
bool Lru_cach<Key, Value>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found_ell = hash_table.find(key);

    if(found_ell != hash_table.end())
    {
        make_recent(found_ell -> second);
        //found_ell -> second = lru_cach.begin(); вроде не нужно
        return true;
    }

    insert_new(key);

    if(lru_cach.size() > capacity_)
    {
        evict_oldest();
        return false;
    }

    return false;
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

    return {true, &(*found -> second)};
}

template <typename Key, typename Value>
void Lru_cach<Key, Value>::make_recent(Iterator position)// перенести существующий узел списка в head
{
    lru_cach.splice(lru_cach.begin(), lru_cach, position);
}

template <typename Key, typename Value>
void Lru_cach<Key, Value>::evict_oldest()// удалить самый давний элемент из списка и хеш-таблицы
{
    Key key = lru_cach.back();
    hash_table.erase(key);
    lru_cach.pop_back();
}

template <typename Key, typename Value>
void Lru_cach<Key, Value>::insert_new(const Key&  key)
{
    //TODO нейронка предлагает налепить try и catch это надо сделать
    lru_cach.push_front(key);
    hash_table.emplace(key, lru_cach.begin());
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
