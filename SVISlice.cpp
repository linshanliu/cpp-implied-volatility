#include "SVISlice.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <limits>

namespace
{
    // Evaluates w(k), w'(k), w''(k) for the raw SVI parameterisation at once,
    // since the Durrleman check needs all three at the same k.
    struct SVIDerivs
    {
        double w;
        double wPrime;
        double wDoublePrime;
    };

    SVIDerivs EvaluateWithDerivatives(const SVIParams& p, double k)
    {
        const double x = k - p.m;
        const double s = std::sqrt(x * x + p.sigma * p.sigma);

        SVIDerivs out;
        out.w = p.a + p.b * (p.rho * x + s);
        out.wPrime = p.b * (p.rho + x / s);
        out.wDoublePrime = p.b * p.sigma * p.sigma / (s * s * s);
        return out;
    }
}

SVISlice::SVISlice(double maturity, double forward)
    : maturity_(maturity), forward_(forward)
{
}

double SVISlice::CostFunction(const SVIParams& p) const
{
    double sse = 0.0;
    for (const auto& pt : calibrationPoints_)
    {
        const double wModel = EvaluateWithDerivatives(p, pt.k).w;
        const double diff = wModel - pt.wMarket;
        sse += diff * diff;
    }
    return sse;
}

std::vector<double> SVISlice::Transform(const SVIParams& p)
{
    // b, sigma > 0 via log; rho in (-1,1) via atanh. Clamp rho away from the
    // +/-1 boundary first so atanh doesn't blow up on a fresh/edge guess.
    const double rhoClamped = std::max(-0.999, std::min(0.999, p.rho));
    return {
        p.a,
        std::log(std::max(p.b, 1e-8)),
        std::atanh(rhoClamped),
        p.m,
        std::log(std::max(p.sigma, 1e-8))
    };
}

SVIParams SVISlice::Untransform(const std::vector<double>& x)
{
    SVIParams p;
    p.a = x[0];
    p.b = std::exp(x[1]);
    p.rho = std::tanh(x[2]);
    p.m = x[3];
    p.sigma = std::exp(x[4]);
    return p;
}

std::vector<double> SVISlice::NelderMead(
    std::vector<double> x0,
    const std::function<double(const std::vector<double>&)>& costFn,
    int maxIterations,
    double tol)
{
    const size_t n = x0.size();
    const double reflect = 1.0, expand = 2.0, contract = 0.5, shrink = 0.5;

    // Build the initial simplex: x0 plus n perturbed points, one per axis.
    std::vector<std::vector<double>> simplex(n + 1, x0);
    for (size_t i = 0; i < n; ++i)
    {
        const double step = (std::abs(x0[i]) > 1e-8) ? 0.1 * x0[i] : 0.1;
        simplex[i + 1][i] += step;
    }

    std::vector<double> cost(n + 1);
    for (size_t i = 0; i <= n; ++i)
        cost[i] = costFn(simplex[i]);

    for (int iter = 0; iter < maxIterations; ++iter)
    {
        // Sort simplex vertices by cost, ascending (best first).
        std::vector<size_t> order(n + 1);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&cost](size_t a, size_t b) { return cost[a] < cost[b]; });

        std::vector<std::vector<double>> sortedSimplex(n + 1);
        std::vector<double> sortedCost(n + 1);
        for (size_t i = 0; i <= n; ++i)
        {
            sortedSimplex[i] = simplex[order[i]];
            sortedCost[i] = cost[order[i]];
        }
        simplex = sortedSimplex;
        cost = sortedCost;

        if (std::abs(cost[n] - cost[0]) < tol)
            break;

        // Centroid of all points except the worst.
        std::vector<double> centroid(n, 0.0);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = 0; j < n; ++j)
                centroid[j] += simplex[i][j];
        for (size_t j = 0; j < n; ++j)
            centroid[j] /= static_cast<double>(n);

        auto pointAt = [&](double factor) {
            std::vector<double> pt(n);
            for (size_t j = 0; j < n; ++j)
                pt[j] = centroid[j] + factor * (centroid[j] - simplex[n][j]);
            return pt;
            };

        std::vector<double> xr = pointAt(reflect);
        const double fr = costFn(xr);

        if (fr < cost[0])
        {
            std::vector<double> xe = pointAt(expand);
            const double fe = costFn(xe);
            if (fe < fr) { simplex[n] = xe; cost[n] = fe; }
            else { simplex[n] = xr; cost[n] = fr; }
        }
        else if (fr < cost[n - 1])
        {
            simplex[n] = xr;
            cost[n] = fr;
        }
        else
        {
            std::vector<double> xc = pointAt(-contract);
            const double fc = costFn(xc);
            if (fc < cost[n])
            {
                simplex[n] = xc;
                cost[n] = fc;
            }
            else
            {
                // Shrink the whole simplex toward the best point.
                for (size_t i = 1; i <= n; ++i)
                {
                    for (size_t j = 0; j < n; ++j)
                        simplex[i][j] = simplex[0][j] + shrink * (simplex[i][j] - simplex[0][j]);
                    cost[i] = costFn(simplex[i]);
                }
            }
        }
    }

    // Return the best vertex found.
    size_t best = 0;
    for (size_t i = 1; i <= n; ++i)
        if (cost[i] < cost[best]) best = i;
    return simplex[best];
}

