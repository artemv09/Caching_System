#ifndef LIRS_
#define LIRS_

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

#include "crutch.hpp"


template <typename Key, typename Value>
class Lirs_cach
{
    private:
        using Key_List = std::list<Value>;
        using Iterator = typename Key_List::iterator;

        std::size_t capacity_;
        std::size_t size_;

        std::size_t LIR_capacity_;
        std::size_t HIR_capacity_; 

        std::size_t LIR_count_ = 0;
        std::size_t HIR_resident_count_ = 0;

        enum class Status
        {
            LIR,
            HIR,
            HIR_NO_RES
        };

        struct Node//это должно храниться в хеш таблице
        {
            Status status;
            bool resident_flag;//если настоящий то true это флаг то что эллемент находится в кеше
            bool in_s_flag;
            bool in_q_flag;

            Iterator s_position;
            Iterator q_position;            
        };

        using Directory = std::unordered_map<Key, Node>;
        using Directory_Iterator = typename Directory::iterator;

        Key_List list_Q;//хранит только HIR resident
        Key_List list_S;

        Directory general_hash_table;

        void insert_new(const Key& key);
        void hit_hir_S_ell(Directory_Iterator hash_table_it);
        void move_to_top_S(Directory_Iterator hash_table_it);
        void erase_from_Q(Directory_Iterator hash_table_it);
        void hit_hir_Q_ell(Directory_Iterator hash_table_it);
        void hit_no_resident(Directory_Iterator hash_table_it);
        void prune_S();//берет и очищает низ LIR от HIR
    
    public:
        //новые функции
        Access_Result<Value> look_up(const Key& key);
        //конец
        explicit Lirs_cach(std::size_t capacity, std::size_t HIR_capacity);
        explicit Lirs_cach();

        Lirs_cach(const Lirs_cach&) = delete;
        Lirs_cach& operator=(const Lirs_cach&) = delete;

        bool access(const Key& key);

        std::size_t size() const noexcept
        {
            return list_Q.size() + LIR_count_;
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }
};

