#ifndef COSPRICER_HPP
#define COSPRICER_HPP

#include <functional>
#include <complex>
#include <vector>


std::vector <double> COSPrice(
    std::function<std::complex<double>(double)> cf,
    bool call_put,
    double S0,
    double r,
    double T,
    const std::vector<double>& K,
    std::size_t N,
    double L);

#endif // !COSPRICER_HPP
