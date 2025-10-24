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
    float y, y_1, y_2;
    int out[3];
    double y_min = INFINITY, y_max = 0.0;

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
