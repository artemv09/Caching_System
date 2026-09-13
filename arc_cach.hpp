#ifndef ARC
#define ARC

#include <iostream>
#include <vector>
#include <string>
#include <list>
#include <unordered_map>
#include <iterator>
#include <cstddef>
#include <cassert>

template <typename Key>//тип значения

class Arc_cach
{
    private:
        using Key_List = std::list<Key>;
        using Iterator = typename Key_List::iterator;

        enum class Type_list_save// в каком списке храниться эллемент
        {
            T1,
            T2,
            B1,
            B2
        };

        struct Node_hash
        {
            Type_list_save type_list;
            Iterator position;
        };

        using Directory = std::unordered_map<Key, Entry>;
        using Directory_Iterator = typename Directory::iterator;
        
        std::size_t capacity_;
        std::size_t size_;
        std::size_t target_recent_size_ ;

        Key_List T1_;
        Key_List T2_;
        Key_List B1_;
        Key_List B2_;

        Directory general_hash_table;

        void insert_new(Key key);//создать новый эллемент и запись в хеш таблице для T1
        void move_begin_T2(Iterator position);//перенести в начало T2
        void repeated_hit_transfer_T2(Directory_Iterator hash_iterator);//перенос эллемента 1 в T2
        //Type_list_save check_ell(Key key);// хз можно наверное убрать 
        void delete_ell_list(Key_List& list_out);// удаляет эллемент из кеша и соотвественно заполняет его в новый список B1/2
        void cache_one_ell_clean(Type_list_save request_in);//правида для удаления эллемента из T
        void rules_clear_ell_B(Type_list_save request_in);//правила для удаления эллемента из B

    public:
        explicit ArcCache(std::size_t capacity);
        explicit ArcCache();

        ArcCache(const ArcCache&) = delete;
        ArcCache& operator=(const ArcCache&) = delete;

        bool access(const Key& key);//нельзя менять и мы хотим поместить в кеш оригинал а не копию 

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

        //void clear() noexcept;// Очищает содержимое и историю, сохраняя вместимость.
};

template <typename Key>
Arc_cach<Key>::ArcCache(std::size_t capacity): capacity_(capacity), target_recent_size_(0), size_(0)
{
}

template <typename Key>
Arc_cach<Key>::ArcCache(): capacity_(0), target_recent_size_(0), size_(0)
{
}

template <typename Key>
void Arc_cach<Key>::insert_new(Key key)//создать новый эллемент и запись в хеш таблице
{
    T1_.push_front(key);
    Node_hash node_hash{Type_list_save::T1, T1_.begin()};
    general_hash_table.emplace(key, node_hash);
}

template <typename Key>
void Arc_cach<Key>::move_begin_T2(Iterator position)//перенести в начало T2
{
    T2_.splice(T2_.begin(), T2_, position);
}

template <typename Key>
void Arc_cach<Key>::repeated_hit_transfer_T2(Directory_Iterator hash_iterator)//перенос эллемента в T2
{
    Iterator it = hash_iteterator -> second.position;
    hash_iteterator -> second.type_list = Type_list_save::T2;

    switch(hash_iteterator -> second.type_list):
        case(Type_list_save::T1)
        {
            T2_.splice(T2_.begin(), T1_, it);
            break;
        }
        case(Type_list_save::B1)
        {
            T2_.splice(T2_.begin(), B1_, it);
            target_recent_size_++;
            break;
        }
        case(Type_list_save::B2)
        {
            T2_.splice(T2_.begin(), B2_, it);
            target_recent_size_--;
            break;
        }
}

// Type_list_save Arc_cach<Key>::check_ell(Key key)// хз можно наверное убрать

template <typename Key>
void Arc_cach<Key>::delete_ell_list(Key_List& list_out)// удаляет эллемент из кеша и соотвественно заполняет его в новый список B1/2
{
    Key key = list_out.back();
    Directory_Iterator it_dec = general_hash_table.find(key);
    Type_list_save& type = it_dec -> second.type_list;

    if(type == Type_list_save::T2)
    {
        rules_clear_ell_B(type)
        type = Type_list_save::B2;
        B2_.splice(B2_.begin(), list_out, it_dec -> second.position);
    }
    else if(type == Type_list_save::T1 && T1.size() != capacity)
    {
        rules_clear_ell_B(type)
        type = Type_list_save::B1;
        B1_.splice(B1_.begin(), list_out, it_dec -> second.position);
    }
    else if(type == Type_list_save::T1 && T1.size() != capacity)
    {
        general_hash_table.erase(it_dec);
        list_out.pop_back();
    }
    else if(type == Type_list_save::B2 || type == Type_list_save::B1)
    {
        general_hash_table.erase(it_dec);
        list_out.pop_back();
    }
    else
    {
        std::cerr << "Ошибка, попытка освободить несуществующий эллемент в ARC\n";
    }
}

template <typename Key>
void Arc_cach<Key>::cache_one_ell_clean(Type_list_save request_in)//правида для удаления эллемента из T при нехватке места
{
    if(capacity_ == 0 || size() < capacity_)
    {
        return;
    }

    assert(size() == capacity_ && target_recent_size_ <= capacity_);

    if (!T1_.empty() && T1_.size() > target_recent_size_ || (request_in == Type_list_save::B2 && T1_.size() == target_recent_size_))
    {
        delete_ell_list(T1_);
    }
    else
    {
        delete_ell_list(T2_)
    }

}

template <typename Key>
void Arc_cach<Key>::rules_clear_ell_B(Type_list_save request_in)//правила для удаления эллемента из B при нехзватке места
{
    if(capacity_ == 0)
    {
        return;
    }

    assert(size() <= capacity_);
    assert(T1_.size() + B1_.size() <= capacity_);
    assert(B1_.size() + B2_.size() <= capacity_);

    if(T1_.size() + B1_.size() == capacity_)
    {
        if(!T1_.empty)
        {
            delete_ell_list(B1_);
        }
    }
    else if(size() == capacity_ && B1_.size() + B2_.size() == capacity_)
    {
        delete_ell_list(B2_);
    }

    else
    {
        return;
    }
}

template <typename Key>
bool Arc_cach<Key>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found_ell = general_hash_table.find(key);

    if(found_ell != general_hash_table.end())
    {
        if(found_ell -> second.type_list != Type_list_save::T2 )
        {
            repeated_hit_transfer_T2(found_ell)
        }
        else
        {
            move_begin_T2(found_ell -> second.position)
            return true;
        }
    }
    else
    {
        cache_one_ell_clean(Type_list_save::T1)
        insert_new(key)
    }

    return false;
}


// template <typename Key>
// void Arc_cach<Key>::()

#endif