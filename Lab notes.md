# Circuit Simulator

## 2. Preparation

### 2.1
The original C5 code was quite simple. It was capable of loading vectors, matrices and the netlist from files. It could build up the netlist given a circuit description and then analyze it via nodal analysis.
The original code used standard dynamic memory allocation constructs in C, libc file reading functions and finally structs to represent circuit, vector, matrix data. The API allowed the construction of arbitrary size vectors and matrices.
Vectors and matrices can be converted to simple classes backed by `std::valarray` so we can benefit from RAII. `std::valarray` has support for element-wise operations which I assume could be useful for simplifying the row reduction solver.

```cpp
class Vector : public std::valarray<double> {
    public:
    using std::valarray<double>::valarray;
};

class Matrix {
    std::valarray<double> _data;
    size_t rows;
    size_t cols;

    public:
    explicit Matrix(size_t rows, size_t cols);
    Matrix(size_t rows, std::initializer_list<double> init);
    Matrix(double val, size_t rows, size_t cols);
    double& operator()(size_t r, size_t c);
    std::slice_array<double> operator[](size_t row);
    std::slice_array<double> column(size_t col);
    size_t row_size();
    size_t col_size();
    size_t size();
};
```
I have defined some constructors on Matrix that make it easy to integrate with the old c5 code. It can take an initializer list of elements as well as a value to fill the matrix with. The explicit constructor `Matrix(size_t rows, size_t cols)` prevents the confusion initialization of a matrix using its dims
```cpp
// Without explicit, Matrix(size_t rows, size_t cols) is called.
Matrix mat = { 1, 2 };
// so mat is a 1x2 matrix - rather than a matrix that contains 1 and 2
```

For _circuit.c_. I changed the `Circuit` struct to a class. Originally, the Circuit stores the length of the components list inline but by switching to the more convenient `std::vector`, the `Circuit::nN` field can be dropped.

```cpp
class Circuit {
    unsigned int nN;
    unsigned int nV;
    unsigned int nR;
    unsigned int nI;
    std::vector<Component> comp; // comp.size() replaces nc
    
    public:
    Circuit();
    Circuit(unsigned int nN, unsigned int nV, unsigned int nR, unsigned int nI);
    static Circuit createFromFile(const char* filename);
    void analyseCircuit();
};
```

`Component` remains a struct however. It is plain old data which is encapsulated via its composition in `Circuit`. As `Circuit::comp` is private, invalid `Component` instances cannot be inserted into it.

The other methods: `createFromFile` and `analyseCircuit` are mostly the same with modifications to work with the overloaded operator for Matrix element access
```cpp
Matrix mat(3, 
{ 
	1, 2, 3, 
	4, 5, 6
});

std::cout << mat(1, 1) << std::endl;
// 5
```

### Testing
```sh
~/Documents/Docs/Uni/eee/programming/labs/rho2> ./build/bin/main
[0] Circuit simulator
[1] Plotter
[2] Logic gate array

Enter a mode: 0

Welcome to MSPICE
Please enter a file to analyze: res/example1.cir
----------------------------
 Voltage sources: 1
 Current sources: 0
       Resistors: 3
           Nodes: 3
----------------------------
 Node   0 =   0.000000 V
 Node   1 = -12.000000 V
 Node   2 =  -6.000000 V
----------------------------
 I(V1)    =   0.006000 A
----------------------------
~/Documents/Docs/Uni/eee/programming/labs/rho2>
```

### UML diagram
![[Untitled Diagram.drawio.png|800]]
### Plotter

_old plotter code_
```cpp
while (running) {
	x += 1;
	printf("x = " ORG "%0*d" RESET, 8, x);

	y = (sinf(FREQ * (x) * PI/180) + 1) / 2; // Normalized offset
	printf(", y = " ORG "%.2f" RESET "|", y);

	if (x % TICK_INTERVAL == 0) printf(RED "---" RESET);
	else printf("   ");
        
	lerp(c1, c3, out, y); // sample the gradient at y
	printf("\x1b[48;2;%d;%d;%dm", out[0], out[1], out[2]);
	plotval(y, PLOT_WIDTH);
	printf(RESET "\n");
}
```

