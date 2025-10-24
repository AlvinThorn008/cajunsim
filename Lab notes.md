# Circuit Simulator

## 2. Preparation

### 2.1
The original C5 code was quite simple. It was capable of loading vectors, matrices and the netlist from files. It could build up the netlist given a circuit description and then analyze it via nodal analysis.
The original code used standard dynamic memory allocation constructs in C, libc file reading functions and finally structs to represent circuit, vector, matrix data. The API allowed the construction of arbitrary size vectors and matrices.
Vectors and matrices can be converted to simple classes backed by `std::valarray` so we can benefit from RAII.







