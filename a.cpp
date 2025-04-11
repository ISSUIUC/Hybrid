#include<iostream>

uint32_t rand_nsmb(uint32_t *state) {
    uint64_t value = (uint64_t)(*state) * 1664525 + 1013904223;
    return *state = value + (value >> 32);
}

int main() {
    uint32_t* cycles = (uint32_t*)malloc(4ull * 1ull<<32);
    if(!cycles) std::cerr << "aloc fail\n";
    uint32_t cycle_num = 0;
    size_t s = 0;
    for(uint32_t i = 0; i != -1; i++) {
        if(cycles[i] == 0) {
            cycle_num ++;
            uint32_t v = i;
            while(cycles[v] == 0) {
                cycles[v] = cycle_num;
                rand_nsmb(&v);
            }
        }
    }
    std::cout << cycle_num << '\n';

    return 0;
}