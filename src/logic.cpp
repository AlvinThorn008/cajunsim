#include <stdlib.h>
#include <cstdio>
#include <ctype.h>
#include <fstream>
#include <sstream>
#include <string_view>
#include "logic.hpp"

/*
    Parses an parametric boolean algebra expression.

    The syntax is specified below:

    ```ebnf
    [a-z] <spaces> = <spaces> ([A-Z]'? <spaces>)+ (<plus> <spaces> ([A-Z]'? <spaces>)+)*
    ```

    For example, `x = A B' + CD + S'` is valid.
*/
ParseResult parse2(std::string_view& str) {
    size_t cur = 0;
    char func;
    ParseResult res;
    parse_status err;
    std::vector<Node> stack;
    CharBitSet vars(0);
    bool eof = false;
    bool valid = false;
    while (true) {
        if (!islower(str[0])) { err = expect_binding; break; }
        func = str[0];
        err = expect_equal;
        str = str.substr(1); // skip <binding name>
        if (eof = !skip_while(str, ' ')) break;
        if (str[0] != '=') break;
        str = str.substr(1); // skip `=`
        err = expect_var;
        if (eof = !skip_while(str, ' ')) break;

        while (true) {
            if (!isupper(str[0])) { break; }
            vars.set(str[0]);
            stack.push_back({val, str[0], false});
            str = str.substr(1); // skip <var name>
            valid = true;
            if (str.length() > 0 && str[0] == '\'') { stack.back().inv = true; str = str.substr(1); } // skip `'`
            if (eof = !skip_while(str, ' ')) break;
            if (str[0] == '+') { stack.push_back({op_and, '\0', false}); valid = false; str = str.substr(1); } // skip `+`
            if (eof = !skip_while(str, ' ')) break;
        }
        break;
    }
    res = err;
    if (eof && valid) { stack.push_back({op_or, '\0', false}); res = Equation(func, stack, vars); }
    return res;
}

bool skip_while(std::string_view& str, char char_to_skip) {
    size_t cur = 0;
    bool in_eqn = false;
    while (in_eqn = (cur < str.length() && str[cur] != '\n')) {
        printf("cur = %u, c = %c\n", cur, str[cur]);
        if (str[cur] == char_to_skip) cur++;
        else { cur = cur == 0 ? 0 : cur--; break; }
    }
    str = str.substr(cur);
    return in_eqn;
}

ParseResult parse(const char* str, size_t& cur, std::size_t len) {
    parse_state s = S0;
    parse_status err;
    char func;
    bool skip_space = false;
    bool may_end = false;
    bool reject = false;
    bool seen_newline = false;
    std::vector<Node> stack;
    CharBitSet vars(0);

    ParseResult res;

    while (true) {
        if (reject) { break; }
        if (seen_newline) break;
        if (cur >= len || str[cur] == '\n') { 
            if (may_end) stack.push_back({op_or, '\0', false});
        }
        if (cur >= len) break;
        if (str[cur] == '\n') { cur++; seen_newline = true; }
        if (skip_space) { 
            while(str[cur] == ' ') {
                cur++;
                if (cur >= len) break;
            }
            skip_space = false;
        }
        // printf("State: S%d; Input: ", s);
        // str[cur] == ' ' 
        // ? printf("' '") 
        // : (str[cur] == '\n' 
        //     ? printf("'\\n'") 
        //     : printf("'%c'", str[cur]));
        // printf("\n");
        switch (s) {
            case S0: {
                if (islower(str[cur])) {
                    func = str[cur];
                    s = S1;
                    cur++;
                    skip_space = true;
                } else { err = expect_binding; reject = true; }
                break;
            }
            case S1: {
                if (str[cur] == '=') { cur++; s = S2; skip_space = true; }
                else { err = expect_equal; reject = true; }
                break;
            }
            case S2: {
                if (isupper(str[cur])) { 
                    stack.push_back({val, str[cur], false});
                    vars.set(str[cur]);
                    cur++; 
                    s = S3;
                    may_end = true;
                } else { err = expect_var; reject = true; }
                break;
            }
            case S3: {
                if (str[cur] == '\'') {
                    stack.back().inv = true;
                    may_end = true;
                    cur++;
                }
                s = S4;
                skip_space = true;
                break;
            }
            case S4: {
                if (str[cur] == '+') {
                    stack.push_back({op_and, '\0', false});
                    cur++;
                    may_end = false;
                    skip_space = true;
                    s = S2;
                }
                else if (isupper(str[cur])) {
                    may_end = true; 
                    s = S2;
                }
                break;                
            }
        }
    }

    if (reject) {
        res = err;
        cur++;
    } else if (may_end) {
        res = Equation(func, stack, vars);
    } else { res = unexpected_eof; }
    return res;
}

