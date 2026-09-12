#include "2Q.hpp"

void Two_Q_Cach::delet_hach_list_last(std::unordered_map<int, Iterator>& hach_table, std::list<int>& list)
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

void Two_Q_Cach::make_recent_am(Iterator position)// перенести существующий узел списка в head
{
    am.splice(am.begin(), am, position);
}

void Two_Q_Cach::insert_new(int key, std::unordered_map<int, Iterator>& hach_table, std::list<int>& list)// добавить новый узел списка и соответствующую запись в хеш-таблицу
{
    list.push_front(key);
    hach_table.emplace(key, hach_table.begin());
}

Two_Q_Cach::Two_Q_Cach(): capacity_(0)
{
}

Two_Q_Cach::Two_Q_Cach(std::size_t capacity): capacity_(capacity), Kin(capacity/4), Kout(capacity/2)
{
}

bool Two_Q_Cach::access(int key)
{
    if(capacity_ == 0)
    {
        return false;
    }

    auto found_ell = a1out_table.find(key);
    auto found_am_ell = am_table.find(key);
    auto found_a1in_ell = a1in_table.find(key);

    if(found_ell != a1out_table.end())
    {
        insert_new(key, am_table, am);
        a1out.erase(found_ell -> second);
        a1out_table.erase(found_ell);
    }

    else if(found_am_ell != am_table.end())
    {
        make_recent_am(found_am_ell -> second);
        return true;
    }

    else if(found_a1in_ell != a1in_table.end())
    {
        return true;
    }

    else
    {
        insert_new(key, a1in_table, a1in);
        
        if(a1out.size() > Kout)
        {
            delet_hach_list_last(a1out_table, a1out);
        }
    }

    if(am.size() > capacity_)
    {
        if(a1in.size() > Kin)
        {
            delet_hach_list_last(a1in_table, a1in);
        }
        else
        {
            delet_hach_list_last(am_table, am);
        }
        return false;
    }

    return false;
}

bool Two_Q_Cach::check_a1out(int key)
{
    return a1in_table.find(key) != a1in_table.end();
}

std::size_t Two_Q_Cach::size() const noexcept
{
    return a1in.size() + am.size();
}
std::size_t Two_Q_Cach::capacity() const noexcept
{
    return capacity_;
}