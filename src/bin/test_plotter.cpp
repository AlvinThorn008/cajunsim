#include <plotter.hpp>

int main() {
    int count = 8;
    FourierPlotter plotter(count, 2.67);
    int ccount = count;
    while(ccount-->0) plotter.append_cos_coef(0.0);

    for (int i = 0; i < count; i += 1) {
        double coef = (double)(i * i + 1) / (double)((2 * i + 1)*(2 * i + 1));
        plotter.append_sin_coef(coef);
    }
    plotter.start_plotter(400000);
    return 0;
}