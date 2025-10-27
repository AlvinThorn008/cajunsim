#include <circuit.hpp>
#include <stdio.h>

int main() {
    Circuit c1 = Circuit::createFromFile("res/example1.cir");

    c1.analyseCircuit();
}

