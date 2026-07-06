#ifndef UTILITYFUNCTIONS_HPP
#define UTILITYFUNCTIONS_HPP

double ImpliedVolatility(double S0,
    double K,
    double r,
    double T,
    double marketPrice,
    bool isCall,
    int maxIteration = 100,
    double tol = 1e-10);



#endif // !UTILITYFUNCTIONS_HPP
