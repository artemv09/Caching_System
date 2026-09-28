#include "creat_cach.hpp"
#include <charconv>

std::size_t read_size(std::istream& input, const std::string& label, std::size_t limit)
{
    std::string token;
    long long value = 0;

    if(!(input >> token))
    {
        throw std::invalid_argument("Не прочитано: " + label);
    }

    const auto parsed = std::from_chars(token.data(), token.data() + token.size(), value);

    if(parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size() || value < 0 ||
       static_cast<unsigned long long>(value) > limit)
    {
        throw std::invalid_argument("Недопустимое число: " + label);
    }

    return static_cast<std::size_t>(value);
}

static void require_end(std::istream& input)
{
    std::string extra;

    if(input >> extra)
    {
        throw std::invalid_argument("Лишние данные: " + extra);
    }

    if(!input.eof())
    {
        throw std::runtime_error("Ошибка чтения потока");
    }
}

std::vector<Cache_name_size> parsing_cach_parametr(std::istream& config, std::istream& input)
{
    const auto count = read_size(config, "число уровней", 64);

    if(count == 0)
    {
        throw std::invalid_argument("Нужен хотя бы один уровень");
    }

    std::vector<Cache_name_size> result;
    result.reserve(count);

    for(std::size_t i = 0; i < count; ++i)
    {
        Cache_name_size level{};

        if(!(config >> level.name_cach))
        {
            throw std::invalid_argument("Не прочитано имя политики");
        }
        
        cache_type(level.name_cach);
        result.push_back(level);
    }

    require_end(config);

    for(auto& level : result)
    {
        level.capacity = read_size(input, "capacity", MAX_PAGE_KEY);

        if(level.name_cach == "LIRS")
        {
            level.hir_capacity = read_size(input, "HIR", MAX_PAGE_KEY);
            // Конструктор задаёт общие для программы и тестов правила HIR.
            Lirs_cach<int, int> validate(level.capacity, level.hir_capacity);
        }
    }
    return result;
}

std::vector<int> read_requests(std::istream& input, int max_key)
{
    if(max_key < 1 || max_key > MAX_PAGE_KEY)
    {
        throw std::invalid_argument("Недопустимый предел ключей");
    }

    const auto count = read_size(input, "количество запросов", MAX_REQUESTS);

    std::vector<int> keys;
    keys.reserve(count);

    for(std::size_t i = 0; i < count; i++)
    {
        const auto key = read_size(input, "ключ", static_cast<std::size_t>(max_key));

        if(key == 0)
        {
            throw std::invalid_argument("Ключи начинаются с 1");
        }

        keys.push_back(static_cast<int>(key));
    }
    return keys;
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
