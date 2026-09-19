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

template <typename Key, typename Value>
class Lfu_cach
{
    private:
        using Key_List = std::list<Value>;
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

    return {true, &(*found -> second)};
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