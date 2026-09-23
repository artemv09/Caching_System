#ifndef LFU_CACH_
#define LFU_CACH_

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <optional>
#include <cassert>

#include "cach_type.hpp"

template <typename Key, typename Value>
class Lfu_cach
{
    private:
        using Key_List = std::list<Entry<Key, Value>>;
        using Iterator = typename Key_List::iterator;

        using Frequency_List = std::list<Key>;
        using Frequency_Iterator = typename Frequency_List::iterator;

        struct Node
        {
            int frequency;// частота
            Iterator position;
            Frequency_Iterator frequency_position;//позиция в frequency 
        };

        Key_List lfu_cach;

        std::unordered_map<Key, Node> hash_table; // здесь хранятится позиция и частота
        std::unordered_map<int, Frequency_List> frequency_table; // нужно для опредения наименьшей частоты

        std::size_t capacity_;
        int min_frequency;
        
        void insert_new(const Key& key, const Value& value); //создать новый эллемент и запись в хеш таблице

        void move_existing(Node& node_key); //переместить существующий эллемент в начало
        
        Frequency_Iterator new_ell_frequency_table(const Key& key); // создаем новый эллемент в хэш

        void relocation_frequency_table(Node& node_key); // меняем место в хэш таблице

        Key get_key_oldest(); // возвращает ключ наименее часто вызываемого обекта

    public:
        Value* look_up(const Key& key);

        Erase_ELL<Key, Value> extract_entry(const Key& key);

        Erase_ELL<Key, Value> insert_value(const Key& key, const Value& value);

        bool erase_key(const Key& key);
    
        explicit Lfu_cach(std::size_t capacity);
 
        Lfu_cach(const Lfu_cach&) = delete;
        Lfu_cach& operator=(const Lfu_cach&) = delete;

        std::size_t size() const noexcept
        {
            return lfu_cach.size();
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }
};

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lfu_cach<Key, Value>::extract_entry(const Key& key)
{
    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return std::nullopt;
    }

    Erase_ELL<Key, Value> erased = Entry<Key, Value>{key, ((found -> second).position) -> value};

    bool success = erase_key(key);

    return erased;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lfu_cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
    if(capacity_ == 0)
    {
        return Entry<Key, Value>{key, value};
    }

    if(hash_table.find(key) != hash_table.end())
    {
        return std::nullopt;
    }

    Erase_ELL<Key, Value> erased;

    if(lfu_cach.size() == capacity_) // получаем данные о самом старом эллементе
    {
        Key cach_oldest_ell = get_key_oldest();
        auto it_oldes_ell = hash_table.find(cach_oldest_ell);
        erased = Entry<Key, Value>{cach_oldest_ell, ((it_oldes_ell -> second).position) -> value};
    }

    insert_new(key, value);

    if(erased)
    {
        erase_key(erased -> key);
    }

    return erased;
}

template <typename Key, typename Value>
bool Lfu_cach<Key, Value>::erase_key(const Key& key)
{
    auto hash_table_it = hash_table.find(key);

    if(hash_table_it == hash_table.end())
    {
        return false;
    }

    Node& node = hash_table_it -> second;
    int frequency = node.frequency;

    auto frequency_it = frequency_table.find(frequency);

    (frequency_it -> second).erase(node.frequency_position); // удалить из списка данных частот

    const bool frequency_list_empty = (frequency_it -> second).empty();

    if(frequency_list_empty)
    {
        frequency_table.erase(frequency_it);
    }

    lfu_cach.erase(node.position);

    hash_table.erase(hash_table_it);

    if(hash_table.empty())
    {
        min_frequency = 0;
        return true;
    }

    if(frequency_list_empty && frequency == min_frequency)
    {
        auto min_it = std::min_element(frequency_table.begin(), frequency_table.end(),
            [](const auto& lhs, const auto& rhs)
            {
                return lhs.first < rhs.first;
            }
        );

        min_frequency = min_it -> first;
    }

    return true;
}

template <typename Key, typename Value>
Value* Lfu_cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return nullptr;
    }

    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return nullptr;
    }

    move_existing(found -> second);

    return &((found -> second).position) -> value; //вернуть адресс эллемента в кэше
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::relocation_frequency_table(Node& node_key)
{
    int key_frequency = node_key.frequency;
    int new_key_frequency = key_frequency + 1;

    auto& list_with_key = frequency_table.find(key_frequency) -> second;

    auto result = frequency_table.try_emplace(new_key_frequency);
    auto& new_list = result.first -> second;

    new_list.splice(new_list.begin(), list_with_key, node_key.frequency_position);

    if (list_with_key.empty())
    {
        if (min_frequency == key_frequency)
        {
            min_frequency = new_key_frequency;
        }

        frequency_table.erase(key_frequency);
    }
}

template <typename Key, typename Value>
typename Lfu_cach<Key, Value>::Frequency_Iterator 
Lfu_cach<Key, Value>::new_ell_frequency_table(const Key& key)//добавление нового эллемента в frequency_table
{
    auto result = frequency_table.try_emplace(1); //если такого списка нет то она создаст
    auto& frequency_list = result.first -> second;
    bool create_success = result.second;

    try
    {
        frequency_list.push_front(key);
    }
    catch(...)
    {
        if(create_success && frequency_list.empty())
        {
            frequency_table.erase(result.first);
        }

        throw;
    }

    return frequency_list.begin();
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::move_existing(Node& node_key) 
{
    relocation_frequency_table(node_key);
    node_key.frequency++; //увеличили частоту на 1
}

template <typename Key, typename Value>
Lfu_cach<Key, Value>::Lfu_cach(std::size_t capacity): capacity_(capacity), min_frequency(0)
{
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::insert_new(const Key& key, const Value& value)
{
    lfu_cach.push_front(Entry<Key, Value>{key, value});
    Iterator cache_position = lfu_cach.begin();

    try
    {
        Frequency_Iterator frequency_position = new_ell_frequency_table(key);//сама отвечает за безопасность выделения 

        try
        {
            hash_table.emplace(key, Node{1, cache_position, frequency_position});
        }
        catch(...)
        {
            auto frequency_it = frequency_table.find(1);

            (frequency_it -> second).erase(frequency_position);

            if((frequency_it -> second).empty())
            {
                frequency_table.erase(frequency_it);
            }

            throw;
        }
    }
    catch(...)
    {
        lfu_cach.erase(cache_position);
        throw;
    }

    min_frequency = 1;
}

template <typename Key, typename Value>
Key Lfu_cach<Key, Value>::get_key_oldest()
{
    auto min_frequency_list = frequency_table.find(min_frequency);
    Key min_key = (min_frequency_list -> second).back();

    return min_key; 
}

#endif
