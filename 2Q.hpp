#ifndef Q_CACH
#define Q_CACH

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>

#include "lfu_cach.hpp"

class Two_Q_Cach
{
    private://в принципе я могу использовать интерфейс LRU но это нарушит его инкапсуляцию
        std::size_t Kin;
        std::size_t Kout;
        std::size_t capacity_;

        std::list<int> a1in;
        std::unordered_map<int, Iterator> a1in_table;

        std::list<int> a1out;
        std::unordered_map<int, Iterator> a1out_table;

        std::list<int> am;
        std::unordered_map<int, Iterator> am_table;

        void delet_hach_list_last(std::unordered_map<int, Iterator>& hach_table, std::list<int>& list);
        void insert_new(int key, std::unordered_map<int, Iterator>& hach_table, std::list<int>& list);

        void make_recent_am(Iterator position);// перенести существующий узел списка в head
                                
    public:
        Two_Q_Cach();
        explicit Two_Q_Cach(std::size_t capacity);

        Two_Q_Cach(const Two_Q_Cach&) = delete;
        Two_Q_Cach& operator=(const Two_Q_Cach&) = delete;

        bool access(int key);
        bool check_a1out(int key);

        std::size_t size() const noexcept;
        std::size_t capacity() const noexcept;
};

#endif