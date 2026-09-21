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
#include <new>
#include <memory>

#include "lfu_cach.hpp"
#include "crutch.hpp"

//TODO - когда будем добалять шаблоны испольщовать using для карты и списков
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

            union
            {
                Cach_Iterator cach_position{};
                Ghost_Iterator ghost_position{};
            };
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

       // void insert_new(const Key& key, Directory& hash_table, Cach_List& list);

        void make_recent_am(Cach_Iterator position);// перенести существующий узел списка в head
        std::optional<Key> rules_displacment();
                           
    public:
        //новые функции
        Access_Result<Value> look_up(const Key& key);
        std::optional<Key> insert_value(const Key& key, const Value& value);
        bool erase_key(const Key& key);
        void insert_cach(const Key& key, const Value& value, Cach_List& list, Type_list type);
        void move_a1out_to_am(Directory_Iterator position_direct, const Value& value);
        Key move_a1in_to_a1out(Directory_Iterator position_direct);//
        void erase_a1out();//удаляет последний эллемнт из a1out
        Key erase_cach(Cach_List& list);//удаляет последний эллемент из am a1in
        
        //конец
        explicit Two_Q_Cach();
        explicit Two_Q_Cach(std::size_t capacity);

        Two_Q_Cach(const Two_Q_Cach&) = delete;
        Two_Q_Cach& operator=(const Two_Q_Cach&) = delete;

        //bool access(const Key& key);
        bool check(const Key& key, Directory& hash_table);

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

    auto found = hash_table.find(key);

    if(found == hash_table.end())
    {
        return {false, nullptr};
    }

    Node& node = found -> second;

    switch(node.type)
    {
        case(Type_list::A1in):
        {
            return {true, &(node.cach_position -> value)};
        }
        case(Type_list::Am):
        {
            make_recent_am(node.cach_position);

            return {true, &((node.cach_position) -> value)};
        }
        // case(Type_list::A1out):
        // {
        //     return {false, nullptr};
        // }
    }

    return {false, nullptr};
}

template <typename Key, typename Value>
std::optional<Key> Two_Q_Cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
    if(capacity_ == 0)
    {
        return key;
    }

    auto found = hash_table.find(key);

    // Ключ уже известен 2Q.
    if(found != hash_table.end())
    {
        Node& node = found -> second;

        if(node.type == Type_list::A1out)
        {
            std::optional<Key> evicted_key = rules_displacment();

            move_a1out_to_am(found, value);

            return evicted_key;
        }
    }

    // Совершенно новый ключ.
    std::optional<Key> evicted_key = rules_displacment();
    insert_cach(key, value, a1in, Type_list::a1in);

    return evicted_key;
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
            a1in.erase(node.cache_position);
            break;
        }
        case Type_list::Am:
        {
            am.erase(node.cache_position);
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
    Cach_Iterator cache_position = list.begin();

    try
    {
        if(type == Type_list::Am)
        {
            hash_table.emplace(key, Node{Type_list::Am, cache_position,});
        }
        else
        {
            hash_table.emplace(key, Node{Type_list::A1in, cach_position,});
        }
    }
    catch(...)
    {
        list.erase(cach_position);
        throw;
    }
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::move_a1out_to_am(Directory_Iterator position_direct, const Value& value)
{
    Node& node = position_direct -> second;

    am.push_front(Entry<Key, Value>{position_direct -> first, value});

    Cach_Iterator cache_position = am.begin();

    a1out.erase(node.ghost_position);

    std::destroy_at(&node.ghost_position);// замена аргументов в union
    new (&node.cache_position) Cach_Iterator(cache_position);
    
    node.type = Type_list::Am;
}

template <typename Key, typename Value>
Key Two_Q_Cach<Key, Value>::move_a1in_to_a1out(Directory_Iterator position_direct)
{
    Node& node = position_direct -> second;

    Key key_out = position_direct -> first;
    a1out.push_front(key_out);

    Ghost_Iterator ghost_position = a1out.begin();

    a1in.erase(node.cach_position);

    node.type = Type_list::A1out;

    std::destroy_at(&node.cach_position);// замена аргументов в union
    new (&node.ghoyst_position) Cach_Iterator(ghost_position);  
    
    return key_out;
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::erase_a1out()
{
    Key key = a1out.back();
    auto it_hash = hash_table.find(key);
    Node& node = it_hash-> second;

    a1out.erase(node.ghost_position);
    hash_table.erase(it_hash);
}

template <typename Key, typename Value>
Key Two_Q_Cach<Key, Value>::erase_cach(Cach_List& list)
{
    Key key = list.back();
    auto it_hash = hash_table.find(key);
    Node& node = it_hash -> second;

    list.erase(node.cach_position);
    hash_table.erase(it_hash);

    return key;
}

template <typename Key, typename Value>
void Two_Q_Cach<Key, Value>::make_recent_am(Cach_Iterator position)// перенести существующий узел списка в head
{
    am.splice(am.begin(), am, position);
}

// template <typename Key, typename Value>
// void Two_Q_Cach<Key, Value>::insert_new(const Key& key, Cach_Directory& hach_table, Cach_List& list)// добавить новый узел списка и соответствующую запись в хеш-таблицу
// {
//     list.push_front(key);
//     hach_table.emplace(key, list.begin());
// }

template <typename Key, typename Value>
Two_Q_Cach<Key, Value>::Two_Q_Cach(): capacity_(0)
{
}

template <typename Key, typename Value>
Two_Q_Cach<Key, Value>::Two_Q_Cach(std::size_t capacity): capacity_(capacity), Kout(capacity / 2), Kin(capacity / 4)
{
}

// template <typename Key, typename Value>
// bool Two_Q_Cach<Key, Value>::access(const Key& key)
// {
//     if(capacity_ == 0)
//     {
//         return false;
//     }

//     auto found_ell = a1out_table.find(key);
//     auto found_am_ell = am_table.find(key);

//     if(check(key, a1out_table))
//     {
//         a1out.erase(found_ell -> second);
//         a1out_table.erase(found_ell);

//         rules_displacment();
//         insert_new(key, am_table, am);       
//     }

//     else if(check(key, am_table))
//     {
//         make_recent_am(found_am_ell -> second);
//         return true;
//     }

//     else if(check(key, a1in_table))
//     {
//         return true;
//     }

//     else
//     {
//         rules_displacment();
//         insert_new(key, a1in_table, a1in);
//     }

//     return false;
// }

template <typename Key, typename Value>
bool Two_Q_Cach<Key, Value>::check(const Key& key, Directory& hach_table)//проверка наличия ключа в таблице
{
    return hach_table.find(key) != hach_table.end();
}

template <typename Key, typename Value>
std::optional<Key> Two_Q_Cach<Key, Value>::rules_displacment()//правила для выброса эллемента из am и a1in
{
    std::optional<Key> evicted_key = std::nullopt;

    if(size() == capacity_)
    {
        if(a1in.size() > Kin)
        {
            Key key = a1in.back().key;
            auto found = hash_table.find(key);
            evicted_key = move_a1in_to_a1out(found);
        }
        else
        {
            evicted_key = erase_cach(am);
        }
    }

    if(a1out.size() > Kout)
    {
        erase_a1out();
    }

    return evicted_key;
}

#endif