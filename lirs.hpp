#ifndef LIRS
#define LIRS

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <list>
#include <stdexcept>
#include <unordered_map>

template <typename Key>
class Lirs_cach
{
    private:
        using Key_List = std::list<Key>;
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

        using Directory = std::unordered_map<Key, Node_hash>;
        using Directory_Iterator = typename Directory::iterator;

        Key_List list_Q;//хранит только HIR resident
        Key_List list_S;

        Directory general_hash_table;

        void insert_new(const Key& key);
        void hit_hir_S_ell(Directory hash_table_it);
        void move_to_top_S(Directory hash_table_it);
        void erase_from_Q(Directory hash_table_it);
        void hit_hir_Q_ell(Directory hash_table_it);
        void hit_no_resident(Directory hash_table_it);
        void prune_S();//берет и очищает низ LIR от HIR
    
    public:
        explicit Lirs_cach(std::size_t capacity, std::size_t HIR_capacity);
        explicit Lirs_cach();

        Lirs_cach(const Lirs_cach&) = delete;
        Lirs_cach& operator=(const Lirs_cach&) = delete;

        bool access(const Key& key);

        std::size_t size() const noexcept
        {
            return list_Q.size() + LIR_capacity_;
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }
};

template <typename Key>
void Lirs_cach<Key>::insert_new(const Key& key)
{
    list_S.push_front(key);//новый эллемент всегда кладется в S

    if(LIR_capacity_ == LIR_count_)
    {
        if(HIR_capacity_ == HIR_resident_count_)//вытеснить HIR но оставить no_res...
        {
            Key key_hir_del = lisr_Q.back();
            Node& node_hir_del = (general_hash_table.find(key_hir_del)) -> second;

            node_hir_del.q_position = nullptr;
            node_hir_del.status = Status::HIR_NO_RES;
            node_hir_del.resident_flag = false;
            node_hir_del.in_q_flag = false;
            HIR_count_--;

            lisr_Q.pop_back();


        }
            list_Q.push_front(key);
            Node node{Status::HIR, true, false, true, nullptr, list_Q.begin()};
            general_hash_table.emplace(key, node);
            HIR_count_++;
    }
    else
    {
        Node node{Status::LIR, true, true, false, list_S.begin(), nullptr};
        general_hash_table.emplace(key, node);
        LIR_count_++;
    }
}

template <typename Key>
void Lirs_cach<Key>::move_to_top_S(Directory hash_table_it)
{   
    list_S.splice(list_S.begin(), lisr_S, hash_table_it -> second.s_position);
}

template <typename Key>
void Lirs_cach<Key>::hit_hir_S_ell(Directory hash_table_it)
{   
    Node& hit_hir_node = hash_table_it -> second; 

    move_to_top_S(hash_table_it);
    hit_hir_node -> status = Status::LIR; 
    erase_from_Q(hash_table_it);

    if(LIR_capacity_ == LIR_count_)
    {
        list_S.pop_back();
        prune_S();
        return;
    }
    HIR_resident_count_--;
    LIR_count_++;
    return;
}

template <typename Key>
void Lirs_cach<Key>::erase_from_Q(Directory hash_table_it)
{   
    auto& node = hash_table_it -> second;

    if (!node.in_q) return;
    
    Q_.erase(node.q_pos);
    node.in_q = false;
}

template <typename Key>
void Lirs_cach<Key>::prune_S()
{
    while(!S_.empty())
    {
        const Key& key = S_.back();

        auto hash_it = general_hash_table.find(key);
        auto& node = hash_it -> second;

        if(node.status == Status::LIR) break;

        node.in_s_flag = false;

        if(node.resident_flag)
        {
            S_.pop_back();
            continue;
        }

        general_hash_table.erase(hash_it);
        S_.pop_back();
    }
}

template <typename Key>
void Lirs_cach<Key>::hit_hir_Q_ell(Directory hash_table_it)
{
    auto& node = hash_table_it -> second;

    list_S.push_front(*(node.q_position));
    node.s_position = list_S.begin();
    node.in_s_flag = true;

    list_Q.splice(list_Q.begin(), lisr_Q, node.q_position);
}

template <typename Key>
void Lirs_cach<Key>::hit_no_resident(Directory hash_table_it)
{
    assert(LIR_capacity_ == LIR_count_);
    Node& node_hit = hash_table_it -> second;

    move_to_top_S(hash_table_it);
    node_hit.status = Status::LIR;
    
    if(HIR_capacity_ == HIR_resident_count_)
    {
        Directory oldes_Q_ell_it = general_hash_table.find(list_Q.back());
        erase_from_Q(oldes_Q_ell_it);
    }

    Directory oldes_S_ell_it = general_hash_table.find(list_S.back());
    oldes_S_ell_it -> second.status = Status::HIR;
    list_Q.(list_Q.begin(), list_S, list_S.begin());
    HIR_resident_count_++;

    prune_S();
}

template <typename Key>
bool Lirs_cach<Key>::access(const Key& key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found = general_hash_table.find(key);

    if(found == general_hash_table.end())
    {
        insert_new(key);
        return false;
    }

    switch(found -> second.status)
    {
        case(LIR):
        {
            move_to_top_S(found);
            return true;
        }
        case(HIR):
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
        case(NO_HIR):
        {
            hit_no_resident;
            return false;
        }
        default:
            break;
    }
}

template <typename Key>
Lirs_cach<Key>::Lirs_cach(std::size_t capacity, std::size_t HIR_capacity): 
    capacity_(capacity),
    LIR_capacity_(capacity - HIR_capacity),
    HIR_capacity_(HIR_capacity),
    LIR_count_(0),
    HIR_resident_count_(0)
{
    general_hash_table.reserve(capacity_ * 2);//зарезервировал чтобы меньше выделять по мере выполнения
}

template <typename Key>
Lirs_cach<Key>::Lirs_cach(): capacity_(0)
{
}


#endif