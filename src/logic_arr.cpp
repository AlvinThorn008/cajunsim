#include <logic_arr.hpp>
#include <fstream>
#include <span>

LogicMap::LogicMap(): and_nodes(0), or_nodes(0) {}

LogicMap LogicMap::createFromFile(const char* filename, bool& success) {
    success = false;
    LogicMap map;
    std::ifstream logicfile(filename);
    if (!logicfile.is_open()) { return {}; }

    if (!(logicfile >> map.num_inputs >> map.num_outputs >> map.num_products)) { return {}; }
    logicfile.get(); // skip '-'
    for (int i = 0; i < 2 * map.num_inputs * map.num_products; i++) {
        uint8_t num;
        logicfile >> num;
        map.and_nodes.push_back(num == 1);
    }
    logicfile.get(); // skip '-'
    for (int i = 0; i < map.num_outputs * map.num_products; i++) {
        uint8_t num;
        logicfile >> num;
        map.or_nodes.push_back(num == 1);
    }
    success = true;
    return map;
}

bool LogicMap::evaluate(std::span<uint8_t> values, size_t output_idx) {
    if (output_idx >= num_outputs) return false;
    size_t row_start = num_products * output_idx;
    std::span<uint8_t> row(
        or_nodes.begin()+row_start, 
        or_nodes.begin()+row_start+num_products
    );
    uint8_t res = 0;
    for (int i = 0; i < row.size(); i++) {
        if (row[i] == 1) {
            uint8_t res_and = 1;
            for (int j = 0; j < num_inputs * 2; j += 2)
                res_and &= ~(and_nodes[i * num_inputs * 2 + j] & values[j / 2]) & (and_nodes[i * num_inputs * 2 + j] & values[j / 2]);
            res |= res_and;
        }    
    }
    return res == 1;
}