New plotter class
```cpp
class FourierPlotter {
	// List of fourier coefficients
	// [a0 a1 a2 a3 ... aN b0 b1 b3 ... bN]
    std::vector<double> coefs;

    size_t coef_count;
    
    // write heads for the coefs vector
    size_t cos_head;
    size_t sin_head;
    
    double f0; // fundamental frequency
    double max_amplitude;

    public:
    FourierPlotter(size_t coef_count, double base_frequency);

	// Appends coefficients into the correct half of the vector
    void append_cos_coef(double val);
    void append_sin_coef(double val);

	// Evaluate the fourier series at x
    double sample(double x);
    
    double fourier_norm();
    
    // Determine the fourier series' range. Used for normalization purposes
    std::tuple<double, double> find_min_max();
    
    // Begin printing the wave. num_points is the number of samples taken.
    void start_plotter(size_t num_points);
};
```

In the constructor, I initialize the `coefs` vector to be twice `coef_count`. The first half belongs to the cosine coefficients and the second to the sine coefficients. The `append_*_coef` methods are guarded via the `cos_head` and `sin_head` attributes. 

```cpp
FourierPlotter::FourierPlotter(size_t coef_count, double base_frequency):
    max_amplitude(0.0),
    f0(base_frequency), cos_head(0),
    sin_head(coef_count), coef_count(coef_count) {
    
    coefs.resize(2 * coef_count, 0.0);
}

void FourierPlotter::append_cos_coef(double val) {
    if (cos_head == coef_count) return; // guard
    if (val >= max_amplitude) max_amplitude = val;
    coefs[cos_head++] = val;
}
```
No coefficients will be written once `cos_head` reaches the half of the first half. 

```cpp
void FourierPlotter::start_plotter(size_t num_points) {
    // Handling ctrl-c allows the program to reset the terminal before exiting
    signal(SIGINT, ctrl_c_handler);
    
    std::tuple<double, double> range = find_min_max();
    double min = std::get<0>(range), max = std::get<1>(range);

    long x = 0;
    float y;
    int out[3];

    while (num_points-->0 && running) {
        x += 1;
        printf("x = " ORG "%0*d" RESET, 8, x);
        
        // Normalize output to [0, 1]
        y = (sample((double)x) - min) / (max - min);
        printf(", y = " ORG "%.2f" RESET "|", y);

        if (x % TICK_INTERVAL == 0) printf(RED "---" RESET);
        else printf("   ");

        lerp(c1, c3, out, y);
        printf("\x1b[48;2;%d;%d;%dm", out[0], out[1], out[2]);
        plotval(y, PLOT_WIDTH);
        printf(RESET "\n");
    }

    running = 1;
    printf("\n\033[0m");

}
```
In `start_plotter`, `find_min_max` is used to normalize the fourier series. 
$$ 0 \leq \frac{f(x) - f_{min}}{f_{max} - f_{min}} \leq 1 $$
#### C2 bonuses
In my original C2 code, I made the wave be printed out with a coloured area. The code linearly interpolates between two predefined colours. 
```cpp
void lerp(const int c1[3], const int c2[3], int out[3], float t) {
    out[0] = (int) ((c2[0] - c1[0]) * t + c1[0]);
    out[1] = (int) ((c2[1] - c1[1]) * t + c1[1]);
    out[2] = (int) ((c2[2] - c1[2]) * t + c1[2]);
}
```
`t` represents the progression along the line between the two colours. Since `y` is normalized to `[0, 1]`, the colours chosen should lie on the line **segment** joining the two colours in RGB vector space. The spaces used to offset the `*` symbol will have the chosen colour at their background. This means the gradient direction is downwards.

```cpp
// https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
static volatile sig_atomic_t running = 1;
static void ctrl_c_handler(int _) {
    (void)_;
    running = 0;
}

/* set ctrl_c handler */
signal(SIGINT, ctrl_c_handler);
```
If ctrl+c is entered, the program terminates (sending SIGINT before it ends). If this happens before the current text colour setting has been reset, the user's terminal will print with that colour. See the image below:

