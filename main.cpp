#include <iostream>
#include <vector>
#include <string>

// #include "lru_cach.hpp"
// #include "lfu_cach.hpp"
// #include "2Q_cach.hpp"
// #include "arc_cach.hpp"
#include "lirs.hpp"

int main()
{
    Lirs_cach<int> copy(4, 1);

    std::vector<int> as {1, 2, 3, 4, 4};
    int count = 0;
    for(int i : as)
    {
        if(copy.access(i))
        {
            count++;
        }
    }

    std::cout << count << "\n";
    
    return 0;
}
