#ifndef SVISLICE_HPP
#define SVISLICE_HPP

#include <vector>
#include <functional>
#include "OptionQuote.hpp"

//Gatheral, J. (2006).The Volatility Surface : A Practitioner's Guide. Wiley. 
//Gatheral, J.& Jacquier, A. (2014). "Arbitrage-free SVI volatility surfaces." Quantitative Finance, 14(1).
//
//   w(k) = a + b * ( rho*(k - m) + sqrt((k - m)^2 + sigma^2) )
//
// where k = ln(K/F) is log-moneyness (F = forward price for this maturity)
// and w = sigma_BS(k)^2 * T is total implied variance.
//
// Parameter roles (useful for picking initial guesses / sanity-checking a fit):
//   a     : overall vertical level of the curve (a >= 0 keeps w >= 0 near ATM)
//   b     : controls the overall steepness/angle of the two wings (b >= 0)
//   rho   : controls the skew / rotation of the smile, in (-1, 1)
//   m     : horizontal shift of the smile's minimum, in log-moneyness units
//   sigma : controls the curvature/roundedness at the minimum (sigma > 0)
struct SVIParams
{
    double a = 0.0;
    double b = 0.1;
    double rho = 0.0;
    double m = 0.0;
    double sigma = 0.1;
};

// One SVI-parameterised volatility smile, valid for a single fixed maturity T.
//
// Fits the raw SVI total-variance curve to a set of market (strike, IV) quotes
// sharing that maturity, then exposes total-variance / implied-vol lookups
// plus a Durrleman-condition scan for butterfly (vertical spread / convexity)
// arbitrage on the *fitted* curve.
//
// This class only concerns itself with a single maturity. Checking for
// calendar arbitrage requires comparing total variance across two or more
// SVISlice objects at matching log-moneyness and belongs one level up (e.g.
// in VolatilitySurface, once it owns a collection of slices).
class SVISlice
{
private:
    double maturity_;   // T, in years
    double forward_;    // F, forward price used for k = ln(K/F)
    SVIParams params_;
    bool calibrated_ = false;

    // Market calibration targets, cached after Calibrate() so CalibrationRMSE()
    // and diagnostics don't need the original quote list again.
    struct CalibrationPoint
    {
        double k;         // log-moneyness
        double strike;
        double wMarket;   // market total variance = calculatedIV^2 * T
    };
    std::vector<CalibrationPoint> calibrationPoints_;

    // Sum of squared errors between the model's total variance and the
    // market's, evaluated at every calibration point. This is what
    // Calibrate() minimises.
    double CostFunction(const SVIParams& p) const;

    // Unconstrained <-> constrained parameter mapping. Optimising in the
    // unconstrained space (b = exp(x_b), sigma = exp(x_sigma),
    // rho = tanh(x_rho)) means a plain, dependency-free Nelder-Mead search
    // can be used instead of a constrained optimiser.
    static std::vector<double> Transform(const SVIParams& p);
    static SVIParams Untransform(const std::vector<double>& x);

    // Minimal dependency-free Nelder-Mead simplex search. `costFn` is
    // evaluated in the unconstrained parameter space produced by Transform().
    static std::vector<double> NelderMead(
        std::vector<double> x0,
        const std::function<double(const std::vector<double>&)>& costFn,
        int maxIterations = 3000,
        double tol = 1e-14);

public:
    SVISlice(double maturity, double forward);

    // Calibrates the SVI curve to every quote in `quotes` whose maturity is
    // within `maturityTol` of this slice's maturity (a tolerance is needed
    // since maturities read off a CSV are floating point) and whose implied
    // vol is usable. Prefers quote.calculatedIV, falling back to
    // quote.yahooIV if calculatedIV is not set (<= 0). Quotes with
    // valid == false are skipped.
    //
    // Throws std::invalid_argument if fewer than 5 usable quotes are found
    // (SVI has 5 free parameters; fewer points than that is underdetermined).
    void Calibrate(const std::vector<OptionQuote>& quotes, double maturityTol = 1e-6);

    double GetMaturity() const;
    double GetForward() const;
    const SVIParams& GetParams() const;
    bool IsCalibrated() const;

    // Total variance w(k) = a + b*(rho*(k-m) + sqrt((k-m)^2+sigma^2))
    // Requires IsCalibrated() (or a params_ set some other way); throws
    // std::logic_error otherwise.
    double TotalVariance(double logMoneyness) const;

    // Black-Scholes implied vol at a given strike: sqrt(w(k)/T), k = ln(K/F).
    double ImpliedVol(double strike) const;

    // Root-mean-square calibration error, expressed in implied-vol terms
    // (not total variance), for judging fit quality against the quotes
    // passed to Calibrate().
    double CalibrationRMSE() const;

    struct ButterflyCheckResult
    {
        bool arbitrageFree;
        double worstG;   // minimum g(k) found over the scanned range; should be >= -tol
        double worstK;   // log-moneyness at which worstG occurs
    };

    // Durrleman (2004) condition: the fitted smile is free of butterfly
    // (price-convexity) arbitrage over the scanned range if and only if
    //
    //   g(k) = (1 - k*w'(k)/(2w(k)))^2 - (w'(k)^2/4)*(1/w(k) + 1/4) + w''(k)/2 >= 0
    //
    // for every k. This is evaluated analytically from the fitted SVI curve
    // (no finite differencing / interpolation involved), so it isn't
    // sensitive to the grid-noise issues that come from checking convexity
    // on a numerically-interpolated price surface.
    ButterflyCheckResult CheckButterflyArbitrage(
        int numPoints = 200, double kMin = -2.0, double kMax = 2.0, double tol = 1e-8) const;

    // Necessary (not sufficient) condition for a single SVI slice to stay
    // arbitrage-free as k -> +/-infinity: b*(1 + |rho|) <= 4/T. Cheap to
    // check and worth running before/alongside the full Durrleman scan.
    bool SatisfiesLargeMoneynessCondition() const;
};

#endif // !SVISLICE_HPP