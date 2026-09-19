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
#include "crutch.hpp"

//TODO - когда будем добалять шаблоны испольщовать using для карты и списков
template <typename Key, typename Value>
class Two_Q_Cach
{
    private://в принципе я могу использовать интерфейс LRU но это нарушит его инкапсуляци
        using Key_List = std::list<Value>;
        using Iterator = typename Key_List::iterator;

        using Directory = std::unordered_map<Key, Iterator>;
        using Directory_Iterator = typename Directory::iterator;

        std::size_t capacity_;
        std::size_t Kout;
        std::size_t Kin;
        //TODO наверное будет разумнее при перестройке сделать вместо 3 хеш таблиц одну с enum как в ARC
        Key_List a1in;
        Directory a1in_table;

        Key_List a1out;
        Directory a1out_table;

        Key_List am;
        Directory am_table;

        void delet_hach_list_last(Directory& hach_table, Key_List& list);
        void insert_new(const Key& key, Directory& hach_table, Key_List& list);

        void make_recent_am(Iterator position);// перенести существующий узел списка в head
        void rules_displacment();
                           
    public:
        //новые функции
        Access_Result<Value> look_up(const Key& key):

        //конец
        explicit Two_Q_Cach();
        explicit Two_Q_Cach(std::size_t capacity);

        Two_Q_Cach(const Two_Q_Cach&) = delete;
        Two_Q_Cach& operator=(const Two_Q_Cach&) = delete;

        bool access(const Key& key);
        bool check(const Key& key, Directory& hach_table);

        std::size_t size() const noexcept
        {
            return a1in.size() + am.size();
        }

        std::size_t capacity() const noexcept
        {
            return  capacity_;
        }
};

template <typename Key, typename Value>
Access_Result<Value> Two_Q_Cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return {false, nullptr};
    }

    auto found_am = am_table.find(key);

    if(found_am != am_table.end())
    {
        make_recent_am(found_am -> second);

        return {true, &(*found_am -> second)};
    }

    auto found_a1in = a1in_table.find(key);

    if(found_a1in != a1in_table.end())
    {
        return {true, &(*found_a1in -> second)};
    }

    return {false, nullptr};
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::delet_hach_list_last(Directory& hach_table, Key_List& list)
{
    int key = list.back();
    auto it = hach_table.find(key);

    if(it == hach_table.end())
    {
        return;
    }
    list.erase(it -> second);
    hach_table.erase(it);
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::make_recent_am(Iterator position)// перенести существующий узел списка в head
{
    am.splice(am.begin(), am, position);
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::insert_new(const Key& key, Directory& hach_table, Key_List& list)// добавить новый узел списка и соответствующую запись в хеш-таблицу
{
    list.push_front(key);
    hach_table.emplace(key, list.begin());
}

template <typename Key, typename Value>
Two_Q_Cach<Key, Value>::Two_Q_Cach(): capacity_(0)
{
}

template <typename Key, typename Value>
Two_Q_Cach<Key, Value>::Two_Q_Cach(std::size_t capacity): capacity_(capacity), Kout(capacity / 2), Kin(capacity / 4)
{
}

template <typename Key, typename Value>
bool Two_Q_Cach<Key, Value>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found_ell = a1out_table.find(key);
    auto found_am_ell = am_table.find(key);

    if(check(key, a1out_table))
    {
        a1out.erase(found_ell -> second);
        a1out_table.erase(found_ell);

        rules_displacment();
        insert_new(key, am_table, am);       
    }

    else if(check(key, am_table))
    {
        make_recent_am(found_am_ell -> second);
        return true;
    }

    else if(check(key, a1in_table))
    {
        return true;
    }

    else
    {
        rules_displacment();
        insert_new(key, a1in_table, a1in);
    }

    return false;
}

template <typename Key, typename Value>
bool Two_Q_Cach<Key, Value>::check(const Key& key, Directory& hach_table)//проверка наличия ключа в таблице
{
    return hach_table.find(key) != hach_table.end();
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::rules_displacment()//правила для выброса эллемента из am и a1in
{
    if(size() == capacity_)
    {
        if(a1in.size() > Kin)
        {
            int evicted_key = a1in.back();
            delet_hach_list_last(a1in_table, a1in);
            insert_new(evicted_key, a1out_table, a1out);
        }
        else
        {
            delet_hach_list_last(am_table, am);
        }
    }

    if(a1out.size() > Kout)
    {
        delet_hach_list_last(a1out_table, a1out);
    }
}

#endif