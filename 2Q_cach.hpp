#ifndef Q_CACH_
#define Q_CACH_

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <algorithm>
#include <new>
#include <memory>

#include "cach_type.hpp"


template <typename Key, typename Value>
class Two_Q_Cach
{
    private://в принципе я могу использовать интерфейс LRU но это нарушит его инкапсуляци
        using Cach_List = std::list<Entry<Key, Value>>;
        using Cach_Iterator = typename Cach_List::iterator;

        using Ghost_List = std::list<Key>;
        using Ghost_Iterator = typename Ghost_List::iterator;

        enum class Type_list
        {
            A1in,
            Am,
            A1out
        };

        struct Node
        {
            Type_list type;

            Cach_Iterator cach_position{};
            Ghost_Iterator ghost_position{};
        };

        using Directory = std::unordered_map<Key, Node>;
        using Directory_Iterator = typename std::unordered_map<Key, Node>::iterator;
        

        std::size_t capacity_;
        std::size_t Kout;
        std::size_t Kin;
        
        Cach_List a1in;
        Cach_List am;
        Ghost_List a1out;

        Directory hash_table;

        void make_recent_am(Cach_Iterator position);// перенести существующий узел списка в head
        
        Erase_ELL<Key, Value> rules_displacment(); // правила для выброса эллемента из am и a1in

        void insert_cach(const Key& key, const Value& value, Cach_List& list, Type_list type);

        void move_a1out_to_am(Directory_Iterator position_direct, const Value& value);
        Erase_ELL<Key, Value> move_a1in_to_a1out(Directory_Iterator position_direct);//

        void erase_a1out();//удаляет последний эллемнт из a1out
        Erase_ELL<Key, Value> erase_cach_old(Cach_List& list);//удаляет последний эллемент из am a1in
                           
    public:
        Value* look_up(const Key& key);

        Erase_ELL<Key, Value> insert_value(const Key& key, const Value& value);

        bool erase_key(const Key& key);

        Erase_ELL<Key, Value> extract_entry(const Key& key);
        
        explicit Two_Q_Cach(std::size_t capacity);

        Two_Q_Cach(const Two_Q_Cach&) = delete;
        Two_Q_Cach& operator=(const Two_Q_Cach&) = delete;

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
Erase_ELL<Key, Value> Two_Q_Cach<Key, Value>::extract_entry(const Key& key)
{
    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return std::nullopt;
    }

    Node& node = found -> second;

    switch(node.type)
    {
        case(Type_list::A1in):
        {
            auto erased = move_a1in_to_a1out(found);

            if(a1out.size() > Kout)
            {
                erase_a1out();
            }

            return erased;
        }

        case(Type_list::Am):
        {
            Erase_ELL<Key, Value> erased{found -> first, (node.cach_position) -> value};
            erase_key(erased -> key);
            return erased;
        }

        case(Type_list::A1out):
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

template <typename Key, typename Value>
Value* Two_Q_Cach<Key, Value>::look_up(const Key& key)
{
    if(capacity_ == 0)
    {
        return nullptr;
    }

    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return nullptr;
    }

    Node& node = found -> second;

    switch(node.type)
    {
        case(Type_list::A1in):
        {
            return &(node.cach_position -> value);
        }
        case(Type_list::Am):
        {
            make_recent_am(node.cach_position);

            return &((node.cach_position) -> value);
        }
        case(Type_list::A1out):
        {
            return nullptr;
        }
    }
    return nullptr;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Two_Q_Cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
    if(capacity_ == 0)
    {
        return Entry<Key, Value>{key, value};
    }

    auto found = hash_table.find(key);

    if(found != hash_table.end())
    {
        Node& node = found -> second;

        if(node.type == Type_list::A1out)
        {
            Erase_ELL<Key, Value> erased = rules_displacment();

            move_a1out_to_am(found, value);

            if(a1out.size() > Kout)
            {
                erase_a1out();
            }

            return erased;
        }

        return std::nullopt;
    }

    Erase_ELL<Key, Value> erased = rules_displacment();

    insert_cach(key, value, a1in, Type_list::A1in);

    if(a1out.size() > Kout)
    {
        erase_a1out();
    }

    return erased;
}

template <typename Key, typename Value>
bool Two_Q_Cach<Key, Value>::erase_key(const Key& key)
{
    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return false;
    }

    Node& node = found -> second;

    switch(node.type)
    {
        case Type_list::A1in:
        {
            a1in.erase(node.cach_position);
            break;
        }
        case Type_list::Am:
        {
            am.erase(node.cach_position);
            break;
        }
        case Type_list::A1out:
        {
            return false;
        }//не удаляем данные из ghost списка
    }

    hash_table.erase(found);

    return true;
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::insert_cach(const Key& key, const Value& value, Cach_List& list, Type_list type)
{
    list.push_front(Entry<Key, Value>{key, value});
    Cach_Iterator cach_position_beg = list.begin();

    try
    {
        if(type == Type_list::Am)
        {
            hash_table.emplace(key, Node{Type_list::Am, cach_position_beg,});
        }
        else
        {
            hash_table.emplace(key, Node{Type_list::A1in, cach_position_beg,});
        }
    }
    catch(...)
    {
        list.erase(cach_position_beg);
        throw;
    }
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::move_a1out_to_am(Directory_Iterator position_direct, const Value& value)
{
    Node& node = position_direct -> second;

    am.push_front(Entry<Key, Value>{position_direct -> first, value});

    Cach_Iterator cach_position_beg = am.begin();

    a1out.erase(node.ghost_position);

    node.ghost_position = Ghost_Iterator{};
    node.cach_position = cach_position_beg;
    
    node.type = Type_list::Am;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Two_Q_Cach<Key, Value>::move_a1in_to_a1out(Directory_Iterator position_direct)
{
    Node& node = position_direct -> second;

    Erase_ELL<Key, Value> erased = Entry<Key, Value>{position_direct -> first, node.cach_position -> value};
    a1out.push_front(erased -> key);

    Ghost_Iterator ghost_position_beg = a1out.begin();

    a1in.erase(node.cach_position);

    node.type = Type_list::A1out;

    node.cach_position = Cach_Iterator{};
    node.ghost_position = ghost_position_beg;
    
    return erased;
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::erase_a1out()
{
    Key key = a1out.back();
    auto it_hash = hash_table.find(key);
    Node& node = it_hash -> second;

    a1out.erase(node.ghost_position);
    hash_table.erase(it_hash);
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Two_Q_Cach<Key, Value>::erase_cach_old(Cach_List& list)
{
    Erase_ELL<Key, Value> erased = Entry<Key, Value>{list.back().key, list.back().value};
    auto it_hash = hash_table.find(erased -> key);

    Node& node = it_hash -> second;

    list.erase(node.cach_position);
    hash_table.erase(it_hash);

    return erased;
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::make_recent_am(Cach_Iterator position)// перенести существующий узел списка в head
{
    am.splice(am.begin(), am, position);
}

template <typename Key, typename Value>
Two_Q_Cach<Key, Value>::Two_Q_Cach(std::size_t capacity): capacity_(capacity), Kout(capacity / 2), Kin(capacity / 4)
{
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Two_Q_Cach<Key, Value>::rules_displacment()
{
    Erase_ELL<Key, Value> erased;

    if(size() == capacity_)
    {
        if(a1in.size() > Kin)
        {
            Key key = a1in.back().key;
            auto found = hash_table.find(key);
            erased = move_a1in_to_a1out(found);
        }
        else
        {
            erased = erase_cach(am);
        }
    }

    return erased;
}

#endif
