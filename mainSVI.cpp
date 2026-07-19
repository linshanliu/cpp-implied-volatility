#include "SVISlice.hpp"
#include "OptionQuote.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <cmath>
#include <iomanip>



static std::vector<OptionQuote> LoadQuotes(const std::string& path)
{
    std::vector<OptionQuote> quotes;
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("could not open " + path);

    std::string line;
    std::getline(file, line); // skip header

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string cell;
        OptionQuote q;

        std::getline(ss, cell, ','); q.strike = std::stod(cell);
        std::getline(ss, cell, ','); q.maturity = std::stod(cell);
        std::getline(ss, cell, ','); q.mid = std::stod(cell);
        std::getline(ss, cell, ','); q.isCall = std::stoi(cell) != 0;
        std::getline(ss, cell, ','); q.calculatedIV = std::stod(cell);

        q.valid = true;
        quotes.push_back(q);
    }
    return quotes;
}

// --- Forward estimation per maturity via put-call parity ----------------
// F = C - P + K, averaged over strikes where both a call and a put quote
// exist at that maturity. Same approach used earlier when looking for
// arbitrage in the raw quotes.
static std::map<double, double> EstimateForwards(const std::vector<OptionQuote>& quotes)
{
    std::map<double, std::vector<std::pair<double, double> > > calls, puts; // T -> (K, mid)
    for (size_t i = 0; i < quotes.size(); ++i)
    {
        const OptionQuote& q = quotes[i];
        if (q.isCall) calls[q.maturity].push_back(std::make_pair(q.strike, q.mid));
        else          puts[q.maturity].push_back(std::make_pair(q.strike, q.mid));
    }

    std::map<double, double> forwards;
    for (std::map<double, std::vector<std::pair<double, double> > >::const_iterator ct = calls.begin();
        ct != calls.end(); ++ct)
    {
        const double T = ct->first;
        const std::vector<std::pair<double, double> >& callList = ct->second;

        std::map<double, std::vector<std::pair<double, double> > >::const_iterator pt = puts.find(T);
        if (pt == puts.end()) continue;
        const std::vector<std::pair<double, double> >& putList = pt->second;

        double sum = 0.0;
        int n = 0;
        for (size_t i = 0; i < callList.size(); ++i)
        {
            const double Kc = callList[i].first, Cc = callList[i].second;
            for (size_t j = 0; j < putList.size(); ++j)
            {
                const double Kp = putList[j].first, Pp = putList[j].second;
                if (std::abs(Kc - Kp) < 1e-9)
                {
                    sum += (Cc - Pp + Kc);
                    ++n;
                }
            }
        }

        if (n > 0) forwards[T] = sum / n;
    }
    return forwards;
}

int main(int argc, char** argv)
{
    const std::string path = (argc > 1) ? argv[1] : "SPCX_options_IV.csv";

    std::vector<OptionQuote> quotes = LoadQuotes(path);
    std::cout << "Loaded " << quotes.size() << " quotes from " << path << "\n\n";

    std::map<double, double> forwards = EstimateForwards(quotes);

    std::cout << std::fixed << std::setprecision(4);
    std::cout << std::left
        << std::setw(8) << "T"
        << std::setw(10) << "F"
        << std::setw(10) << "RMSE"
        << std::setw(10) << "b"
        << std::setw(10) << "rho"
        << std::setw(10) << "sigma"
        << std::setw(12) << "arb_free"
        << std::setw(10) << "worstG"
        << "\n";
    std::cout << std::string(84, '-') << "\n";

    for (std::map<double, double>::const_iterator it = forwards.begin(); it != forwards.end(); ++it)
    {
        const double T = it->first;
        const double F = it->second;

        SVISlice slice(T, F);
        try
        {
            slice.Calibrate(quotes);
        }
        catch (const std::exception& e)
        {
            std::cout << "T=" << T << ": skipped calibration (" << e.what() << ")\n";
            continue;
        }

        const SVIParams& p = slice.GetParams();
        const SVISlice::ButterflyCheckResult bf = slice.CheckButterflyArbitrage();

        std::cout << std::setw(8) << T
            << std::setw(10) << F
            << std::setw(10) << slice.CalibrationRMSE()
            << std::setw(10) << p.b
            << std::setw(10) << p.rho
            << std::setw(10) << p.sigma
            << std::setw(12) << (bf.arbitrageFree ? "yes" : "NO")
            << std::setw(10) << bf.worstG
            << "\n";
    }

    return 0;
}