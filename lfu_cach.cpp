#include "lfu_cach.hpp"

void Lfu_cach::insert_new(int key)//создать новый эллемент и запись в хеш таблице
{
    lru_cach.push_front(key);
    hash_table.emplace(key, lru_cach.begin());
    frequency_table.emplace();
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

}

void Lfu_cach::evict_oldest()//удалить самый старый эллемент и запись в хеш таблице
{

}

void Lfu_cach::move_existing(int key)//переместить существующий эллемент
{

}

Lfu_cach::Lfu_cach(): capacity_(0), min_frequency(0)
{
}

explicit Lfu_cach::Lfu_cach(std::size_t capacity): capacity_(capacity), min_frequency(0)
{
}


void Lfu_cach::access(int key)//функция для обедлинения всего в одну систему
{

}

int Lfu_cach::get_key_oldest()// возвращает ключ наименее часто вызываемого обекта
{
    auto min_frequency_list_key = frequency_table.find(min_frequency);
    int min_key = (min_frequency_list_key -> second).back();
    (min_frequency_list_key -> second).pop_back();//удаляем этот эллемент из таблицы 
    return min_key; //TODO возможно здесь надо будет руками менять min_frequency
}

std::size_t Lfu_cach::size() const noexcept
{
    return lfu_cach.size();
}

std::size_t Lfu_cach::capacity() const noexcept
{
    return capacity_;
}