template <typename Key, typename Value>
Access_Result<Value> Lirs_cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return {false, nullptr};
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return {false, nullptr};
    }

    Node& node = found -> second;

    // Ключ есть только как история, самой страницы в кэше нет.
    if(node.status == Status::HIR_NO_RES || !node.resident_flag)
    {
        return {false, nullptr};
    }

    Value* value = node.value;

    if(capacity_ == 1)
    {
        return {true, value};
    }

    switch(node.status)
    {
        case(Status::LIR):
        {
            move_to_top_S(found);
            prune_S();

            return {true, value};
        }

        case(Status::HIR):
        {
            if(node.in_s_flag)
            {
                hit_hir_S_ell(found);
            }
            else
            {
                hit_hir_Q_ell(found);
            }

            return {true, value};
        }

        case(Status::HIR_NO_RES):
        {
            return {false, nullptr};
        }
    }

    return {false, nullptr};
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::insert_new(const Key& key)
{
    list_S.push_front(key);//новый эллемент всегда кладется в S

    if(LIR_capacity_ == LIR_count_)
    {
        if(HIR_capacity_ == HIR_resident_count_)//вытеснить HIR но оставить no_res...
        {
            Key key_hir_del = list_Q.back();
            auto it_hash = general_hash_table.find(key_hir_del);
            Node& node_hir_del = it_hash -> second;

            if(!node_hir_del.in_s_flag)
            {
                general_hash_table.erase(it_hash);
            } 

            node_hir_del.q_position = Iterator{};
            node_hir_del.status = Status::HIR_NO_RES;
            node_hir_del.resident_flag = false;
            node_hir_del.in_q_flag = false;
            
            HIR_resident_count_--;
            list_Q.pop_back();
        }

        list_Q.push_front(key);
        Node node{Status::HIR, true, true, true, list_S.begin(), list_Q.begin()};
        general_hash_table.emplace(key, node);
        HIR_resident_count_++;
    }
    else
    {
        Node node{Status::LIR, true, true, false, list_S.begin(), Iterator{}};
        general_hash_table.emplace(key, node);
        LIR_count_++;
    }
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::move_to_top_S(Directory_Iterator hash_table_it)
{   
    list_S.splice(list_S.begin(), list_S, hash_table_it -> second.s_position);
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::hit_hir_S_ell(Directory_Iterator hash_table_it)
{   
    Node& hit_hir_node = hash_table_it -> second; 

    auto demoted_it = general_hash_table.find(list_S.back());
    Node& demoted = demoted_it -> second;

    move_to_top_S(hash_table_it);

    list_Q.erase(hit_hir_node.q_position);
    hit_hir_node.in_q_flag = false;
    hit_hir_node.status = Status::LIR;

    list_Q.splice(list_Q.begin(), list_S, demoted.s_position);// Узел бывшей нижней LIR переносим из S в начало Q

    Node demoted_node{Status::HIR, true, false, true, Iterator{}, list_Q.begin()};
    demoted = demoted_node;

    prune_S();
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::erase_from_Q(Directory_Iterator hash_table_it)
{   
    auto& node = hash_table_it -> second;

    if (!node.in_q_flag) return;
    
    list_Q.erase(node.q_position);
    node.in_q_flag = false;
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::prune_S()
{
    while(!list_S.empty())
    {
        const Key& key = list_S.back();

        auto hash_it = general_hash_table.find(key);
        auto& node = hash_it -> second;

        if(node.status == Status::LIR) break;

        node.in_s_flag = false;

        if(node.resident_flag)
        {
            list_S.pop_back();
            continue;
        }

        general_hash_table.erase(hash_it);
        list_S.pop_back();
    }
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::hit_hir_Q_ell(Directory_Iterator hash_table_it)
{
    auto& node = hash_table_it -> second;

    list_S.push_front(*(node.q_position));
    node.s_position = list_S.begin();
    node.in_s_flag = true;

    list_Q.splice(list_Q.begin(), list_Q, node.q_position);
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::hit_no_resident(Directory_Iterator hash_table_it)
{
    assert(LIR_capacity_ == LIR_count_);
    Node& node_hit = hash_table_it -> second;

    move_to_top_S(hash_table_it);
    node_hit.status = Status::LIR;
    node_hit.resident_flag = true;
    
    if(HIR_capacity_ == HIR_resident_count_)
    {
        Directory_Iterator oldes_Q_ell_it = general_hash_table.find(list_Q.back());

        if(oldes_Q_ell_it -> second.in_s_flag)
        {
            oldes_Q_ell_it -> second.status = Status::HIR_NO_RES;
            oldes_Q_ell_it -> second.in_q_flag = false;
            oldes_Q_ell_it -> second.q_position = Iterator{};
            oldes_Q_ell_it -> second.resident_flag = false;
        }
        else
        {
            general_hash_table.erase(oldes_Q_ell_it);
        }
        list_Q.pop_back();  
        HIR_resident_count_--;
    }

    Directory_Iterator oldes_S_ell_it = general_hash_table.find(list_S.back());
    list_Q.splice(list_Q.begin(), list_S, oldes_S_ell_it -> second.s_position);

    Node copy_node{Status::HIR, true, false, true, Iterator{}, list_Q.begin()};
    oldes_S_ell_it -> second = copy_node;

    HIR_resident_count_++;

    prune_S();
}

template <typename Key, typename Value>
bool Lirs_cach<Key, Value>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found = general_hash_table.find(key);

    // При вместимости 1 разделение на LIR/HIR невозможно: храним одну HIR в Q.
    if(capacity_ == 1)
    {
        if(found != general_hash_table.end())
        {
            return true;
        }

        insert_new(key);
        prune_S(); // История не нужна: S остаётся пустым, страница остаётся в Q.
        return false;
    }

    if(found == general_hash_table.end())
    {
        insert_new(key);
        return false;
    }

    switch(found -> second.status)
    {
        case(Status::LIR):
        {
            move_to_top_S(found);
            prune_S();
            return true;
        }
        case(Status::HIR):
        {
            if(found -> second.in_s_flag)
            {
                hit_hir_S_ell(found);
            }   
            else
            {
                hit_hir_Q_ell(found);
            }
            return true;
        }
        case(Status::HIR_NO_RES):
        {
            hit_no_resident(found);
            return false;
        }
        default:
            break;
    }
    return false;
}

template <typename Key, typename Value>
Lirs_cach<Key, Value>::Lirs_cach(std::size_t capacity, std::size_t HIR_capacity): 
    capacity_(capacity),
    size_(0),
    LIR_capacity_(0),
    HIR_capacity_(HIR_capacity),
    LIR_count_(0),
    HIR_resident_count_(0)
{
    if(HIR_capacity > capacity || (capacity > 1 && (HIR_capacity == 0 || HIR_capacity == capacity)))
    {
        throw std::invalid_argument("LIRS: invalid HIR capacity");
    }

    if(capacity == 1)
    {
        HIR_capacity_ = 1;
    }
    LIR_capacity_ = capacity_ - HIR_capacity_;

    general_hash_table.reserve(capacity_ * 2);//зарезервировал чтобы меньше выделять по мере выполнения
}

template <typename Key, typename Value>
Lirs_cach<Key, Value>::Lirs_cach(): Lirs_cach(0, 0)
{
}


#endif
