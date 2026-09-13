#include "2Q_cach.hpp"

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
    hach_table.emplace(key, list.begin());
}

Two_Q_Cach::Two_Q_Cach(): capacity_(0)
{
}

Two_Q_Cach::Two_Q_Cach(std::size_t capacity): capacity_(capacity), Kout(capacity / 2), Kin(capacity / 4)
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

    if(check(key, a1out_table))
    {
        a1out.erase(found_ell -> second);
        a1out_table.erase(found_ell);

        rules_displacment();
        insert_new(key, am_table, am);       
    }

    else if(check(key, am_table))
    {
        make_recent_am(found_am_ell -> second);
        return true;
    }

    else if(check(key, a1in_table))
    {
        return true;
    }

    else
    {
        rules_displacment();
        insert_new(key, a1in_table, a1in);
    }

    return false;
}

bool Two_Q_Cach::check(int key, std::unordered_map<int, Iterator>& hach_table)//проверка наличия ключа в таблице
{
    return hach_table.find(key) != hach_table.end();
}

std::size_t Two_Q_Cach::size() const noexcept
{
    return a1in.size() + am.size();
}
std::size_t Two_Q_Cach::capacity() const noexcept
{
    return capacity_;
}

void Two_Q_Cach::rules_displacment()//правила для выброса эллемента из am и a1in
{
    if(size() == capacity_)
    {
        if(a1in.size() > Kin)
        {
            int evicted_key = a1in.back();
            delet_hach_list_last(a1in_table, a1in);
            insert_new(evicted_key, a1out_table, a1out);
        }
        else
        {
            delet_hach_list_last(am_table, am);
        }
    }

    if(a1out.size() > Kout)
    {
        delet_hach_list_last(a1out_table, a1out);
    }
}