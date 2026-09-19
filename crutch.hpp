#ifndef C
#define C

#include <iostream>

template<typename Value>
struct Access_Result
{
    bool hit;
    Value* found_ell;//будет {} если не нашли и будет на список если нашли
};

#endif