#include "COSPricer.hpp"

#include <cmath>
#include <stdexcept>

namespace
{
    constexpr double PI = 3.14159265358979323846;

    std::vector<double> ChiPsi(double a, double b, double c, double d,
        const std::vector<double>& k,
        bool return_chi)
    {
        std::vector<double> result(k.size(), 0.0);

        for (std::size_t j = 0; j < k.size(); ++j)
        {
            const double kj = k[j];
            const double u = kj * PI / (b - a);

            if (return_chi)
            {
                const double denom = 1.0 + u * u;

                const double expr1 =
                    std::cos(u * (d - a)) * std::exp(d)
                    - std::cos(u * (c - a)) * std::exp(c);

                const double expr2 =
                    u * std::sin(u * (d - a)) * std::exp(d)
                    - u * std::sin(u * (c - a)) * std::exp(c);

                result[j] = (expr1 + expr2) / denom;
            }
            else
            {
                if (j == 0)
                {
                    result[j] = d - c;
                }
                else
                {
                    result[j] =
                        (std::sin(u * (d - a)) - std::sin(u * (c - a))) / u;
                }
            }
        }

        return result;
    }

    std::vector<double> CallPutCoefficients(bool call_put,
        double a,
        double b,
        const std::vector<double>& k)
    {
        std::vector<double> H_k(k.size(), 0.0);

        if (call_put) // call
        {
            const double c = 0.0;
            const double d = b;

            auto chi = ChiPsi(a, b, c, d, k, true);
            auto psi = ChiPsi(a, b, c, d, k, false);

            for (std::size_t j = 0; j < k.size(); ++j)
            {
                H_k[j] = 2.0 / (b - a) * (chi[j] - psi[j]);
            }
        }
        else // put
        {
            const double c = a;
            const double d = 0.0;

            auto chi = ChiPsi(a, b, c, d, k, true);
            auto psi = ChiPsi(a, b, c, d, k, false);

            for (std::size_t j = 0; j < k.size(); ++j)
            {
                H_k[j] = 2.0 / (b - a) * (-chi[j] + psi[j]);
            }
        }

        return H_k;
    }
}


std::vector <double> COSPrice(
    std::function<std::complex<double>(double)> cf,
    bool call_put,
    double S0,
    double r,
    double T,
    const std::vector<double>& K,
    std::size_t N,
    double L)
{
    if (S0 <= 0.0)
        throw std::invalid_argument("S0 must be positive.");
    if (T <= 0.0)
        throw std::invalid_argument("T must be positive.");
    if (N == 0)
        throw std::invalid_argument("N must be positive.");
    if (L <= 0.0)
        throw std::invalid_argument("L must be positive.");
    if (K.empty())
        throw std::invalid_argument("Strike vector K must not be empty.");

    for (double strike : K)
    {
        if (strike <= 0.0)
            throw std::invalid_argument("All strikes must be positive.");
    }

    const std::complex<double> i(0.0, 1.0);

    std::vector<double> prices(K.size(), 0.0);

    const double a = -L * std::sqrt(T);
    const double b = L * std::sqrt(T);

    std::vector<double> k(N, 0.0);
    std::vector<double> u(N, 0.0);

    for (std::size_t j = 0; j < N; ++j)
    {
        k[j] = static_cast<double>(j);
        u[j] = k[j] * PI / (b - a);
    }

    const std::vector<double> H_k = CallPutCoefficients(call_put, a, b, k);

    const double discount = std::exp(-r * T);

    for (std::size_t m = 0; m < K.size(); ++m)
    {
        const double x0 = std::log(S0 / K[m]);

        std::complex<double> sum(0.0, 0.0);

        for (std::size_t j = 0; j < N; ++j)
        {
            std::complex<double> temp = cf(u[j]) * H_k[j];

            if (j == 0)
            {
                temp *= 0.5;
            }

            const std::complex<double> expo = std::exp(i * u[j] * (x0 - a));
            sum += expo * temp;
        }

        prices[m] = discount * K[m] * sum.real();
    }

    return prices;
}
