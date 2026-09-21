#ifndef ARC_
#define ARC_

#include <optional>
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
class Arc_cach
{
    private:
        using Cach_List = std::list<Entry<Key, Value>>;
        using Cach_Iterator = typename Cach_List::iterator;

        using Ghost_List = std::list<Key>;
        using Ghost_Iterator = typename Ghost_List::iterator;

        enum class Type_list_save
        {
            T1,
            T2,
            B1,
            B2,
            None // Запрошенный ключ отсутствует во всех списках.
        };

        struct Node_hash
        {
            Type_list_save type_list;

            Cach_Iterator cach_position{};
            Ghost_Iterator ghost_position{};
        };

        using Directory = std::unordered_map<Key, Node_hash>;
        using Directory_Iterator = typename Directory::iterator;

        std::size_t capacity_;
        std::size_t target_recent_size_;

        // Начало списка — MRU, конец — LRU. B1/B2 хранят только историю.
        Cach_List T1_;
        Cach_List T2_;
        Ghost_List B1_;
        Ghost_List B2_;

        Directory general_hash_table;

        void insert_new(const Key& key, const Value& value);//создать новый эллемент и щапись в хэш таблице

        void move_begin_T2(Cach_Iterator position);//переместить в T2 в начало

        Erase_ELL<Key, Value> repeated_hit_transfer_T2(Directory_Iterator hash_iterator, const Value& value);//из B1 B2 T1 в T2

        Erase_ELL<Key, Value> delete_ell_list(Type_list_save type_list);// Переносит конец T в соответствующую историю или удаляет хвост B.

        Erase_ELL<Key, Value> move_resident_to_ghost(Directory_Iterator position);

        Erase_ELL<Key, Value> cache_one_ell_clean(Type_list_save request_in);

    public:
        Erase_ELL<Key, Value> find_del(const Key& key);

        Access_Result<Value> look_up(const Key& key);

        Erase_ELL<Key, Value> insert_value(const Key& key, const Value& value);

        bool erase_key(const Key& key);


        //конец
        explicit Arc_cach(std::size_t capacity);
        explicit Arc_cach();

        Arc_cach(const Arc_cach&) = delete;
        Arc_cach& operator=(const Arc_cach&) = delete;

        std::size_t size() const noexcept
        {
            return T1_.size() + T2_.size();
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }

        bool empty() const noexcept
        {
            return T1_.empty() && T2_.empty();
        }
};

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::find_del(const Key& key)
{
    auto found = general_hash_table.find(key);
    if(found == general_hash_table.end() ||
       (found->second.type_list != Type_list_save::T1 &&
        found->second.type_list != Type_list_save::T2))
    {
        return {};
    }

    auto erased = move_resident_to_ghost(found);
    // В отличие от REPLACE при ghost-hit, здесь нет последующей загрузки,
    // которая убрала бы запись из истории. Ограничиваем B1 + B2 отдельно.
    if(B1_.size() + B2_.size() > capacity_)
    {
        // T1 + B1 не увеличилось, поэтому при переполнении B2 непуст.
        delete_ell_list(Type_list_save::B2);
    }
    return erased;
}

template <typename Key, typename Value>
Access_Result<Value> Arc_cach<Key, Value>::look_up(const Key& key)
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

    auto& node = found -> second;

    switch(node.type_list)
    {
        case(Type_list_save::T1):
        {
            T2_.splice(T2_.begin(), T1_, node.cach_position);

            node.type_list = Type_list_save::T2;

            return {true, &(node.cach_position -> value)};
        }

        case(Type_list_save::T2):
        {
            T2_.splice(T2_.begin(), T2_, node.cach_position);

            return {true, &(node.cach_position -> value)};
        }
        default:
        {
            return {false, nullptr};
        }
    }
    return {false, nullptr};
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::insert_value(const Key& key, const Value& value)
{
   if(capacity_ == 0)
    {
        return {key, value};
    }

    auto found = general_hash_table.find(key);

    // Ключ уже известен ARC.
    if(found != general_hash_table.end())
    {
        return repeated_hit_transfer_T2(found, value);
    }

    auto erased = cache_one_ell_clean(Type_list_save::None);
    insert_new(key, value);

    return erased;
    //вызвать общую встваку но перед этим освободить место
}

template <typename Key, typename Value>
bool Arc_cach<Key, Value>::erase_key(const Key& key)
{
    {
    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        return false;
    }

    Node_hash& node = found -> second;

    switch(node.type_list)
    {
        case(Type_list_save::T1):
        {
            T1_.erase(node.cach_position);
            break;
        }
        case(Type_list_save::T2):
        {
            T2_.erase(node.cach_position);
            break;
        }
        case(Type_list_save::B1):
        case(Type_list_save::B2):
        case(Type_list_save::None):
        {
            return false;
        } //не удаляем из ghost записи
    }

    general_hash_table.erase(found);

    return true;
}
}

template <typename Key, typename Value>
Arc_cach<Key, Value>::Arc_cach(std::size_t capacity):capacity_(capacity), target_recent_size_(0)
{
}

template <typename Key, typename Value>
Arc_cach<Key, Value>::Arc_cach(): Arc_cach(0)
{
}

template <typename Key, typename Value>
void Arc_cach<Key, Value>::insert_new(const Key& key, const Value& value)//сохдаем новые эллемент в T1 и запись в хеш таблице
{
    T1_.push_front(Entry<Key, Value>{key, value});

    try
    {
        Node_hash node_hash{Type_list_save::T1, T1_.begin(), Ghost_Iterator{}};

        if(!general_hash_table.emplace(key, node_hash).second)
        {
            throw std::logic_error("ARC: duplicate key");
        }
    }
    catch(...)//надо ловить любое исключение
    {
        T1_.pop_front();
        throw;
    }
}

