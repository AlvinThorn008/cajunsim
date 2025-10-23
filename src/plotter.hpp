#pragma once

#include <cstdio>
#include <vector>
#include <csignal>
#include <cmath>
#include <tuple>

#define PI 3.1415926535897932384626433f

// Source: https://stackoverflow.com/a/23657072
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define ORG "\x1B[38;2;255;143;79m"

// Console width available for '*'s to be printed.
const int PLOT_WIDTH = 160;
// How frequently should graduations appear
const int TICK_INTERVAL = 10;
const int c1[3] = {255, 102, 0};
const int c3[3] = {198, 120, 221};

class FourierPlotter {
    std::vector<double> coefs;
    size_t coef_count;
    size_t cos_head;
    size_t sin_head;
    double f0;
    double max_amplitude;
    
    public:
    FourierPlotter(size_t coef_count, double base_frequency);

    void append_cos_coef(double val);
    void append_sin_coef(double val);
    double sample(double x);
    double fourier_norm();
    std::tuple<double, double> find_min_max();
    void start_plotter(size_t num_points);
};

void plotval(float d, int width);
void lerp(const int c1[3], const int c2[3], int out[3], float t);