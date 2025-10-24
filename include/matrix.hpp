#pragma once

#include <cassert>
#include <valarray>
#include <initializer_list>

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