![[Pasted image 20251024173947.png]]
To fix this, I created a function to run when ctrl+c is pressed. All it does is set a static global to `false`. When this happens, the while loop in `start_plotter` terminates and the rest of the function executes as normal. Proceeding the while loop is a printf to reset the terminal colour.

### UML diagram

![[fourierplotter_uml.png]]
### Testing
![[Pasted image 20251026231720.png|500]]
**Generated waveform**
![[Pasted image 20251026231847.png]]
### Logic gates
The logic gate task is to devise a method that computes the value of a Sum of Products (SOP) expression given some representation for it and the values of its variables. I implemented two different approaches to representing the sum of products expression.

#### Approach 1: Literal SOP expression
Given a sum of products expression in its literal form e.g. `x = ABC' + BD`, a parse stack of the inputs is created which can later be used to evaluate the expression. There are 3 keys concepts here: **bindings**, **variables** and **values**. A binding is the name of an equation(in this case `x`). Variables are the uppercase letters referenced in expression. Values are the evaluation-time substitutes for variables.
The grammar is very simple
```ebnf
<Equation> = <lower> <space> "=" <space> <term> (<space> "+" <space> <term>)*
<term> = <var> (<space> <var>)* (*1 or more <var>s separated optional whitespace*)
<var> = <upper> "'"?
<space> = (" ")*  (*0 or more spaces*)
<lower> = [a-z]
<upper> = [A-Z]
```
It turns out this grammar also happens to be regular so it can parsed via regex which C++ supports in its standard library. An explicit state machine would also work but the implementation I made turned out to be rather verbose. Due to this, I reimplemented the state machine implicitly using standard iterations and selection structures. Before presenting that, I will explain the relevant classes.

```cpp
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

enum node_type { op_or = 0, op_and, val };

struct Node {
    node_type type;
    char name;
    bool inv;
};

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
```

The `Equation` class holds the binding and variables of an expression. The internal node list `nodes` is essentially the parser output. `Equation::evaluate` takes an array(technically a span) of values. Since the variables are limited to uppercase letters, the max size of the values array is 26. I use this to get constant time access to the value for each variables during evaluation. The restriction to A-Z led to the design of `CharBitSet`, a very basic handrolled bitset that enforces bit writes and reads to be limited to the least 26 bits.

**Parsing**
```cpp
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
        // Section #1
        if (!islower(str[0])) { err = expect_binding; break; }
        func = str[0];
        err = expect_equal;
        str = str.substr(1); // skip <binding name>
        if (eof = !skip_while(str, ' ')) break;
        if (str[0] != '=') break;
        str = str.substr(1); // skip `=`
        err = expect_var;
        if (eof = !skip_while(str, ' ')) break;
  
	    // Section #2
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
```
Following the grammar, the general operation of the code is fairly clear. Section 1 parses `x = ` while Section 2 parses the terms using `'+'`, `EOF` or `'\n'` as a delimiter, signalling to group previously seen variables. The function can then be called multiple times in order to parse several equations in a file.

**Evaluation**
```cpp
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
```
To understand the evaluation logic, consider the node list for the expression: `AB + B'C + A'C`
NodeList: `[A, B, op_and, B', C, op_and, A', C, op_or]`
The `op_and` node essentially delimits the variables, forming terms while `op_or` is just a terminator. Evaluation is simply a matter of ANDing variables until an operator appears, then the result of ANDing is ORed with an accumulator variable(`res`).

#### Approach 2: Logic matrix
This is the method we were expected to implement. It is comparatively much simpler. I will only focus on the evaluation logic for this part.

```cpp
bool LogicMap::evaluate(std::span<bool> values, size_t output_idx) {
    if (output_idx >= num_outputs) return false;
    size_t row_start = num_products * output_idx;

	// Slice row `output_idx` from the output matrix (or_nodes)
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
                c(I, R, V)
                    = V  if (I R) = (0 1)
                    = V' if (I R) = (1 0)
                    = 1  if (I R) = (0 0)
                    = 0  if (I R) = (1 1)
                c(I, R, V) = V ~I + ~V ~R (minimized)
                */
                res_and &= (val & ~inv) | (~val & ~reg);
                not_clear |= inv | reg;
            }
            res |= res_and & not_clear;
        }
    }
    return res == 1;

}
```

