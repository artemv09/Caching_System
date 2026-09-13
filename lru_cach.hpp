#ifndef LRU_CACH
#define LRU_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>

using Iterator = std::list<int>::iterator;

class Lru_cach
{
    private:
        std::list<int> lru_cach; //сам кэш
        std::unordered_map<int, Iterator> hash_table;
        std::size_t capacity_;
        
        void make_recent(Iterator position);// перенести существующий узел списка в head
        void evict_oldest();// удалить самый давний элемент из списка и хеш-таблицы
        void insert_new(int key);// добавить новый узел списка и соответствующую запись в хеш-таблицу

    public:
        explicit Lru_cach();
        explicit Lru_cach(std::size_t capacity);

        Lru_cach(const Lru_cach&) = delete;
        Lru_cach& operator=(const Lru_cach&) = delete;

        bool access(int key);//функция для обединения всего в одну систему

        std::size_t size() const noexcept;
        std::size_t capacity() const noexcept;

        void output();
};


#endif
