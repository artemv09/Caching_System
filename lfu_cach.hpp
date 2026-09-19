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

#include "crutch.hpp"


template <typename Key, typename Value>
class Lfu_cach
{
    private:
        using Key_List = std::list<Entry<Key, Value>>;
        using Iterator = typename Key_List::iterator;
        struct Node
        {
            int frequency;// частота
            Iterator position;
            Iterator frequency_position;//позиция в frequency 
        };
        //можно это запихнуть в один ассоциативный конетейнер но это буде  очень нагромаждено
        Key_List lfu_cach;
        std::unordered_map<Key, Node> hash_table;// здесь хранятится позиция и частота
        std::unordered_map<int, Key_List> frequency_table;// нужно для опредения наименьшей частоты

        std::size_t capacity_;
        int min_frequency;
        
        void insert_new(const Key& key);//создать новый эллемент и запись в хеш таблице
        void evict_oldest();//удалить самый старый эллемент и запись в хеш таблице
        void move_existing(Node& node_key);//переместить существующий эллемент в начало
        
        Iterator new_ell_frequency_table(const Key& key);
        void relocation_frequency_table(Node& node_key);
        Key get_key_oldest(); 

    public:
        //новые функции
        Access_Result<Value> look_up(const Key& key);
        bool erase_key(const Key& key);
        std::optional<Key> Lfu_cach<Key, Value>::insert_value(const Key& key, const Value& value);
        //конец
    
        explicit Lfu_cach();
        explicit Lfu_cach(std::size_t capacity);

        Lfu_cach(const Lfu_cach&) = delete;
        Lfu_cach& operator=(const Lfu_cach&) = delete;

        bool access(const Key& key);//функция для обединения всего в одну систему;

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
    // По контракту функция вызывается только после miss.
    assert(hash_table.find(key) == hash_table.end());

    if (capacity_ == 0)
        return key;

    std::optional<Key> evicted_key;

    // Если кэш заполнен, заранее определяем жертву.
    if (hash_table.size() == capacity_)
    {
        auto freq_it = frequency_table.find(min_frequency_);

        assert(freq_it != frequency_table.end());
        assert(!freq_it->second.empty());

        // Самый старый среди элементов с минимальной частотой.
        evicted_key = freq_it->second.back();
    }

    // Сначала создаём сам Entry.
    lfu_cache.push_front(Entry{key, value});

    auto cache_position = lfu_cache.begin();

    // Новый элемент всегда имеет frequency = 1.
    auto& frequency_one = frequency_table[1];

    try
    {
        frequency_one.push_front(key);
    }
    catch (...)
    {
        lfu_cache.pop_front();

        if (frequency_one.empty())
            frequency_table.erase(1);

        throw;
    }

    auto frequency_position = frequency_one.begin();

    try
    {
        auto [it, inserted] = hash_table.emplace(key, Node {1, cache_position, frequency_position});

    }
    catch (...)
    {
        frequency_one.erase(frequency_position);
        lfu_cache.erase(cache_position);

        if (frequency_one.empty())
            frequency_table.erase(1);

        throw;
    }

    // После вставки нового элемента минимальная частота всегда 1.
    min_frequency_ = 1;

    // Теперь безопасно удаляем старую жертву.
    if(evicted_key)
    {
        erase_key(*evicted_key);
    }

    return evicted_key;
}

template <typename Key, typename Value>
bool Lfu_cach<Key, Value>::erase_key(const Key& key)
{
    auto hash_it = hash_table.find(key);

    if (hash_it == hash_table.end())
        return false;

    const std::size_t frequency = hash_it -> second.frequency;

    auto freq_it = frequency_table.find(frequency);

    // Удаляем ключ из его frequency bucket.
    freq_it -> second.erase(hash_it -> second.frequency_position);

    // Удаляем настоящий Entry.
    lfu_cache.erase(hash_it -> second.cache_position);

    // Удаляем Key -> Node.
    hash_table.erase(hash_it);

    // Если frequency bucket опустел — удаляем его.
    if (freq_it -> second.empty())
    {
        frequency_table.erase(freq_it);

        if (frequency == min_frequency_)
        {
            if (hash_table.empty())
            {
                min_frequency_ = 0;
            }
            else
            {
                // Находим новую минимальную frequency.
                auto it = frequency_table.begin();

                min_frequency_ = it -> first;
                ++it;

                while (it != frequency_table.end())
                {
                    min_frequency_ = std::min(min_frequency_, it->first);
                    ++it;
                }
            }
        }
    }

    return true;
}

template <typename Key, typename Value>
Access_Result<Value> Lfu_cach<Key, Value>::look_up(const Key& key)
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

    move_existing(found -> second);

    return {true, &(*found -> second.position)};
}

template <typename Key, typename Value>
void Lfu_cach<Key, Value>::insert_new(const Key& key)//создать новый эллемент и запись в хеш таблице
{
    lfu_cach.push_front(key);
    Iterator frequency_position = new_ell_frequency_table(key);
    Node node{1, lfu_cach.begin(), frequency_position};

    hash_table.emplace(key, node);
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
typename Lfu_cach<Key, Value>::Iterator Lfu_cach<Key, Value>::new_ell_frequency_table(const Key& key)//добавление нового эллемента в frequency_table
{
    auto result = frequency_table.try_emplace(1);//если такого списка нет то она создаст
    auto& frequency_list = result.first -> second;

    frequency_list.push_front(key);
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
bool Lfu_cach<Key, Value>::access(const Key& key)//функция для обедлинения всего в одну систему
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found_ell = hash_table.find(key);

    if(found_ell != hash_table.end())
    {
        move_existing(found_ell -> second);
        return true;
    }

    if(lfu_cach.size() == capacity_)
    {
        evict_oldest();
        insert_new(key);
        return false;
    }

    insert_new(key);

    return false;
}

template <typename Key, typename Value>
Key Lfu_cach<Key, Value>::get_key_oldest()// возвращает ключ наименее часто вызываемого обекта
{
    auto min_frequency_list_key = frequency_table.find(min_frequency);
    Key min_key = (min_frequency_list_key -> second).back();
    (min_frequency_list_key -> second).pop_back();//удаляем этот эллемент из таблицы 
    
    if (min_frequency_list_key -> second.empty())
    {
        frequency_table.erase(min_frequency_list_key);
    }
    return min_key; 
}

#endif