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

#include "lru_cach.hpp"

class Lfu_cach
{
    private:
        struct Node
        {
            int frequency;// частота
            Iterator position;
        };
        //можно это запихнуть в один ассоциативный конетейнер но это буде  очень нагромаждено
        std::list<int> lfu_cach;
        std::unordered_map<int, Node> hash_table;// здесь хранятится позиция и частота
        std::unordered_map<int, std::list<int>> frequency_table;

        std::size_t capacity_;
        int min_frequency;
        
        void insert_new(int key);//создать новый эллемент и запись в хеш таблице
        void evict_oldest();//удалить самый старый эллемент и запись в хеш таблице
        void move_existing(int key);//переместить существующий эллемент
        
        void new_ell_frequency_table(int key);
        void relocation_frequency_table(int key_frequency, int key);
        int get_key_oldest(); 

    public:
        Lfu_cach();
        explicit Lfu_cach(std::size_t capacity);

        Lfu_cach(const Lru_cach&) = delete;
        Lfu_cach& operator=(const Lru_cach&) = delete;

        void access(int key);//функция для обединения всего в одну систему;
        std::size_t size() const noexcept;
        std::size_t capacity() const noexcept;

        
};

#endif