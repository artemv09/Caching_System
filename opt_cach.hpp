#ifndef OPT_CACH_
#define OPT_CACH_

#include <cassert>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <ostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "cach_type.hpp"

template <typename Key, typename Value>
class OptCache
{
    private:
        using Big_Data = std::unordered_map<Key, Value>;
        using Cach_List = std::list<Entry<Key, Value>>;
        using Cach_Iterator = typename Cach_List::iterator;

        struct Node
        {
            std::deque<std::size_t> future_positions; // заполняем его в конструкторе содержит все обращения 
            Cach_Iterator resident_position;
        };

        using Directory = std::unordered_map<Key, Node>;
        using Directory_Iterator = typename Directory::iterator;

        Cach_List resident;

        Directory future_requests_hash;
        const Big_Data* big_data;

        std::size_t capacity_;
        std::size_t request_count_;
        std::size_t current_position_ = 0;
        std::size_t hit_count_ = 0;

        const Value& get_long_data(const Key& key) const;
        
        Directory_Iterator choose_who_delete();

    public:
        explicit OptCache(std::size_t capacity, const std::vector<Key>& requests, const Big_Data& data);

        OptCache(const OptCache&) = delete;
        OptCache& operator=(const OptCache&) = delete;

        Public_Access_Result<Value> access(const Key& key);

        std::size_t size() const noexcept
        {
            return resident.size();
        }

        std::size_t capacity() const noexcept
        {
            return capacity_;
        }

        std::size_t hits() const noexcept
        {
            return hit_count_;
        }

        void print_statistics(std::ostream& output) const;
};

template <typename Key, typename Value>
OptCache<Key, Value>::OptCache(std::size_t capacity, const std::vector<Key>& requests, const Big_Data& data):
    big_data(&data),
    capacity_(capacity),
    request_count_(requests.size())
{
    for(std::size_t position = 0; position < requests.size(); position++)
    {
        auto inserted = future_requests_hash.try_emplace(requests[position]);
        Node& node = inserted.first -> second;

        if(inserted.second)
        {
            node.resident_position = resident.end();
        }

        node.future_positions.push_back(position);
    }
}

template <typename Key, typename Value>
const Value& OptCache<Key, Value>::get_long_data(const Key& key) const
{
    auto found = big_data -> find(key);

    if(found == big_data -> end())
    {
        throw std::runtime_error("попытка найти не существующий ключ");
    }

    return found -> second;
}

template <typename Key, typename Value>
typename OptCache<Key, Value>::Directory_Iterator OptCache<Key, Value>::choose_who_delete()
{
    auto victim = future_requests_hash.end();
    std::size_t farthest_position = 0;

    for(const auto& entry : resident)
    {
        auto found = future_requests_hash.find(entry.key);

        const auto& positions = (found -> second).future_positions;

        if(positions.empty())
        {
            return found;
        }

        if(victim == future_requests_hash.end() || positions.front() > farthest_position)
        {
            victim = found;
            farthest_position = positions.front();
        }
    }

    return victim;
}

template <typename Key, typename Value>
Public_Access_Result<Value> OptCache<Key, Value>::access(const Key& key)
{
    if(current_position_ == request_count_)
    {
        throw std::logic_error("OPT: история запросов уже обработана");
    }

    auto found = future_requests_hash.find(key);
    auto& future_deq_node = (found -> second).future_positions;

    if(found == future_requests_hash.end() || future_deq_node.empty() || future_deq_node.front() != current_position_)
    {
        throw std::logic_error("OPT: ключ не совпадает со следующим запросом истории");
    }

    Node& node = found -> second;
    const bool hit = node.resident_position != resident.end();
    const Value& value = hit ? node.resident_position -> value : get_long_data(key);

    Public_Access_Result<Value> result{hit, value};

    if(!hit && capacity_ != 0)
    {
        auto victim = future_requests_hash.end();

        if(size() == capacity_)
        {
            victim = choose_who_delete();
        }

        resident.emplace_back(key, value);
        node.resident_position = std::prev(resident.end());

        if(victim != future_requests_hash.end())
        {
            resident.erase((victim -> second).resident_position);
            (victim -> second).resident_position = resident.end();
        }
    }

    node.future_positions.pop_front();
    current_position_++;

    if(hit)
    {
        hit_count_++;
    }

    return result;
}

template <typename Key, typename Value>
void OptCache<Key, Value>::print_statistics(std::ostream& output) const
{
    output << "\n========== OPT statistics ============\n"
           << "Total hits: " << hits() << '\n'
           << "Total capacity: " << capacity() << '\n'
           << "======================================\n";
}

#endif
