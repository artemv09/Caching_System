#include "lru_cach.hpp"

bool Lru_cach::access (int key)
{

}

void Lru_cach::make_recent(Iterator position);// перенести существующий узел списка в head
{

}
void Lru_cach::evict_oldest();// удалить самый давний элемент из списка и хеш-таблицы
{

}
void Lru_cach::insert_new(int key)
{

}

Lru_cach::Lru_cach(): capacity(0), quantity_fill_elements(0), fill_flag(false)
{
}

Lru_cach::Lru_cach(std::size_t capacity): capacity(capacity), quantity_fill_elements(0), fill_flag(true)
{
}

bool Lru_cach::access(int key);
{

}

bool Lru_cach::find_key_in_hash_table(int key);
{

}

void Lru_cach::addition_hash_table(int key);
{

}

void Lru_cach::delete_hash_table();
{

}