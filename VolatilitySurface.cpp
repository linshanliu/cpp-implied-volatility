#include "VolatilitySurface.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <cassert>
#include <cstddef>

VolatilitySurface::VolatilitySurface(const std::vector<OptionQuote>& quotes)
{   
	// Check for empty input and throw an exception if no quotes are provided.
    if (quotes.empty())
    {
        throw std::invalid_argument(
            "Cannot build volatility surface from empty quotes."
        );
    }


  
    for (const auto& quote : quotes)
    {
		// Skip any quotes that have non-finite or non-positive values for strike, maturity, or calculated implied volatility.
        if (!std::isfinite(quote.strike) || !std::isfinite(quote.maturity) || !std::isfinite(quote.calculatedIV))
        {
            continue;
        }
        if (quote.strike <= 0.0 || quote.maturity <= 0.0 || quote.calculatedIV <= 0.0)
        {
            continue;
        }

		// Add the valid quote to the appropriate smile vector in the smiles_ map, indexed by maturity.
        smiles_[quote.maturity].push_back(
            SmilePoint{quote.strike, quote.calculatedIV}
        );
    }


	// After processing all quotes, check if any valid smiles were created.
    // If not, throw an exception indicating that no valid quotes were available to build the surface.
    if (smiles_.empty())
    {
        throw std::runtime_error("No valid quotes were available to build the surface.");
    }



    
    for (auto& entry : smiles_)
    {
        std::vector<SmilePoint>& smile = entry.second;

        // Invariant (1): points must be ascending by strike ¡ª GetSmileVol
        // binary-searches this vector.
        std::sort(smile.begin(), smile.end(),
            [](const SmilePoint& lhs, const SmilePoint& rhs)
            {
                return lhs.strike < rhs.strike;
            });

        // Collapse duplicate strikes (call/put, bid/ask, multiple venues) into a
        // single point holding the equal-weighted mean vol. Duplicates are
        // adjacent after the sort, so one linear pass suffices. Without this,
        // LinearInterpolate can be handed x1 == x2 and divide by zero.
        std::vector<SmilePoint> uniqueSmile;
        std::vector<std::size_t> counts;
        uniqueSmile.reserve(smile.size());
        counts.reserve(smile.size());

        for (const SmilePoint& point : smile)
        {
            if (!uniqueSmile.empty() && uniqueSmile.back().strike == point.strike)
            {
                // Incremental mean: keeps every duplicate equally weighted,
                // unlike a running 0.5 * (acc + new).
                double& acc = uniqueSmile.back().impliedVol;
                std::size_t& n = counts.back();
                acc += (point.impliedVol - acc) / static_cast<double>(n + 1);
                ++n;
            }
            else
            {
                uniqueSmile.push_back(point);
                counts.push_back(1);
            }
        }

        smile = std::move(uniqueSmile);

        assert(!smile.empty());
        assert(std::is_sorted(smile.begin(), smile.end(),
            [](const SmilePoint& lhs, const SmilePoint& rhs)
            {
                return lhs.strike < rhs.strike;
            }));
    }
}


double VolatilitySurface::LinearInterpolate(double x, double x1, double y1, double x2, double y2)
{
    if (std::abs(x2 - x1) < 1e-12)
    {
        throw std::runtime_error(
            "Cannot interpolate between identical x values."
        );
    }

    const double weight =
        (x - x1) / (x2 - x1);

    return y1 + weight * (y2 - y1);
}


double VolatilitySurface::GetSmileVol(const std::vector<SmilePoint>& smile, double strike) const
{
	// The smile vector is assumed to be sorted by strike and non-empty, as guaranteed by the constructor.
    if (smile.empty())
    {
        throw std::runtime_error("Cannot interpolate an empty volatility smile.");
    }


	// If the smile contains only one point, we can only return that point's implied volatility if the requested strike matches it. 
    // Otherwise, we throw an exception.
    if (smile.size() == 1)
    {
        if (std::abs(strike - smile.front().strike) < 1e-12)
        {
            return smile.front().impliedVol;
        }

        throw std::out_of_range("Strike lies outside a smile containing only one point.");
    }

	// No extrapolation: reject strikes outside the quoted range.
    if (strike < smile.front().strike || strike > smile.back().strike)
    {
        throw std::out_of_range("Strike lies outside the available smile range.");
    }


	// Use std::lower_bound to find the first smile point with strike >= the requested strike.
    // std::lower_bound(first, last, value, comp)
	// compares the value with the elements in the range [first, last) using the provided comparison function comp.
    auto upper = std::lower_bound(
        smile.begin(),
        smile.end(),
        strike,
        [](const SmilePoint& point, double value)
        {
            return point.strike < value;
        }
    );



    if (upper != smile.end() &&std::abs(upper->strike - strike) < 1e-12)
    {
        return upper->impliedVol;
    }

    if (upper == smile.begin() ||upper == smile.end())
    {
        throw std::runtime_error(
            "Unable to locate strike interpolation interval."
        );
    }

    const auto lower = std::prev(upper);

    return LinearInterpolate(
        strike,
        lower->strike,
        lower->impliedVol,
        upper->strike,
        upper->impliedVol
    );
}

double VolatilitySurface::GetVol(double strike,double maturity) const
{
    if (!std::isfinite(strike) || !std::isfinite(maturity))
    {
        throw std::invalid_argument("Strike and maturity must be finite.");
    }

    if (strike <= 0.0 || maturity <= 0.0)
    {
        throw std::invalid_argument("Strike and maturity must be positive.");
    }

    // No extrapolation: reject maturities outside the quoted range.
    // smiles_ is an ordered map keyed by maturity, so begin()->first is the
    // shortest quoted maturity and rbegin()->first the longest.
    if (maturity < smiles_.begin()->first || maturity > smiles_.rbegin()->first)
    {
        throw std::out_of_range("Maturity lies outside the surface range.");
    }

	// upper is the pointer point to the first smile (a pair) with maturity >= the requested maturity.
    auto upper = smiles_.lower_bound(maturity);


    if (upper != smiles_.end() && std::abs(upper->first - maturity) < 1e-12)
    {
        return GetSmileVol(upper->second,strike);
    }

    if (upper == smiles_.begin() || upper == smiles_.end())
    {
        throw std::runtime_error("Unable to locate maturity interpolation interval.");
    }

    const auto lower = std::prev(upper);

    const double T1 = lower->first;
    const double T2 = upper->first;

    const double sigma1 =
        GetSmileVol(lower->second, strike);

    const double sigma2 =
        GetSmileVol(upper->second, strike);

    const double totalVariance1 =
        sigma1 * sigma1 * T1;

    const double totalVariance2 =
        sigma2 * sigma2 * T2;

    const double interpolatedVariance =
        LinearInterpolate(
            maturity,
            T1,
            totalVariance1,
            T2,
            totalVariance2
        );

    if (interpolatedVariance < 0.0)
    {
        throw std::runtime_error(
            "Interpolated total variance is negative."
        );
    }

    return std::sqrt(
        interpolatedVariance / maturity
    );
}