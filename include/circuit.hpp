#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>
#include "matrix.hpp"
#include "vector.hpp"

enum CompType {
    resistor,
    voltage,
    current
};

struct Component {
    char name[20];
    double value;
    unsigned int n1;
    unsigned int n2;
    CompType type;
};

class Circuit {
    unsigned int nN;
    unsigned int nV;
    unsigned int nR;
    unsigned int nI;
    std::vector<Component> comp;

    public: 
    Circuit();
    Circuit(unsigned int nN, unsigned int nV, unsigned int nR, unsigned int nI);

    static Circuit createFromFile(const char* filename);
    void analyseCircuit();
};

Vector solveLinearSystem(Matrix A, Vector b);