CharBitSet::CharBitSet() {}
CharBitSet::CharBitSet(std::uint32_t x): x(x & 0x03ffffff), x_mut(x & 0x03ffffff) {}
CharBitSet CharBitSet::operator+(CharBitSet other) {
    return CharBitSet(other.x | x);
}
std::uint32_t CharBitSet::get_raw() { return x; }
void CharBitSet::set(char a) {
    x |= 1u << (a - 'A');
    x &= 0x03ffffff;
    x_mut = x;
}
char CharBitSet::next_char() { 
    int idx = std::countl_zero(x_mut);
    x_mut &= ~(0x80000000 >> idx);
    if (idx > 31) return '\0';
    else return 'A' + (31 - idx);
}

bool CharBitSet::contains(char a) { 
    return (x & (1 << (a - 'A'))) != 0;
}


Equation::Equation() {}
Equation::Equation(char binding, std::vector<Node> nodes, CharBitSet vars)
    : binding(binding), nodes(nodes), vars(vars) {}

char Equation::get_binding() { return binding; }
CharBitSet Equation::get_vars() { return vars; }

std::span<Node> Equation::get_nodes() { return std::span(nodes.begin(), nodes.size()); }

bool Equation::evaluate(std::span<bool, 26> map) {
    std::vector<bool> stack;
    bool res = 0;
    bool term = 1;
    for (size_t i = 0; i < nodes.size(); i++) {
        Node node = nodes[i];
        switch (node.type) {
            case val: term = term && (map[node.name - 'A'] != node.inv); break;
            case op_and:
            case op_or: {
                res = res || term;
                term = 1;
                break;
            }
            default: break;
        }
    }
    return res;
}

void print_stack(std::span<Node> stack) {
    for (auto node : stack) {
        if (node.type == val) printf("[%s%c]\n", node.inv ? "~" : "", node.name);
        else { printf("[%s]\n", node.type ? "and" : "or"); }
    }
}

std::vector<Equation> parse_file(const char* filename, bool& success) {
    std::ifstream logicfile(filename);
    if (!logicfile.is_open()) { success = false; return {}; }
    success = true;
    logicfile.seekg(0, std::ios::end);
    size_t size = logicfile.tellg();
    std::string buffer(size, ' ');
    logicfile.seekg(0);
    logicfile.read(&buffer[0], size);

    std::vector<Equation> eqns;

    size_t cur = 0;
    
    while (cur < size) {
        ParseResult res = parse(buffer.c_str(), cur, size);
        if (Equation* eqn = std::get_if<Equation>(&res)) eqns.push_back(*eqn);
        else {
            parse_status status = std::get<parse_status>(res);
            switch (status) {
                case expect_binding: printf("Expected a binding (lower case letter) at %d", cur); break;
                case expect_equal: printf("Expected an equal '=' at %d", cur); break;
                case expect_var: printf("Expected a variable (upper case letter) at %d", cur); break;
                case unexpected_eof: printf("Unexpected Eof"); break;
            }
            printf("\n");
        }
        while (cur < size && (buffer[cur] == '\n' || buffer[cur] == ' ')) { cur++; }
        if (cur >= size) break;
    }

    return eqns;
}

/* Helper methods */
CharBitSet combine_vars(std::span<Equation> eqns) {
    CharBitSet out;
    for (auto eqn : eqns) out = out + eqn.get_vars();
    return out;
}

int search_binding(std::span<Equation> eqns, char binding) {
    for (int i = 0; i < eqns.size(); i++)
        if (eqns[i].get_binding() == binding) return i;

    return -1;
}

/* Print an equation in this textual format */
void print_equation_pretty(Equation& eqn) {
    std::span<Node> nodes = eqn.get_nodes();
    printf("%c = ", eqn.get_binding());
    for (size_t i = 0; i < nodes.size(); i++) {
        Node node = nodes[i];
        if (node.type == val) {
            printf("%c%s", node.name, node.inv ? "'" : "");
        } else if (node.type == op_and) {
            printf(" + ");
        } else {
            break;
        }
    }
}

