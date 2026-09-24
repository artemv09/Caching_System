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
#include <optional>

#include "cach_type.hpp"

template <typename Key, typename Value>
class Lirs_cach
{
    private:
        using Key_List = std::list<Key>;
        using Key_Iterator = typename Key_List::iterator;

        using Cach_List = std::list<Entry<Key, Value>>;
        using Cach_Iterator = typename Cach_List::iterator;

        std::size_t capacity_;

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
    
            Cach_Iterator resident_position;
            Key_Iterator s_position;
            Key_Iterator q_position;            
        };

        using Directory = std::unordered_map<Key, Node>;
        using Directory_Iterator = typename Directory::iterator;

        Key_List list_Q; //хранит только ключи HIR resident
        Key_List list_S;

        Cach_List resident; // храняться resident данные ключ + значение

        Directory general_hash_table;

        Erase_ELL<Key, Value> insert_new(const Key& key, const Value& value); // создать подностью новый эллемент

        void move_to_top_S(Directory_Iterator hash_table_it);

        //void erase_from_Q(Directory_Iterator hash_table_it);

        void hit_hir_S_ell(Directory_Iterator hash_table_it); 

        void hit_hir_Q_ell(Directory_Iterator hash_table_it);

        Erase_ELL<Key, Value> hit_no_resident(Directory_Iterator hash_table_it, const Value& value); // опападние в NON_RES

        void prune_S(); //берет и очищает низ LIR от HIR
    
    public:
        Erase_ELL<Key, Value> extract_entry(const Key& key);

        Value* look_up(const Key& key);

        bool erase_key(const Key& key);

        Erase_ELL<Key, Value> insert_value(const Key& key, const Value& value);
  
        explicit Lirs_cach(std::size_t capacity, std::size_t HIR_capacity);

        Lirs_cach(const Lirs_cach&) = delete;
        Lirs_cach& operator=(const Lirs_cach&) = delete;

        std::size_t size() const noexcept
        {
            assert(list_Q.size() + LIR_count_ == resident.size());
            return list_Q.size() + LIR_count_;
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }
};

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lirs_cach<Key, Value>::extract_entry(const Key& key)
{
    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return std::nullopt;
    }

    Node& node = found -> second;

    if(node.status == Status::HIR_NO_RES)
    {
        return std::nullopt;
    }

    Erase_ELL<Key, Value> erased_ell = Entry<Key, Value>{found -> first, (node.resident_position) -> value};
    erase_key(erased_ell -> key); // удаляем без занесения в ghost

    return erased_ell;
}

template <typename Key, typename Value>
Value* Lirs_cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return nullptr;
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return nullptr;
    }

    Node& node = found -> second;

    if(node.status == Status::HIR_NO_RES) // Ключ есть только как история, самой страницы в кэше нет.
    {
        return nullptr;
    }

    Value* addr_value = &((node.resident_position) -> value);

    if(capacity_ == 1)
    {
        return addr_value;
    }

    switch(node.status)
    {
        case(Status::LIR):
        {
            move_to_top_S(found);
            prune_S();

            return addr_value;
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

            return addr_value;
        }

        case(Status::HIR_NO_RES):
        {
            return nullptr;
        }
    }

    return nullptr;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lirs_cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
    if(capacity_ == 0)
    {
        return Entry<Key, Value>{key, value};
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return insert_new(key, value);
    }

    Node& node = found -> second;

    if(node.status == Status::HIR_NO_RES)
    {
        return hit_no_resident(found, value);
    }

    return std::nullopt;
}

