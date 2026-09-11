#include "lru_cach.hpp"

void Lru_cach::access (int key)
{
    if(capacity_ == 0)
    {
        return;
    }

    auto found_ell = hash_table.find(key);

    if(found_ell != hash_table.end())
    {
        Lru_cach::make_recent(found_ell -> second);
        //found_ell -> second = lru_cach.begin(); вроде не нужно
        return;
    }

    Lru_cach::insert_new(key);

    if(lru_cach.size() > capacity_)
    {
        Lru_cach::evict_oldest();
        return;
    }

    return;
}

void Lru_cach::make_recent(Iterator position)// перенести существующий узел списка в head
{
    lru_cach.splice(lru_cach.begin(), lru_cach, position);

}

void Lru_cach::evict_oldest()// удалить самый давний элемент из списка и хеш-таблицы
{
    int key = lru_cach.back();
    hash_table.erase(key);
    lru_cach.pop_back();
}

void Lru_cach::insert_new(int key)
{
    //TODO нейронка предлагает налепить try и catchБ это надо сделать
    lru_cach.push_front(key);
    hash_table.emplace(key, lru_cach.begin());
}

Lru_cach::Lru_cach(): capacity_(0)
{
}

Lru_cach::Lru_cach(std::size_t capacity): capacity_(capacity)
{
}

std::size_t Lru_cach::size() const noexcept
{
    return lru_cach.size();
}

std::size_t Lru_cach::capacity() const noexcept
{
    return capacity_;
}

void Lru_cach::output()
{
    for(int copy : lru_cach)
    {
        std::cout << copy << " ";
    }
}
