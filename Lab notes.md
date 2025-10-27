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
![[Untitled Diagram.drawio.png]]
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
![[Pasted image 20251026231720.png]]
**Generated waveform**
![[Pasted image 20251026231847.png]]
**Reference image**
![[Pasted image 20251026233036.png]]
## 3. Laboratory Work

### 3.1 Unit testing

#### Circuit simulator
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
![[Pasted image 20251027013139.png]]

**Falstad simulation**
![[Pasted image 20251027010045.png]]

#### Fourier plotter
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
![[Pasted image 20251027015147.png]]