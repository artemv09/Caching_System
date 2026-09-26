#include <iostream>
#include <vector>
#include <string>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q_cach.hpp"
#include "arc_cach.hpp"
#include "lirs_cach.hpp"
#include "creat_cach.hpp"
#include "multi_level_cache.hpp"

int main()
{
    general_fun<int, int>(std::cin, std::cout);
    std::cout << "конец\n";
    
    return 0;
}
