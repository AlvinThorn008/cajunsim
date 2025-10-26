#pragma once
#include <vector>
#include <span>
#include <variant>
#include <cstdint>
#include <string_view>

class CharBitSet { 
    std::uint32_t x;
    std::uint32_t x_mut;

    public:
    CharBitSet();
    CharBitSet(std::uint32_t x);

    CharBitSet operator +(CharBitSet other);

    void set(char a);

    char next_char();

    bool contains(char a);

    std::uint32_t get_raw();
};

enum parse_state {
    S0, S1, S2, S3, S4
};

enum node_type { op_or = 0, op_and, val };

struct Node {
    node_type type;
    char name;
    bool inv;
};

enum parse_status { expect_binding, expect_equal, expect_var, unexpected_eof };

class Equation {
    char binding;
    std::vector<Node> nodes;
    CharBitSet vars;

    public:
    Equation();
    Equation(char binding, std::vector<Node> nodes, CharBitSet vars);
    char get_binding();
    CharBitSet get_vars();
    std::span<Node> get_nodes();
    bool evaluate(std::span<bool, 26> map);
};

using ParseResult = std::variant<parse_status, Equation>;

ParseResult parse(const char* str, size_t& cur, std::size_t len);
void print_stack(std::span<Node> stack);
std::vector<Equation> parse_file(const char* filename, bool& success);

/* Helper methods */
CharBitSet combine_vars(std::span<Equation> eqns);
int search_binding(std::span<Equation> eqns, char binding);

ParseResult parse2(std::string_view& str);
bool skip_while(std::string_view& str, char char_to_skip);

void print_equation_pretty(Equation& eqn);