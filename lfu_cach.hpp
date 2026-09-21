#ifndef LFU_CACH
#define LFU_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <optional>

#include "crutch.hpp"

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
        //можно это запихнуть в один ассоциативный конетейнер но это буде  очень нагромаждено
        Key_List lfu_cach;

        std::unordered_map<Key, Node> hash_table;// здесь хранятится позиция и частота
        std::unordered_map<int, Frequency_List> frequency_table;// нужно для опредения наименьшей частоты

        std::size_t capacity_;
        int min_frequency;
        
        void insert_new(const Key& key, const Value& value);//создать новый эллемент и запись в хеш таблице
        void evict_oldest();//удалить самый старый эллемент и запись в хеш таблице
        void move_existing(Node& node_key);//переместить существующий эллемент в начало
        
        Frequency_Iterator new_ell_frequency_table(const Key& key);
        void relocation_frequency_table(Node& node_key);
        Key get_key_oldest() const; 

    public:
        //новые функции
        Access_Result<Value> look_up(const Key& key);
        bool erase_key(const Key& key);
        std::optional<Key> insert_value(const Key& key, const Value& value);
        //конец
    
        explicit Lfu_cach();
        explicit Lfu_cach(std::size_t capacity);

        Lfu_cach(const Lfu_cach&) = delete;
        Lfu_cach& operator=(const Lfu_cach&) = delete;

        //bool access(const Key& key);//функция для обединения всего в одну систему;

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
std::optional<Key> Lfu_cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
    if(capacity_ == 0)
    {
        return key;
    }

    std::optional<Key> evicted_key = std::nullopt;//ключ на вытеснутого эллемента

    if(lfu_cach.size() == capacity_)
    {
        evicted_key = get_key_oldest();
    }

    // Сначала пытаемся безопасно вставить новый элемент.
    insert_new(key, value);

    // Только после успешной вставки удаляем старую жертву.
    if(evicted_key)
    {
        bool erased = erase_key(*evicted_key);
        assert(erased);//прсле отладки удалить
    }

    return evicted_key;
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

    (frequency_it -> second).erase(node.frequency_position);

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
        auto it = frequency_table.begin();

        min_frequency = it->first;

        ++it;

        for(; it != frequency_table.end(); ++it)
        {
            min_frequency =
                std::min(
                    min_frequency,
                    it->first
                );
        }
    }

    return true;
}

template <typename Key, typename Value>
Access_Result<Value> Lfu_cach<Key, Value>::look_up(const Key& key)
{
    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return {false, nullptr};
    }

    move_existing(found -> second);

    return {true, &(found -> second.position -> value)};//вернуть адресс эллемент ав кэше
}

// template <typename Key, typename Value>
// void Lfu_cach<Key, Value>::insert_new(const Key& key, const Value& value)//создать новый эллемент и запись в хеш таблице
// {
//     lfu_cach.push_front(key);
//     Iterator frequency_position = new_ell_frequency_table(key);
//     Node node{1, lfu_cach.begin(), frequency_position};

//     hash_table.emplace(key, node);
// }

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
    auto result = frequency_table.try_emplace(1);//если такого списка нет то она создаст
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
            frequency_table.erase(frequency_list);
        }

        throw;
    }

    min_frequency = 1;

    return frequency_list.begin();
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::evict_oldest()//удалить самый старый эллемент и запись в хеш таблице
{
    Key min_frequency_key = get_key_oldest();
    Node& node_min_frequency = (hash_table.find(min_frequency_key)) -> second;

    lfu_cach.erase(node_min_frequency.position);//удалили из кэша самый старый эллемент
    hash_table.erase(min_frequency_key);//удалили из hash
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::move_existing(Node& node_key)//переместить существующий эллемент
{
    relocation_frequency_table(node_key);
    node_key.frequency++;//увеличили частоту на 1
}

template <typename Key, typename Value>
Lfu_cach<Key, Value>::Lfu_cach(): capacity_(0), min_frequency(0)
{
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
            auto it_hash_table = (hash_table.emplace(key, Node{1, cache_position, frequency_position})).first;
        }
        catch(...)
        {
            auto frequency_it = frequency_table.find(1);//

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
Key Lfu_cach<Key, Value>::get_key_oldest() const// возвращает ключ наименее часто вызываемого обекта
{
    auto min_frequency_list = frequency_table.find(min_frequency);
    Key min_key = (min_frequency_list -> second).back();

    (min_frequency_list -> second).pop_back();//удаляем этот эллемент из таблицы 
    
    if (min_frequency_list -> second.empty())
    {
        frequency_table.erase(min_frequency_list);
    }

    return min_key; 
}

#endif