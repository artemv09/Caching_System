#include "creat_cach.hpp"

std::vector<Cache_name_size> parsing_cach_parametr(FILE* config, std::istream& input)
{
    if (config == nullptr)
    {
        throw std::runtime_error("config file не открылся");
    }

    std::size_t count_level;

    if (std::fscanf(config, "%zu", &count_level) != 1)
    {
        throw std::runtime_error("не смог прочитать count");
    }

    std::vector<Cache_name_size> general_cach;
    general_cach.reserve(count_level);

    for (std::size_t i = 0; i < count_level; ++i)
    {
        char name[32];
        if (std::fscanf(config, "%31s", name) != 1)
        {
            throw std::runtime_error("не смог прочитать name");
        }

        std::size_t capacity;
    
        if(name == "LIRS")
        {
            std::size_t hir_capacity;
            if (!(input >> hir_capacity))
            {
                throw std::runtime_error("не смог прочитать capacity");
            }

            general_cach.push_back(Cache_name_size{std::string{name}, capacity, hir_capacity});
        }

        general_cach.push_back(Cache_name_size{std::string{name}, capacity, 0});
    }

    return general_cach;
}

Type_Cach cache_type(const std::string& name)
{
    if (name == "LRU")
        return Type_Cach::LRU;

    if (name == "LFU")
        return Type_Cach::LFU;

    if (name == "2Q")
        return Type_Cach::TWO_Q;

    if (name == "ARC")
        return Type_Cach::ARC;

    if (name == "LIRS")
        return Type_Cach::LIRS;

    throw std::invalid_argument("Неопознаный тип при parse_cache_type");
}