template <typename Key, typename Value>
bool Lirs_cach<Key, Value>::erase_key(const Key& key)
{
    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return false;
    }

    Node& node = found -> second;

    if(node.status == Status::HIR_NO_RES)
    {
        return false;
    }

    if(node.status == Status::HIR)
    {
        list_Q.erase(node.q_position);

        HIR_resident_count_--;

       resident.erase(node.resident_position); // точно удаляем из резидентов
       node.resident_position = Cach_Iterator{};

        if(node.in_s_flag)
        {
            node.status = Status::HIR_NO_RES;
            node.resident_flag = false;
            node.in_q_flag = false;
            node.q_position = Key_Iterator{};
        }
        else
        {
            general_hash_table.erase(found);
        }

        return true;
    }

    resident.erase(node.resident_position);

    node.status = Status::HIR_NO_RES;
    node.resident_flag = false;
    node.in_q_flag = false;
    node.q_position = Key_Iterator{};
    node.resident_position = Cach_Iterator{};

    LIR_count_--;

    prune_S();

    return true;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lirs_cach<Key, Value>::insert_new(const Key& key, const Value& value)
{
    Erase_ELL<Key, Value> erased;

    list_S.push_front(key); //новый эллемент всегда кладется в S
    resident.push_front(Entry<Key, Value>{key, value});

    if(LIR_capacity_ == LIR_count_)
    {
        if(HIR_capacity_ == HIR_resident_count_) //вытеснить HIR но оставить no_res...
        {
            auto oldest_ell_it = general_hash_table.find(list_Q.back());
            Node& node_hir_del = oldest_ell_it -> second;

            erased = Entry<Key, Value>{oldest_ell_it -> first, node_hir_del.resident_position -> value};

            list_Q.pop_back();
            resident.erase(node_hir_del.resident_position); 

            HIR_resident_count_--;

            if(node_hir_del.in_s_flag)
            {
                node_hir_del.q_position = Key_Iterator{};
                node_hir_del.resident_position = Cach_Iterator{};
                node_hir_del.status = Status::HIR_NO_RES;
                node_hir_del.resident_flag = false;
                node_hir_del.in_q_flag = false;
            }
            else
            {
                general_hash_table.erase(oldest_ell_it);
            }
        }

        list_Q.push_front(key);
        Node node{Status::HIR, true, true, true, resident.begin(), list_S.begin(), list_Q.begin()};

        general_hash_table.emplace(key, node);
        HIR_resident_count_++;

        if(capacity_ == 1)
        {
            prune_S();
        }

        return erased;
    }
    else
    {
        Node node{Status::LIR, true, true, false, resident.begin(), list_S.begin(), Key_Iterator{}};
        general_hash_table.emplace(key, node);
        LIR_count_++;

        prune_S();

        return std::nullopt;
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

    hit_hir_node.q_position = Key_Iterator{};
    hit_hir_node.in_q_flag = false;
    hit_hir_node.status = Status::LIR;

    if(LIR_count_ < LIR_capacity_)
    {
        LIR_count_++;
        HIR_resident_count_--;
        return;
    }

    //не может быть такого что мы попали в HIR из LIR и LIR не заполнен
    list_Q.splice(list_Q.begin(), list_S, demoted.s_position);// Узел бывшей нижней LIR переносим из S в начало Q

    demoted.status = Status::HIR; //понижаем один из LIR
    demoted.in_s_flag = false;
    demoted.s_position = Key_Iterator{};
    demoted.in_q_flag = true;
    demoted.q_position = list_Q.begin();

    prune_S();
}

template <typename Key, typename Value>
void Lirs_cach<Key, Value>::prune_S()
{
    while(!list_S.empty())
    {
        const Key& key = list_S.back();

        auto hash_it = general_hash_table.find(key);
        auto& node = hash_it -> second;

        if(node.status == Status::LIR) 
        {
            break;
        }

        node.in_s_flag = false;
        node.s_position = Key_Iterator{};

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

    list_S.push_front(hash_table_it -> first);
    node.s_position = list_S.begin();
    node.in_s_flag = true;

    list_Q.splice(list_Q.begin(), list_Q, node.q_position);
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Lirs_cach<Key, Value>::hit_no_resident(Directory_Iterator hash_table_it, const Value& value)
{
    Node& node_hit = hash_table_it -> second;

    Erase_ELL<Key, Value> erased = std::nullopt;

    move_to_top_S(hash_table_it);

    resident.push_front(Entry<Key, Value>{hash_table_it -> first, value});

    node_hit.status = Status::LIR;
    node_hit.resident_flag = true;
    node_hit.resident_position = resident.begin();
    
    if(HIR_capacity_ == HIR_resident_count_ && LIR_count_ == LIR_capacity_)
    {
        Directory_Iterator oldest_Q_it = general_hash_table.find(list_Q.back());
        Node& node_old_Q = oldest_Q_it -> second; 

        erased = Entry<Key, Value>{oldest_Q_it -> first, (node_old_Q.resident_position) -> value};

        list_Q.pop_back();  
        resident.erase(node_old_Q.resident_position);

        HIR_resident_count_--;

        if(node_old_Q.in_s_flag)
        {
            node_old_Q.status = Status::HIR_NO_RES;
            node_old_Q.in_q_flag = false;
            node_old_Q.q_position = Key_Iterator{};
            node_old_Q.resident_flag = false;
            node_old_Q.resident_position = Cach_Iterator{};
        }
        else
        {
            general_hash_table.erase(oldest_Q_it);
        }
    }

    if(LIR_count_ < LIR_capacity_)
    {
        LIR_count_++;
    }
    else
    {
        Directory_Iterator oldes_S_ell_it = general_hash_table.find(list_S.back());
        Node& node_oldes_S = oldes_S_ell_it -> second;

        list_Q.splice(list_Q.begin(), list_S, node_oldes_S.s_position);

        node_oldes_S.status = Status::HIR;
        node_oldes_S.in_s_flag = false;
        node_oldes_S.s_position = Key_Iterator{};
        node_oldes_S.in_q_flag = true;
        node_oldes_S.q_position = list_Q.begin();

        HIR_resident_count_++;
    }

    prune_S();
    return erased;
}

template <typename Key, typename Value>
Lirs_cach<Key, Value>::Lirs_cach(std::size_t capacity, std::size_t HIR_capacity): 
    capacity_(capacity),
    LIR_capacity_(0),
    HIR_capacity_(HIR_capacity),
    LIR_count_(0),
    HIR_resident_count_(0)
{
    if(HIR_capacity > capacity || (capacity > 1 && (HIR_capacity == 0 || HIR_capacity == capacity)))
    {
        throw std::invalid_argument("Нельзя делать HIR = 1 при capciy = 1");
    }

    if(capacity == 1)
    {
        HIR_capacity_ = 1;
    }

    LIR_capacity_ = capacity_ - HIR_capacity_;

    general_hash_table.reserve(capacity_ * 2);
}


#endif
