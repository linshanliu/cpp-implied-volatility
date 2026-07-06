#include "EuropeanOption.hpp"
#include <cmath>
#include<iostream>

double normal_cdf(double x)
{
	return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

double normal_pdf(double x)
{
	const double INV_SQRT_2PI = 0.3989422804014327;
	return INV_SQRT_2PI * std::exp(-0.5 * x * x);
}


/// <summary>
/// 
/// </summary>

EuropeanOption::EuropeanOption():K(100),T(1),r(0.0375), S0(100), sigma(0.2), call_put(1){}

EuropeanOption::EuropeanOption(double K_in, double T_in, double r_in, double S0_in, double sigma_in, bool call_put_in):K(K_in), T(T_in), r(r_in), S0(S0_in), sigma(sigma_in), call_put(call_put_in) {}

EuropeanOption::EuropeanOption(const EuropeanOption& source) :S0(source.S0), K(source.K), r(source.r), sigma(source.sigma), T(source.T),call_put(source.call_put){}

EuropeanOption::~EuropeanOption() {}


double EuropeanOption::BS_Price() const
{
	double d1 = (std::log(S0 / K) + (r + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
	double d2 = (std::log(S0 / K) + (r - sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
	if (call_put == 1)
	{
		return S0 * normal_cdf(d1) - K * std::exp(-r * T) * normal_cdf(d2);
	}
	else
	{
		return -S0 * normal_cdf(-d1) + K * std::exp(-r * T) * normal_cdf(-d2);
	}
}




double EuropeanOption::Vega() const
{
	double d1 = (std::log(S0 / K) + (r + sigma * sigma / 2) * T) / (sigma * std::sqrt(T));
	return S0 * std::sqrt(T) * normal_pdf(d1);
}


void EuropeanOption::SetK(double K_in)
{
	K = K_in;
}

void EuropeanOption::SetT(double T_in)
{
	T = T_in;
}
void EuropeanOption::SetSigma(double sigma_in)
{
	sigma = sigma_in;
}


