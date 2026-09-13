#include <iostream>
#include <vector>
#include <string>

#include "lru_cach.hpp"
#include "lfu_cach.hpp"
#include "2Q.hpp"

int main()
{
    Lfu_cach copy(3);

    std::vector<int> as {1, 2, 3, 4, 2, 4, 1, 3, 5};
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