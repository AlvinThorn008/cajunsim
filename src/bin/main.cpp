#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string_view>
#include <regex>
#include <bitset>
#include <circuit.hpp>
#include <plotter.hpp>
#include <logic.hpp>

void plotter();
void circuit_sim();
void logic();
bool main_menu();

int main() {
    while (main_menu());
    printf("Thank you for using Cajun5im\n");

    return 0;
}

bool main_menu()
{
    printf("\n[0] Circuit simulator\n[1] Plotter\n[2] Logic gate array\n[3] Quit\n\n");
    printf("Enter a mode: ");
    unsigned int mode;
    std::cin >> mode;
    printf("\n");

    switch (mode) {
        case 0: circuit_sim(); break;
        case 1: plotter(); break;
        case 2: logic(); break;
        case 3: return false;
        default: break;
    }

    printf("\n\n");
    return true;
}

void logic() {
    printf("Welcome to Logicer\n");
    printf("Please enter a file to analyze: ");
    std::string filename;
    std::cin >> filename;
    bool success;
    std::vector<Equation> eqns = parse_file(filename.c_str(), success);
    if (!success) { printf("File could not be opened"); return; }
    CharBitSet vars = combine_vars(eqns);
    std::array<bool, 26> map;
    std::fill(std::begin(map), std::end(map), false);

    printf("Bindings loaded:");
    for (auto eqn : eqns) {
        printf(" %c", eqn.get_binding());
    }
    printf("\n\n");
    printf("You may now evaluate bindings or set variables\n");
    printf("To evaluate a binding:\n    eval <binding>\nwhere <binding> is the binding name\n\n");
    printf("To set a variable:\n    set <var> <val>\nwhere <var> is the variable name and <val> is the value\n\n");
    printf("Enter\n   Q or q        Quit the app\n    listb        List available bindings\n    listv       List available variables\n");

    std::string input("");
    while (true) {
        printf(">> ");
        std::getline(std::cin >> std::ws, input);
        if (input == "q" || input == "Q") break;
        else if (input == "listb") {
            printf("Bindings loaded:");
            for (auto eqn : eqns) {
                printf(" %c", eqn.get_binding());
            }
            printf("\n");
            continue;
        } else if (input == "listv") {
            printf("Referenced variables:");
            while (char c = vars.next_char()) printf(" %c", c);
            printf("\n");
            continue;
        }
        if (input.size() == 0) continue;
        size_t idx = input.find_first_of(' ');
        if (idx == std::string::npos) { printf("Invalid syntax\n"); continue; }
        auto view = std::string_view(input);
        if (view.substr(0, idx) == "eval" && (idx + 1 < input.size())) {
            char binding = view[idx+1];
            if (binding < 'a' || binding > 'z') { printf("Invalid syntax: binding name must be a lower case letter\n"); continue; }
            int idx = search_binding(eqns, binding);
            if (idx == -1) { printf("No binding named %c\n", binding); continue; }

            printf("eval(%c) -> %s\n", binding, eqns[idx].evaluate(map) ? "true" : "false"); 

        } else if (view.substr(0, idx) == "set" && (idx + 3 < input.size())) {
            char var = view[idx+1];
            bool val = view[idx+3] == '1';

            if (view[idx+2] != ' ') { printf("Invalid syntax: space expected between var and value\n"); continue; }
            if (var < 'A' || var > 'Z') { printf("Invalid syntax: variable name must be an upper case letter\n"); continue; }
            if (!vars.contains(var)) { printf("No bindings reference the var '%c'\n", var); continue; }
            
            map[var - 'A'] = val;
            printf("~> %c = %d\n", var, val);
        } else {
            printf("Invalid syntax\n");
            continue;
        }
        
    }
}

void circuit_sim() {
    Circuit c;

    printf("Welcome to MSPICE\n");
    printf("Please enter a file to analyze: ");
    std::string filename;
    std::cin >> filename;

    c = Circuit::createFromFile(filename.c_str());

    c.analyseCircuit();
}

void plotter() {
    int coefs = 0;
    double coef, f0;
    size_t num_points = 0;
    printf("Enter the number of coefficients: ");
    std::cin >> coefs;
    printf("Enter the base frequency: ");
    std::cin >> f0;
    FourierPlotter p = FourierPlotter(coefs, f0);
    for (int i = 1; i <= coefs; i++) {
        printf("Enter cos coef #%d: ", i);
        std::cin >> coef;
        p.append_cos_coef(coef);
    }
    for (int i = 1; i <= coefs; i++) {
        printf("Enter sin coef #%d: ", i);
        std::cin >> coef;
        p.append_sin_coef(coef);
    }
    printf("Enter the number of points to display: ");
    std::cin >> num_points;
    p.start_plotter(num_points);
    
}