template <typename Key, typename Value>
void Arc_cach<Key, Value>::move_begin_T2(Cach_Iterator position)
{
    T2_.splice(T2_.begin(), T2_, position);
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::repeated_hit_transfer_T2(Directory_Iterator hash_iterator, const Value& value)//переместить эллемент в T2 ЭТО ГЛАВАНАЯ ФУНКЦИЯ
{
    auto& node = hash_iterator -> second;
    Erase_ELL<Key, Value> erased;

    switch(node.type_list)// из какого list мы вызываем
    {
        case(Type_list_save::T1):
        {
            T2_.splice(T2_.begin(), T1_, node.cach_position);
            erased = {};
            break;
        }
        case(Type_list_save::B1):
        {
            const auto delta = std::max(std::size_t{1}, B2_.size() / B1_.size());//TODO можно поменять на вариант из книги
            target_recent_size_ += std::min(delta, capacity_ - target_recent_size_);

            erased = cache_one_ell_clean(Type_list_save::B1);

            Key key = hash_iterator -> first;

            T2_.push_front(Entry<Key, Value>{key, value});
            Cach_Iterator new_position = T2_.begin();

            B1_.erase(node.ghost_position);

            node.cach_position = new_position;
            break;
        }
        case(Type_list_save::B2):
        {
            const auto delta = std::max(std::size_t{1}, B1_.size() / B2_.size());
            target_recent_size_ -= std::min(delta, target_recent_size_);

           erased = cache_one_ell_clean(Type_list_save::B2);

            Key key = hash_iterator -> first;

            T2_.push_front(Entry<Key, Value>{key, value});
            Cach_Iterator new_position = T2_.begin();

            B2_.erase(node.ghost_position);

            node.cach_position = new_position; 
            break;
        }
        case(Type_list_save::T2):
        {
            move_begin_T2(node.cach_position);
            erased = {};
            break;
        }
        case(Type_list_save::None):
        {
            std::cerr << "в хэш таблице несуществующий эллемент";
            assert(false);
            erased = {};
            break;
        }
    }

    node.type_list = Type_list_save::T2;
    return erased;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::move_resident_to_ghost(Directory_Iterator position)
{
    assert(position != general_hash_table.end());
    Node_hash& node = position->second;
    assert(node.type_list == Type_list_save::T1 || node.type_list == Type_list_save::T2);

    const bool recent = node.type_list == Type_list_save::T1;
    auto& resident = recent ? T1_ : T2_;
    auto& ghost = recent ? B1_ : B2_;
    Erase_ELL<Key, Value> erased{position->first, node.cach_position->value};

    // Копирование пары и создание ghost-узла выполняются до удаления Entry.
    ghost.push_front(*erased.key_erase);
    resident.erase(node.cach_position);
    node.cach_position = Cach_Iterator{};
    node.ghost_position = ghost.begin();
    node.type_list = recent ? Type_list_save::B1 : Type_list_save::B2;
    return erased;
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::delete_ell_list(Type_list_save type_list)//понижение одного конкретного эллемента до B1 или B2 передаем что нужно понизить T1 T2 поницаем B1 B2 удаляем
{
    switch(type_list)
    {
        case(Type_list_save::T1):
        {
            return move_resident_to_ghost(general_hash_table.find(T1_.back().key));
        }
        case(Type_list_save::T2):
        {
            return move_resident_to_ghost(general_hash_table.find(T2_.back().key));
        }

        case(Type_list_save::B1):
        {
            Key key = B1_.back();

            auto found = general_hash_table.find(key);

            B1_.erase(found -> second.ghost_position);
            general_hash_table.erase(found);

            return {};
        }

        case(Type_list_save::B2):
        {
            Key key = B2_.back();

            auto found = general_hash_table.find(key);

            B2_.erase(found -> second.ghost_position);
            general_hash_table.erase(found);

            return {};
        }

        case(Type_list_save::None):
        {
            return {};
        }
    }

    return {};
}

template <typename Key, typename Value>
Erase_ELL<Key, Value> Arc_cach<Key, Value>::cache_one_ell_clean(Type_list_save request_in)//отвечает за выбор логики удаления и перемещения эллементов
{
    if(capacity_ == 0)
    {        
        return {};
    }

    if(request_in == Type_list_save::None)
    {
        if(T1_.size() + B1_.size() == capacity_)
        {
            if(T1_.size() == capacity_)
            {
                auto oldest_ell = general_hash_table.find(T1_.back().key);//полностью удаляем из T1 без захода в B1
                Erase_ELL<Key, Value> erased{oldest_ell->first, T1_.back().value};

                general_hash_table.erase(oldest_ell);
                T1_.pop_back();

                return erased;
            }

            delete_ell_list(Type_list_save::B1);
        }
        else if(size() == capacity_ && B1_.size() + B2_.size() == capacity_)
        {
            delete_ell_list(Type_list_save::B2);
        }

    }

    if(size() < capacity_)
    {
        return {};
    }

    if((!T1_.empty() && T1_.size() > target_recent_size_ )|| (request_in == Type_list_save::B2 && T1_.size() == target_recent_size_))
    {
        return delete_ell_list(Type_list_save::T1);
    }
    else
    {
        assert(!T2_.empty());
        return delete_ell_list(Type_list_save::T2);
    }
}

#endif
