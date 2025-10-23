#include "plotter.hpp"

#include <iostream>

// https://stackoverflow.com/questions/4217037/catch-ctrl-c-in-c
static volatile sig_atomic_t running = 1;
static void ctrl_c_handler(int _) {
    (void)_;
    running = 0;
}

FourierPlotter::FourierPlotter(size_t coef_count, double base_frequency):
    max_amplitude(0.0),
    f0(base_frequency), cos_head(0), 
    sin_head(coef_count), coef_count(coef_count) { 
    coefs.resize(2 * coef_count, 0.0);
}

void FourierPlotter::append_cos_coef(double val) {
    if (cos_head == coef_count) return;
    if (val >= max_amplitude) max_amplitude = val;
    coefs[cos_head++] = val;
}

void FourierPlotter::append_sin_coef(double val) {
    if (sin_head == 2 * coef_count) return;
    if (val >= max_amplitude) max_amplitude = val;
    coefs[sin_head++] = val;
}

double FourierPlotter::sample(double x) {
    double value = 0.0;
    for (size_t i = 0; i < coef_count; i++) {
        value += coefs[i] * cos(i * f0 * x * PI/180.0) 
                + coefs[i + coef_count] * sin(i * f0 * x * PI/180.0);
    }
    return value;
}

double FourierPlotter::fourier_norm() {
    double value = 0.0;
    for (int i = 0; i < coef_count; i++) {
        value += coefs[i] * coefs[i] + coefs[i + coef_count] * coefs[i + coef_count];
    }
    return sqrt(value);
}


std::tuple<double, double> FourierPlotter::find_min_max() {
    long T0 = (long)(360.0 / f0) + 1;
    double min = INFINITY, max = 0.0;
    double val;
    for (long i = 0; i < T0; i++) {
        val = sample((double)i);
        min = __min(min, val);
        max = __max(max, val);
    }
    return {min, max};
}

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

        y = (sample((double)x) - min) / (max - min);
        printf(", y = " ORG "%.2f" RESET "|", y);

        if (x % TICK_INTERVAL == 0) printf(RED "---" RESET);
        else printf("   ");

        lerp(c1, c3, out, y);
        printf("\x1b[48;2;%d;%d;%dm", out[0], out[1], out[2]);
        plotval(y, PLOT_WIDTH);
        printf(RESET "\n");
    }
    running = 1;
    printf("\n\033[0m");
}

void plotval(float d, int width) {
    printf("%*s", (int)(d * width) + 1, "*");
}

void lerp(const int c1[3], const int c2[3], int out[3], float t) {
    out[0] = (int) ((c2[0] - c1[0]) * t + c1[0]);
    out[1] = (int) ((c2[1] - c1[1]) * t + c1[1]);
    out[2] = (int) ((c2[2] - c1[2]) * t + c1[2]);
}