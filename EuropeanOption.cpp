#include "EuropeanOption.hpp"
#include <cmath>
#include<iostream>

using namespace std;

double normal_cdf(double x)
{
	return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

double normal_pdf(double x)
{
	const double INV_SQRT_2PI = 0.3989422804014327;
	return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}



EuropeanOption::EuropeanOption():K_(100),T_(1), isCall_(true){}

EuropeanOption::EuropeanOption(double K, double T, bool isCall):K_(K), T_(T), isCall_(isCall) {}

EuropeanOption::EuropeanOption(const EuropeanOption& source) : K_(source.K_), T_(source.T_),isCall_(source.isCall_){}

EuropeanOption::~EuropeanOption() {}


double EuropeanOption::BS_Price(double S0, double r, double sigma, double q) const
{
	double d1 = (log(S0 / K_) + (r - q + sigma * sigma / 2) * T_) / (sigma * sqrt(T_));
	double d2 = d1 - sigma * sqrt(T_);
	if (isCall_ == 1)
	{
		return S0 * exp(-q * T_) * normal_cdf(d1) - K_ * exp(-r * T_) * normal_cdf(d2);
	}
	else
	{
		return -S0 * exp(-q * T_) * normal_cdf(-d1) + K_ * exp(-r * T_) * normal_cdf(-d2);
	}
}




double EuropeanOption::Vega(double S0, double r, double sigma, double q) const
{
	double d1 = (log(S0 / K_) + (r - q + sigma * sigma / 2) * T_) / (sigma * sqrt(T_));
	return S0 * exp(-q * T_) * sqrt(T_) * normal_pdf(d1);
}


void EuropeanOption::SetK(double K)
{
	K_ = K;
}

void EuropeanOption::SetT(double T)
{
	T_ = T;
}




double EuropeanOption::ImpliedVolatility(double S0, double r, double q, double marketPrice, int maxIteration, double tol)
{
    double sigma = 0.4;

    // TODO: Pass dividend yield q once EuropeanOption supports it.
    /*EuropeanOption RefOption(K, T, r, S0, sigma, isCall);*/




    for (int i = 0; i < maxIteration; i++)
    {
        double price = this -> BS_Price(S0,r,sigma,q);
        double vega = this -> Vega(S0, r, sigma, q);


        // Temporary debug output. Remove or disable after testing.
        /*std::cout << "Iteration: " << i
            << ", sigma: " << sigma
            << ", price: " << price
            << ", market price: " << marketPrice
            << ", error: " << std::abs(price - marketPrice)
            << ", vega: " << vega
            << '\n';*/


        if (std::abs(price - marketPrice) < tol)
        {
            return sigma;
        }


        // TODO: Re-enable this after handling failure properly.
        if (std::abs(vega) < 1e-8)
            break;


        /*std::cout << RefOption.Vega();*/
        sigma -= (price - marketPrice) / vega;


        // Prevent negative volatility during Newton iterations.
        sigma = std::max(sigma, 1e-6);

    }


    // TODO: Replace this temporary fallback with an exception
    // or a structured convergence result.
    throw std::runtime_error("Implied volatility failed to converge.");

    return sigma;
}