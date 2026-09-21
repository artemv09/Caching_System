#include <iostream>
#include <vector>
#include <string>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q_cach.hpp"
#include "arc_cach.hpp"
#include "lirs_cach.hpp"
#include "creat_cach.hpp"
#include "work_cach.hpp"

int main()
{
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

    FILE* config = std::fopen("config.txt", "r");
    std::vector<Cache_name_size> cach_name_size = parsing_cach_parametr(config, std::cin);

    Multi_Level_Cach<int, int, Cach_Mode::Inclusive, true> x(cach_name_size ,data);

    int d = 0;
    std::cin >> d;
    int i = 0;
    while(d > i)
    {
        int key = 0;
        std::cin >> key;
        std::cout << "  QSX";

        int f = (x.access(key)).sought_element;
        std::cout << f;
    }

    

    std::vector<int> as {1, 2, 3, 4, 4};
    
    return 0;
}
