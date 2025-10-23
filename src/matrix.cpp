#include "matrix.hpp"

Matrix::Matrix(size_t rows, size_t cols) 
    : rows(rows), cols(cols), _data(rows * cols) {}

Matrix::Matrix(double val, size_t rows, size_t cols) 
    : rows(rows), cols(cols), _data(val, rows * cols) {}

Matrix::Matrix(size_t cols, std::initializer_list<double> init)
    : rows(init.size() / cols), cols(cols), _data(init) {}

std::slice_array<double> Matrix::operator[](size_t row) {
    return _data[std::slice(cols * row, cols, 1)];
}
double& Matrix::operator()(size_t r, size_t c) { return _data[c + r * cols]; }
std::slice_array<double> Matrix::column(size_t col) {
    return _data[std::slice(col, cols, cols)];
}

size_t Matrix::row_size() { return rows; }
size_t Matrix::col_size() { return cols; }
size_t Matrix::size() { return _data.size(); }

