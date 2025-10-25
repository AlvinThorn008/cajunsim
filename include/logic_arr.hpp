#pragma once

#include <logic.hpp>
#include <span>

class LogicMap {
    std::vector<uint8_t> and_nodes;
    std::vector<uint8_t> or_nodes;
    size_t num_inputs;
    size_t num_products;
    size_t num_outputs;

    LogicMap();
    static LogicMap createFromFile(const char* logicfile, bool& success);
    bool evaluate(std::span<uint8_t> values, size_t output_idx);
};