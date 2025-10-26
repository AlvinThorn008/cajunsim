#include <logic_arr.hpp>
#include <fstream>
#include <span>

LogicMap::LogicMap(): and_nodes(0), or_nodes(0) {}

LogicMap LogicMap::create_from_file(const char* filename, bool& success) {
    success = false;
    LogicMap map;
    std::ifstream logicfile(filename);
    if (!logicfile.is_open()) { return {}; }
    char skip;

    // TODO: Implement checks to validate .larr input.
    // ...limit symbols to 0 and 1
    if (!(logicfile >> map.num_inputs >> map.num_outputs >> map.num_products)) { return {}; }
    logicfile >> skip; // skip '-'
    for (int i = 0; i < 2 * map.num_inputs * map.num_products; i++) {
        // >> treats uint8_t as a char (because uint8_t is an alias of unsigned char)
        // so no int parsing for uint8_t - could maybe use a bigger int
        char num;
        logicfile >> num;
        map.and_nodes.push_back(num == '1');
    }
    logicfile >> skip; // skip '-'
    for (int i = 0; i < map.num_outputs * map.num_products; i++) {
        char num;
        logicfile >> num;
        map.or_nodes.push_back(num == '1');
    }
    success = true;
    return map;
}

// 
bool LogicMap::evaluate(std::span<bool> values, size_t output_idx) {
    if (output_idx >= num_outputs) return false;
    size_t row_start = num_products * output_idx;
    std::span<uint8_t> row(
        or_nodes.begin()+row_start, 
        or_nodes.begin()+row_start+num_products
    );
    uint8_t res = 0;
    
    for (int i = 0; i < row.size(); i++) {
        uint8_t not_clear = 0;
        int product_start = i * num_inputs * 2;
        if (row[i] == 1) {
            uint8_t res_and = 1;
            for (int j = 0; j < num_inputs * 2; j += 2) {
                uint8_t val = values[j / 2];
                uint8_t inv = and_nodes[product_start + j];
                uint8_t reg = and_nodes[product_start + j + 1]; 
                /*
                c(a, c, v) 
                    = v  if (a c) = (0 1)
                    = v' if (a c) = (1 0)
                    = 1  if (a c) = (0 0)
                    = 0  if (a c) = (1 1)
                // V ~I + ~V ~R
                */
                res_and &= (val & ~inv) | (~val & ~reg);
                not_clear |= inv | reg;
            }
            res |= res_and & not_clear;
        }
    }

    return res == 1;
}

void LogicMap::print_map() {
    printf("I: %u O: %u P: %u\n", num_inputs, num_outputs, num_products);
    printf("or_nodes.size() = %u\nand_nodes.size() = %u\n", or_nodes.size(), and_nodes.size());

    for (size_t i = 0; i < and_nodes.size(); i++) {
        if (i % (num_inputs * 2) == 0) printf("\n");
        printf("%u ", and_nodes[i]);
    }

    for (size_t i = 0; i < or_nodes.size(); i++) {
        if (i % (num_products) == 0) printf("\n");
        printf("%u ", or_nodes[i]);
    }
}