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