```
     =====Inputs matrix======       =Output matrix=
     ~a | a | ~b | b | ~c | c         x | y | z  
[0]   0   1    1   0    0   1   <--   1   1   0
[1]   1   0    1   0    1   0   <--   0   1   0
[2]   0   1    0   1    1   0   <--   1   0   1
 
 x = [1 0 1]
 x_out = and([0]) || and([2])

```
Each referenced product has its value calculated by working with the inverse and regular line of each variable in pairs. The boolean function `c(I, R, V)` selects the either the variable or its complement depending on whether I or R is set. When they are both set, the result should be zero since `~A . A = 0`. When neither line is set, the result is 1. This effectively removes the pair from the SOP term. Finally, the `not_clear` flag is used to ignore the products if no lines are set.

**Testing**
res/ex1.logic
```haskell
x =  ABC' + A'BC + A'B'C
```
![[Pasted image 20251027185631.png|500]]
## 3. Laboratory Work

### 3.1 Unit testing

#### Circuit simulator
In this test, I compare the simulator results with an established circuit simulator (Falstad). The nodes are being measured with references to Node 0 in [Falstad](https://www.falstad.com/circuit/circuitjs.html).
```cpp
#include <circuit.hpp>
#include <stdio.h>

int main() {
    Circuit c1 = Circuit::createFromFile("res/example1.cir");
    c1.analyseCircuit();
}
```

**res/example1.cir**
```
V1 1 0 12.0
R1 1 2 1000.0
R2 2 0 2000.0
R3 2 0 2000.0
```

**Console output**
![[Pasted image 20251027013139.png|400]]

**Falstad simulation**
![[Pasted image 20251027010045.png|400]]
As can be seen from the meter reading, the results agree.

#### Fourier plotter
In this test, I forgo the user interface using the class that handles plotting directly. Square wave coefficients are computed and passed into the plotter. Finally, the output is compared to a reference graph created using [desmos](https://desmos.com/calculator)
```cpp
#include <plotter.hpp>

int main() {
    FourierPlotter plotter(14, 2.67);
    int count = 14;
    while(count-->0) plotter.append_cos_coef(0.0);

    for (int i = 1; i < 29; i += 2) {
        plotter.append_sin_coef(0.0);
        plotter.append_sin_coef(1.0 / (double)i);
    }
    plotter.start_plotter(5000);
}
```

**Terminal**
![[Pasted image 20251027015147.png]]
**Desmos**
![[Pasted image 20251027154626.png]]
#### Logic simulator

**Code**
```cpp
#include <logic.hpp>
#include <stdio.h>
#include <iostream>
#include <bitset>

// Write `val` into the values at the given character indices
void write_val(uint32_t val, CharBitSet vars, std::span<bool> values) {
    while (val) {
        int idx = vars.next_char() - 'A';
        values[idx] = val & 1;
        val >>= 1;
    }
}

int main() {
    std::array<bool, 26> map;
    std::fill(std::begin(map), std::end(map), false);
    bool success;
    auto eqns = parse_file("res/ex1.logic", success);
    printf("Number of equations: %u\n", eqns.size());
    CharBitSet vars = combine_vars(eqns);

    //                    SRQPONMLKJIHGFEDCBA
    CharBitSet ref_vars(0b0000000000000000111);

    printf("Vars: %u | RefVars: %u | Match: %u\n", vars.get_raw(), ref_vars.get_raw(), vars.get_raw() == ref_vars.get_raw());

    for (int i = 0; i < 7; i++) {
        write_val(i, vars, map);
        std::cout << "(ABC) = (" << std::bitset<3>(i) << ") : ";
        printf("x = %u\n", eqns[0].evaluate(map));
        std::fill(std::begin(map), std::end(map), false);
    }
}
```

**Simulator input file** (res/ex1.logic)
```haskell
x =  ABC' + A'BC + A'B'C
```

**Program output**
![[Pasted image 20251027035727.png|500]]
