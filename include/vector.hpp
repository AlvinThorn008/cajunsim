#pragma once

#include <cassert>
#include <valarray>
#include <initializer_list>

class Vector : public std::valarray<double> {
    public:
    using std::valarray<double>::valarray;
};
