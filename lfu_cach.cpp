#include "lfu_cach.hpp"

void Lfu_cach::insert_new(int key)//создать новый эллемент и запись в хеш таблице
{
    lfu_cach.push_front(key);
    Iterator frequency_position = new_ell_frequency_table(key);
    Node node{1, lfu_cach.begin(), frequency_position};

    hash_table.emplace(key, node);
}

void Lfu_cach::relocation_frequency_table(Node& node_key)
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

Iterator Lfu_cach::new_ell_frequency_table(int key)//добавление нового эллемента в frequency_table
{
    auto result = frequency_table.try_emplace(1);//если такого списка нет то она создаст
    auto& frequency_list = result.first -> second;

    frequency_list.push_front(key);
    min_frequency = 1;

    return frequency_list.begin();
}

void Lfu_cach::evict_oldest()//удалить самый старый эллемент и запись в хеш таблице
{
    int min_frequency_key = get_key_oldest();
    Node& node_min_frequency = (hash_table.find(min_frequency_key)) -> second;
    lfu_cach.erase(node_min_frequency.position);//удалили из кэша самый старый эллемент
    hash_table.erase(min_frequency_key);//удалили из hash
}

void Lfu_cach::move_existing(Node& node_key)//переместить существующий эллемент
{
    relocation_frequency_table(node_key);
    node_key.frequency++;//увеличили частоту на 1
}

Lfu_cach::Lfu_cach(): capacity_(0), min_frequency(0)
{
}

Lfu_cach::Lfu_cach(std::size_t capacity): capacity_(capacity), min_frequency(0)
{
}


void Lfu_cach::access(int key)//функция для обедлинения всего в одну систему
{
    if(capacity_ == 0)
    {
        return;
    }

    auto found_ell = hash_table.find(key);

    if(found_ell != hash_table.end())
    {
        move_existing(found_ell -> second);
        return;
    }

    if(lfu_cach.size() == capacity_)
    {
        evict_oldest();
        insert_new(key);
        return;
    }

    insert_new(key);

    return;
}

int Lfu_cach::get_key_oldest()// возвращает ключ наименее часто вызываемого обекта
{
    auto min_frequency_list_key = frequency_table.find(min_frequency);
    int min_key = (min_frequency_list_key -> second).back();
    (min_frequency_list_key -> second).pop_back();//удаляем этот эллемент из таблицы 
    
    if (min_frequency_list_key -> second.empty())
    {
        frequency_table.erase(min_frequency_list_key);
    }
    return min_key; 
}

std::size_t Lfu_cach::size() const noexcept
{
    return lfu_cach.size();
}

std::size_t Lfu_cach::capacity() const noexcept
{
    return capacity_;
}