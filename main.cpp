#include <exception>
#include <iostream>

#include "multi_level_cache.hpp"

// Запускает ручной пример с config.txt; ошибки ввода выводит в stderr.
int main()
{
    try
    {
        general_fun<int, int>(std::cin, std::cout);
        std::cout << "конец\n";
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
