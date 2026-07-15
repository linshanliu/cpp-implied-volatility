//#include "VolatilitySurface.hpp"
//
//#include <algorithm>
//#include <cmath>
//#include <stdexcept>
//
//VolatilitySurface::VolatilitySurface(
//    const std::vector<OptionQuote>& quotes)
//{
//    if (quotes.empty())
//    {
//        throw std::invalid_argument(
//            "Cannot build volatility surface from empty quotes."
//        );
//    }
//
//    for (const auto& quote : quotes)
//    {
//        if (!std::isfinite(quote.strike) ||
//            !std::isfinite(quote.maturity) ||
//            !std::isfinite(quote.calculatedIV))
//        {
//            continue;
//        }
//
//        if (quote.strike <= 0.0 ||
//            quote.maturity <= 0.0 ||
//            quote.calculatedIV <= 0.0)
//        {
//            continue;
//        }
//
//        smiles_[quote.maturity].push_back(
//            SmilePoint{
//                quote.strike,
//                quote.calculatedIV
//            }
//        );
//    }
//
//    if (smiles_.empty())
//    {
//        throw std::runtime_error(
//            "No valid quotes were available to build the surface."
//        );
//    }
//
//    for (auto& [maturity, smile] : smiles_)
//    {
//        std::sort(
//            smile.begin(),
//            smile.end(),
//            [](const SmilePoint& lhs,
//                const SmilePoint& rhs)
//            {
//                return lhs.strike < rhs.strike;
//            }
//        );
//
//        std::vector<SmilePoint> uniqueSmile;
//        uniqueSmile.reserve(smile.size());
//
//        for (const auto& point : smile)
//        {
//            if (!uniqueSmile.empty() &&
//                std::abs(
//                    uniqueSmile.back().strike -
//                    point.strike
//                ) < 1e-12)
//            {
//                uniqueSmile.back().impliedVol =
//                    0.5 *
//                    (
//                        uniqueSmile.back().impliedVol +
//                        point.impliedVol
//                        );
//            }
//            else
//            {
//                uniqueSmile.push_back(point);
//            }
//        }
//
//        smile = std::move(uniqueSmile);
//    }
//}
//
//
//double VolatilitySurface::LinearInterpolate(
//    double x,
//    double x1,
//    double y1,
//    double x2,
//    double y2)
//{
//    if (std::abs(x2 - x1) < 1e-12)
//    {
//        throw std::runtime_error(
//            "Cannot interpolate between identical x values."
//        );
//    }
//
//    const double weight =
//        (x - x1) / (x2 - x1);
//
//    return y1 + weight * (y2 - y1);
//}
//
//
//double VolatilitySurface::GetSmileVol(
//    const std::vector<SmilePoint>& smile,
//    double strike) const
//{
//    if (smile.empty())
//    {
//        throw std::runtime_error(
//            "Cannot interpolate an empty volatility smile."
//        );
//    }
//
//    if (smile.size() == 1)
//    {
//        if (std::abs(strike - smile.front().strike) < 1e-12)
//        {
//            return smile.front().impliedVol;
//        }
//
//        throw std::out_of_range(
//            "Strike lies outside a smile containing only one point."
//        );
//    }
//
//    if (strike < smile.front().strike ||
//        strike > smile.back().strike)
//    {
//        throw std::out_of_range(
//            "Strike lies outside the available smile range."
//        );
//    }
//
//    auto upper = std::lower_bound(
//        smile.begin(),
//        smile.end(),
//        strike,
//        [](const SmilePoint& point, double value)
//        {
//            return point.strike < value;
//        }
//    );
//
//    if (upper != smile.end() &&
//        std::abs(upper->strike - strike) < 1e-12)
//    {
//        return upper->impliedVol;
//    }
//
//    if (upper == smile.begin() ||
//        upper == smile.end())
//    {
//        throw std::runtime_error(
//            "Unable to locate strike interpolation interval."
//        );
//    }
//
//    const auto lower = std::prev(upper);
//
//    return LinearInterpolate(
//        strike,
//        lower->strike,
//        lower->impliedVol,
//        upper->strike,
//        upper->impliedVol
//    );
//}
//
//double VolatilitySurface::GetVol(
//    double strike,
//    double maturity) const
//{
//    if (!std::isfinite(strike) ||
//        !std::isfinite(maturity))
//    {
//        throw std::invalid_argument(
//            "Strike and maturity must be finite."
//        );
//    }
//
//    if (strike <= 0.0 ||
//        maturity <= 0.0)
//    {
//        throw std::invalid_argument(
//            "Strike and maturity must be positive."
//        );
//    }
//
//    if (maturity < smiles_.begin()->first ||
//        maturity > smiles_.rbegin()->first)
//    {
//        throw std::out_of_range(
//            "Maturity lies outside the surface range."
//        );
//    }
//
//    auto upper = smiles_.lower_bound(maturity);
//
//    if (upper != smiles_.end() &&
//        std::abs(upper->first - maturity) < 1e-12)
//    {
//        return GetSmileVol(
//            upper->second,
//            strike
//        );
//    }
//
//    if (upper == smiles_.begin() ||
//        upper == smiles_.end())
//    {
//        throw std::runtime_error(
//            "Unable to locate maturity interpolation interval."
//        );
//    }
//
//    const auto lower = std::prev(upper);
//
//    const double T1 = lower->first;
//    const double T2 = upper->first;
//
//    const double sigma1 =
//        GetSmileVol(lower->second, strike);
//
//    const double sigma2 =
//        GetSmileVol(upper->second, strike);
//
//    const double totalVariance1 =
//        sigma1 * sigma1 * T1;
//
//    const double totalVariance2 =
//        sigma2 * sigma2 * T2;
//
//    const double interpolatedVariance =
//        LinearInterpolate(
//            maturity,
//            T1,
//            totalVariance1,
//            T2,
//            totalVariance2
//        );
//
//    if (interpolatedVariance < 0.0)
//    {
//        throw std::runtime_error(
//            "Interpolated total variance is negative."
//        );
//    }
//
//    return std::sqrt(
//        interpolatedVariance / maturity
//    );
//}