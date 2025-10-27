#include <logic.hpp>
#include <stdio.h>
#include <iostream>
#include <bitset>

// Write `val` into the values at the given character indices
void write_val(uint32_t val, CharBitSet vars, std::span<bool> values) {
    while (val) {
        int idx = vars.next_char() - 'A';
        values[idx] = val & 1;
        val >>= 1;
    }
}

int main() {
    std::array<bool, 26> map;
    std::fill(std::begin(map), std::end(map), false);
    bool success;
    auto eqns = parse_file("res/ex1.logic", success);
    
    printf("Number of equations: %u\n", eqns.size());
    CharBitSet vars = combine_vars(eqns);

    //                    SRQPONMLKJIHGFEDCBA 
    CharBitSet ref_vars(0b0000000000000000111);

    printf("Vars: %u | RefVars: %u | Match: %u\n", vars.get_raw(), ref_vars.get_raw(), vars.get_raw() == ref_vars.get_raw());

    for (int i = 0; i < 7; i++) {
        write_val(i, vars, map);
        std::cout << "(ABC) = (" << std::bitset<3>(i) << ") : ";
        printf("x = %u\n", eqns[0].evaluate(map));
        std::fill(std::begin(map), std::end(map), false);
    }
}

