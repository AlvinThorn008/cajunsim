#pragma once

#include <logic.hpp>
#include <span>

class LogicMap {
    // std::vector<bool> has an unfortunately specialization 
    // has treats it as a bit vector. This _can_ be useful but 
    // bitvectors elements are not addressable complicates my evaluate code.
    // Perhaps in the future, I may take advantage for this but I imagine
    // it would be less of hassle to handcraft my own implementation.

    std::vector<uint8_t> and_nodes;
    std::vector<uint8_t> or_nodes;
    size_t num_inputs;
    size_t num_products;
    size_t num_outputs;

    public:
    LogicMap();
    static LogicMap create_from_file(const char* logicfile, bool& success);
    bool evaluate(std::span<bool> values, size_t output_idx);
    void print_map();
};