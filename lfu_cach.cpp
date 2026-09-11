#include "lfu_cach.hpp"

void Lfu_cach::insert_new(int key)//создать новый эллемент и запись в хеш таблице
{
    lfu_cach.push_front(key);
    Node node{1, lfu_cach.begin()};
    hash_table.emplace(key, node);
    new_ell_frequency_table(key);//добавление нового эллемента в frequency_table
}

void Lfu_cach::relocation_frequency_table(int key_frequency, int key)
{
    //нужно взять частоту и перенести из одного списка в другой ключ
    auto freq_it = frequency_table.find(key_frequency);

    if (freq_it == frequency_table.end())
    {
        std::cerr << "Нет такой частоты\n";
        return;
    }

    auto& list_with_key = freq_it -> second;// бля нельзя здесь использовать копирование надо оришинал
    Iterator it = std::find(list_with_key.begin(), list_with_key.end(), key);

    if (it != list_with_key.end())
    {
        int new_key_frequency = key_frequency + 1;
        list_with_key.erase(it);

        if (list_with_key.empty())//удаляем если пустой
        {
            if (min_frequency == key_frequency)
            {
                min_frequency = new_key_frequency;//если это был минимальный список и он опустел меняем мин частоту
            }
            frequency_table.erase(key_frequency);
        }
        //и теперь надо определиться существует ли список с таким значением частоты
        auto it_frequency_table = frequency_table.find(new_key_frequency);

        if(it_frequency_table != frequency_table.end())
        {
            (it_frequency_table -> second).push_front(key);
        }
        else
        {
            frequency_table.emplace(new_key_frequency, std::list<int> {key});
        }
    }
    else
    {
        std::cerr << "не нашлось эллемента в new_key_frequency";
    }
}

void Lfu_cach::new_ell_frequency_table(int key)//добавление нового эллемента в frequency_table
{
    int val_freq = 1;
    auto freq_it = frequency_table.find(val_freq);

    if(freq_it != frequency_table.end())
    {
        freq_it -> second.push_front(key); 
    }
    else
    {
        frequency_table.emplace(val_freq, std::list<int> {key});
    }
    min_frequency = 1;
}

void Lfu_cach::evict_oldest()//удалить самый старый эллемент и запись в хеш таблице
{
    int min_frequency_key = get_key_oldest();
    Node& node_min_frequency = (hash_table.find(min_frequency_key)) -> second;
    lfu_cach.erase(node_min_frequency.position);//удалили из кэша самый старый эллемент
    hash_table.erase(min_frequency_key);//удалили из hash
}

void Lfu_cach::move_existing(int key)//переместить существующий эллемент
{
    Node& node_key = hash_table.find(key) -> second;
    relocation_frequency_table(node_key.frequency, key);
    node_key.frequency++;//увеличили частоту на 1
}

Lfu_cach::Lfu_cach(): capacity_(0), min_frequency(0)
{
}

explicit Lfu_cach::Lfu_cach(std::size_t capacity): capacity_(capacity), min_frequency(0)
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
        move_existing(key);
        //found_ell -> second = lru_cach.begin(); вроде не нужно
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