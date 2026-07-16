#ifndef VOLATILITYSURFACE_HPP
#define VOLATILITYSURFACE_HPP

#include <map>
#include <vector>

#include "OptionQuote.hpp"

class VolatilitySurface
{
private:
    // A single point on the volatility surface: the implied vol observed
    // at a given strike, for one fixed maturity.
    struct SmilePoint{double strike; double impliedVol;};


    // The full volatility surface, stored as maturity-indexed slices.
    //   key   : time to maturity T, in years (e.g. 0.25 == 3 months)
    //   value : the volatility smile for that maturity, i.e. the list of
    //           (strike, implied vol) points quoted at T.
    //
    // std::map rather than unordered_map: GetVol needs the maturities in
    // sorted order so it can lower_bound to the two bracketing smiles and
    // interpolate along the time axis.
    //
    // Invariants (established by the constructor):
    //   1) Points within each vector are sorted by strike, strictly ascending.
    //   2) No vector is empty.
    // GetSmileVol relies on both; violating them is undefined behaviour.
    std::map<double, std::vector<SmilePoint>> smiles_;

    double GetSmileVol(const std::vector<SmilePoint>& smile, double strike) const;

    static double LinearInterpolate(double x, double x1, double y1, double x2, double y2);

public:
    explicit VolatilitySurface(const std::vector<OptionQuote>& quotes);

    double GetVol(double strike, double maturity) const;
};

#endif // !VOLATILITYSURFACE_HPP
