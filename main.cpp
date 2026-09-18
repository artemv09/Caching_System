#include <iostream>
#include <vector>
#include <string>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q_cach.hpp"
#include "arc_cach.hpp"
#include "lirs_cach.hpp"
#include "creat_cach.hpp"

int main()
{
    FILE* config = std::fopen("config.txt", "r");
    std::vector<Cache_name_size> cach_name_size = parsing_cach_parametr(config, std::cin);

    std::vector<Cach_ptr<int>> general_cach = create_cach<int>(cach_name_size);
    
    std::unordered_map<int, int> data
    {
        {1, 100},
        {2, 200},
        {3, 300},
        {4, 400},
        {5, 500},
        {6, 600},
        {7, 700},
        {8, 800},
        {9, 900},
        {10, 1000}
    };

    

    std::vector<int> as {1, 2, 3, 4, 4};
    
    return 0;
}