void SVISlice::Calibrate(const std::vector<OptionQuote>& quotes, double maturityTol)
{
    calibrationPoints_.clear();

    for (const auto& q : quotes)
    {
        if (!q.valid) continue;
        if (std::abs(q.maturity - maturity_) > maturityTol) continue;

        const double iv = (q.calculatedIV > 0.0) ? q.calculatedIV : q.yahooIV;
        if (iv <= 0.0 || q.strike <= 0.0) continue;

        CalibrationPoint pt;
        pt.strike = q.strike;
        pt.k = std::log(q.strike / forward_);
        pt.wMarket = iv * iv * maturity_;
        calibrationPoints_.push_back(pt);
    }

    if (calibrationPoints_.size() < 5)
    {
        throw std::invalid_argument(
            "SVISlice::Calibrate: need at least 5 usable quotes to fit 5 SVI "
            "parameters, found " + std::to_string(calibrationPoints_.size()));
    }

    // Initial guess: a at roughly half the smallest observed total variance
    // (SVI's minimum is close to a when sigma is small), a small positive b,
    // a mild negative skew (typical for equity/equity-like underlyings),
    // m at the ATM point, sigma at a fraction of the k-range.
    const double wMin = std::min_element(
        calibrationPoints_.begin(), calibrationPoints_.end(),
        [](const CalibrationPoint& x, const CalibrationPoint& y) { return x.wMarket < y.wMarket; })->wMarket;

    double kMin = calibrationPoints_.front().k, kMax = calibrationPoints_.front().k;
    for (const auto& pt : calibrationPoints_)
    {
        kMin = std::min(kMin, pt.k);
        kMax = std::max(kMax, pt.k);
    }

    SVIParams init;
    init.a = 0.5 * wMin;
    init.b = 0.1;
    init.rho = -0.3;
    init.m = 0.0;
    init.sigma = std::max(0.05, 0.25 * (kMax - kMin));

    const std::function<double(const std::vector<double>&)> cost =
        [this](const std::vector<double>& x) { return CostFunction(Untransform(x)); };

    std::vector<double> best = NelderMead(Transform(init), cost);
    params_ = Untransform(best);
    calibrated_ = true;
}

double SVISlice::GetMaturity() const { return maturity_; }
double SVISlice::GetForward() const { return forward_; }
const SVIParams& SVISlice::GetParams() const { return params_; }
bool SVISlice::IsCalibrated() const { return calibrated_; }

double SVISlice::TotalVariance(double logMoneyness) const
{
    if (!calibrated_)
        throw std::logic_error("SVISlice::TotalVariance: slice has not been calibrated");
    return EvaluateWithDerivatives(params_, logMoneyness).w;
}

double SVISlice::ImpliedVol(double strike) const
{
    const double k = std::log(strike / forward_);
    const double w = TotalVariance(k);
    return std::sqrt(std::max(w, 0.0) / maturity_);
}

double SVISlice::CalibrationRMSE() const
{
    if (calibrationPoints_.empty()) return std::numeric_limits<double>::quiet_NaN();

    double sse = 0.0;
    for (const auto& pt : calibrationPoints_)
    {
        const double wModel = TotalVariance(pt.k);
        const double ivModel = std::sqrt(std::max(wModel, 0.0) / maturity_);
        const double ivMarket = std::sqrt(pt.wMarket / maturity_);
        const double diff = ivModel - ivMarket;
        sse += diff * diff;
    }
    return std::sqrt(sse / static_cast<double>(calibrationPoints_.size()));
}

SVISlice::ButterflyCheckResult SVISlice::CheckButterflyArbitrage(
    int numPoints, double kMin, double kMax, double tol) const
{
    if (!calibrated_)
        throw std::logic_error("SVISlice::CheckButterflyArbitrage: slice has not been calibrated");

    ButterflyCheckResult result;
    result.worstG = std::numeric_limits<double>::infinity();
    result.worstK = kMin;

    for (int i = 0; i < numPoints; ++i)
    {
        const double k = kMin + (kMax - kMin) * static_cast<double>(i) / static_cast<double>(numPoints - 1);
        const SVIDerivs d = EvaluateWithDerivatives(params_, k);

        double g;
        if (d.w <= 0.0)
        {
            // A non-positive total variance is itself degenerate/arbitrageable;
            // flag it as a hard violation rather than dividing by w.
            g = -1.0;
        }
        else
        {
            const double term1 = 1.0 - (k * d.wPrime) / (2.0 * d.w);
            const double term2 = (d.wPrime * d.wPrime) / 4.0 * (1.0 / d.w + 0.25);
            g = term1 * term1 - term2 + d.wDoublePrime / 2.0;
        }

        if (g < result.worstG)
        {
            result.worstG = g;
            result.worstK = k;
        }
    }

    result.arbitrageFree = (result.worstG >= -tol);
    return result;
}

bool SVISlice::SatisfiesLargeMoneynessCondition() const
{
    if (!calibrated_)
        throw std::logic_error("SVISlice::SatisfiesLargeMoneynessCondition: slice has not been calibrated");
    return params_.b * (1.0 + std::abs(params_.rho)) <= 4.0 / maturity_ + 1e-12;
}