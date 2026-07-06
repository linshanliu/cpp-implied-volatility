#include "EuropeanOption.hpp"
#include "UtilityFunctions.hpp"
#include <iostream>
#include <cmath>

double ImpliedVolatility(double S0,
    double K,
    double r,
    double T,
    double marketPrice,
    bool isCall,
    int maxIteration,
    double tol)
{
    double sigma = 0.4;
    EuropeanOption RefOption(K, T, r, S0, sigma, isCall);
    for (int i = 0; i < maxIteration; i++)
    {
        if (std::abs(RefOption.BS_Price() - marketPrice) < tol)
        {
            return sigma;
        }
        else
        {
            /*std::cout << RefOption.Vega();*/
            sigma = sigma - (RefOption.BS_Price() - marketPrice) / RefOption.Vega();
            RefOption.SetSigma(sigma);
        }
    }
    return sigma;
}