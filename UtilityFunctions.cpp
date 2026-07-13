#include "EuropeanOption.hpp"
#include "UtilityFunctions.hpp"
#include <iostream>
#include <cmath>


// TODO:
// 1. Add continuous dividend yield q to the Black-Scholes model.
// 2. Update EuropeanOption::BS_Price() and EuropeanOption::Vega() accordingly.
// 3. Re-enable the small-Vega convergence check.
// 4. Re-enable exception handling when Newton-Raphson fails to converge.
// 5. Add no-arbitrage bounds before starting the iteration.



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

    // TODO: Pass dividend yield q once EuropeanOption supports it.
    EuropeanOption RefOption(K, T, r, S0, sigma, isCall);




    for (int i = 0; i < maxIteration; i++)
    {
        double price = RefOption.BS_Price();
        double vega = RefOption.Vega();


        // Temporary debug output. Remove or disable after testing.
        std::cout << "Iteration: " << i
            << ", sigma: " << sigma
            << ", price: " << price
            << ", market price: " << marketPrice
            << ", error: " << std::abs(price - marketPrice)
            << ", vega: " << vega
            << '\n';


        if (std::abs(price - marketPrice) < tol)
        {
            return sigma;
        }


        // TODO: Re-enable this after handling failure properly.
        //if (std::abs(vega) < 1e-8)
        //    break;


        /*std::cout << RefOption.Vega();*/
        sigma -= (price - marketPrice) / vega;


        // Prevent negative volatility during Newton iterations.
        sigma = std::max(sigma, 1e-6);

        RefOption.SetSigma(sigma);
    }


    // TODO: Replace this temporary fallback with an exception
    // or a structured convergence result.
    /*throw std::runtime_error("Implied volatility failed to converge.");*/

    return sigma;
}