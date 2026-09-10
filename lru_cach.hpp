#ifndef LRU_CACH
#define LRU_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>

using Iterator = std::list<int>::iterator;

class Lru_cach
{
    private:
        std::list<int> lru_cach; //сам кэш
        std::unordered_map<int, Iterator> hash_table;
        std::size_t capacity;
        std::size_t quantity_fill_elements;
        bool fill_flag; // если есть свободное место то true, нет то false

        void make_recent(Iterator position);// перенести существующий узел списка в head
        void evict_oldest();// удалить самый давний элемент из списка и хеш-таблицы
        void insert_new(int key);// добавить новый узел списка и соответствующую запись в хеш-таблицу
        
        void addition_hash_table(int key);
        void delete_hash_table();

    public:
        Lru_cach();
        Lru_cach(std::size_t capacity);

        bool access(int key);
        bool find_key_in_hash_table(int key);
        //TODO написать метод для вставки в хеш таблицу, удаления из хеш таблицы 
};

#endif
