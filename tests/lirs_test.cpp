#include "lirs.hpp"
#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <vector>

void check(const char* name, std::size_t c, std::size_t h,
           std::vector<int> keys, const std::string& expected) {
    Lirs_cach<int> cache(c, h);
    assert(cache.size() == 0);
    assert(keys.size() == expected.size());
    for (std::size_t i = 0; i < keys.size(); ++i) {
        bool hit = cache.access(keys[i]);
        if (hit != (expected[i] == '1') || cache.size() > c) {
            std::cerr << name << " failed at index " << i << '\n';
            std::abort();
        }
    }
    std::cout << "PASS " << name << '\n';
}

int main() {
    check("capacity one, HIR=0", 1, 0, {1,1,2,1,3,2,1,1}, "01000001");
    check("capacity one, HIR=1", 1, 1, {1,1,2,1,3,2,1,1}, "01000001");
    for (std::size_t c = 0; c <= 12; ++c) {
        for (std::size_t h = 0; h <= 13; ++h) {
            const bool invalid = h > c || (c > 1 && (h == 0 || h == c));
            bool rejected = false;
            try {
                Lirs_cach<int> cache(c, h);
                assert(cache.size() == 0);
            } catch (const std::invalid_argument&) {
                rejected = true;
            }
            assert(rejected == invalid);
        }
    }
    std::cout << "PASS constructor quotas\n";
    check("empty", 3, 1, {}, "");
    check("zero capacity", 0, 0, {1, 1}, "00");
    check("warmup and LIR hits", 30, 10, {1,2,3,4,2,4,1,3,5}, "000011110");
    check("resident HIR promotion and Q-only hit", 3, 1, {1,2,3,3,1,1}, "000111");
    check("ghost returns", 3, 1, {1,2,3,4,3,4,3}, "0000001");
    check("stack pruning then Q-only hit", 3, 1, {1,2,3,1,2,3,3}, "0001111");
    check("evict HIR outside S", 3, 1, {1,2,3,3,4,1}, "000100");
    check("ghost return evicts HIR outside S", 3, 1, {1,2,3,4,3,4,1}, "0000000");
    Lirs_cach<std::string> strings(3, 1);
    assert(!strings.access("a"));
    assert(strings.access("a"));
    Lirs_cach<int> empty;
    assert(empty.size() == 0 && empty.capacity() == 0 && !empty.access(1));
    std::mt19937 rng(42);
    for (std::size_t h : {0u, 1u}) {
        Lirs_cach<int> cache(1, h);
        int previous = -1;
        for (int i = 0; i < 10000; ++i) {
            int key = static_cast<int>(rng() % 5);
            assert(cache.access(key) == (key == previous));
            assert(cache.size() == 1);
            previous = key;
        }
    }
    std::cout << "PASS 20000 single-page accesses against previous-key model\n";
    for (std::size_t c = 2; c <= 12; ++c) {
        for (std::size_t h = 1; h < c; ++h) {
            Lirs_cach<int> cache(c, h);
            for (int i = 0; i < 2000; ++i) {
                cache.access(static_cast<int>(rng() % (3*c)));
                assert(cache.size() <= c);
            }
        }
    }
    std::cout << "PASS string keys, default constructor, 132000 random accesses (seed 42; capacity checks)\n";
}
