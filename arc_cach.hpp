#ifndef ARC_
#define ARC_

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

template <typename Key>
class Arc_cach
{
    private:
        using Key_List = std::list<Key>;
        using Iterator = typename Key_List::iterator;

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
            Iterator position;
        };

        using Directory = std::unordered_map<Key, Node_hash>;
        using Directory_Iterator = typename Directory::iterator;

        std::size_t capacity_;
        std::size_t target_recent_size_;

        // Начало списка — MRU, конец — LRU. B1/B2 хранят только историю.
        Key_List T1_;
        Key_List T2_;
        Key_List B1_;
        Key_List B2_;

        Directory general_hash_table;

        void insert_new(const Key& key);//создать новый эллемент и щапись в хэш таблице
        void move_begin_T2(Iterator position);//переместить в T2 в начало
        void repeated_hit_transfer_T2(Directory_Iterator hash_iterator);//из B1 B2 T1 в T2

        void delete_ell_list(Key_List& list_out);// Переносит конец T в соответствующую историю или удаляет хвост B.

        void cache_one_ell_clean(Type_list_save request_in);

    public:
        explicit Arc_cach(std::size_t capacity);
        explicit Arc_cach();

        Arc_cach(const Arc_cach&) = delete;
        Arc_cach& operator=(const Arc_cach&) = delete;

        bool access(const Key& key);

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

template <typename Key>
Arc_cach<Key>::Arc_cach(std::size_t capacity):capacity_(capacity), target_recent_size_(0)
{
}

template <typename Key>
Arc_cach<Key>::Arc_cach(): Arc_cach(0)
{
}

template <typename Key>
void Arc_cach<Key>::insert_new(const Key& key)//сохдаем новые эллемент в T1 и запись в хеш таблице
{
    T1_.push_front(key);

    try
    {
        Node_hash node_hash{Type_list_save::T1, T1_.begin()};

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

template <typename Key>
void Arc_cach<Key>::move_begin_T2(Iterator position)
{
    T2_.splice(T2_.begin(), T2_, position);
}

template <typename Key>
void Arc_cach<Key>::repeated_hit_transfer_T2(Directory_Iterator hash_iterator)//переместить эллемент в T2 ЭТО ГЛАВАНАЯ ФУНКЦИЯ
{
    auto& node = hash_iterator -> second;

    switch(node.type_list)// из какого list мы вызываем
    {
        case(Type_list_save::T1):
        {
            T2_.splice(T2_.begin(), T1_, node.position);
            break;
        }
        case(Type_list_save::B1):
        {
            const auto delta = std::max(std::size_t{1}, B2_.size() / B1_.size());//TODO можно поменять на вариант из книги
            target_recent_size_ += std::min(delta, capacity_ - target_recent_size_);

            cache_one_ell_clean(Type_list_save::B1);//TODO это спорный момент бля дай бог заработает
            T2_.splice(T2_.begin(), B1_, node.position);
            break;
        }
        case(Type_list_save::B2):
        {
            const auto delta = std::max(std::size_t{1}, B1_.size() / B2_.size());
            target_recent_size_ -= std::min(delta, target_recent_size_);

            cache_one_ell_clean(Type_list_save::B2);
            T2_.splice(T2_.begin(), B2_, node.position);
            break;
        }
        case(Type_list_save::T2):
        {
            move_begin_T2(node.position);
            return;
        }
        case(Type_list_save::None):
        {
            std::cerr << "в хэш таблице несуществующий эллемент";
            assert(false);

            return;
        }
    }

    node.type_list = Type_list_save::T2;
}

template <typename Key>
void Arc_cach<Key>::delete_ell_list(Key_List& list_out)//понижение одного конкретного эллемента до B1 или B2 передаем что нужно понизить T1 T2 поницаем B1 B2 удаляем
{
    assert(!list_out.empty());

    auto found = general_hash_table.find(list_out.back());
    assert(found != general_hash_table.end());

    auto& node = found -> second;

    if(node.type_list == Type_list_save::T1)
    {
        B1_.splice(B1_.begin(), list_out, node.position);
        node.type_list = Type_list_save::B1;
    }
    else if(node.type_list == Type_list_save::T2)
    {
        B2_.splice(B2_.begin(), list_out, node.position);
        node.type_list = Type_list_save::B2;
    }
    else
    {
        general_hash_table.erase(found);
        list_out.pop_back();
    }
}

template <typename Key>
void Arc_cach<Key>::cache_one_ell_clean(Type_list_save request_in)//отвечает за выбор логики удаления и перемещения эллементов
{
    if(capacity_ == 0)
    {        
        return;
    }

    assert(size() <= capacity_);
    assert(target_recent_size_ <= capacity_);

    if(request_in == Type_list_save::None)
    {
        if(T1_.size() + B1_.size() == capacity_)
        {
            if(T1_.size() == capacity_)
            {
                auto oldest_ell = general_hash_table.find(T1_.back());//полностью удаляем из T1 без захода в B1
                assert(oldest_ell != general_hash_table.end());
                general_hash_table.erase(oldest_ell);
                T1_.pop_back();

                return;
            }

            delete_ell_list(B1_);
        }
        else if(size() == capacity_ && B1_.size() + B2_.size() == capacity_)
        {
            delete_ell_list(B2_);
        }

        return;
    }

    if(size() < capacity_)
    {
        return;
    }

    if(!T1_.empty() && T1_.size() > target_recent_size_ || (request_in == Type_list_save::B2 && T1_.size() == target_recent_size_))
    {
        delete_ell_list(T1_);
    }
    else
    {
        assert(!T2_.empty());
        delete_ell_list(T2_);
    }
}

template <typename Key>
bool Arc_cach<Key>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        cache_one_ell_clean(Type_list_save::None);
        insert_new(key);

        return false;
    }

    const auto old_type = found -> second.type_list;
    const bool hit = old_type == Type_list_save::T1 || old_type == Type_list_save::T2; //определить попадание

    repeated_hit_transfer_T2(found);

    return hit;
}

#endif
