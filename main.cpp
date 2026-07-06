

//输入网络上的期权数据
// 计算Implied volatility
//       BS 模型的反函数
// 然后看一下是否有 volatility smile/skew
// calibration Heston model
// 对新的期权生成价格

#include "EuropeanOption.hpp"
#include "UtilityFunctions.hpp"
#include "SPYOptionData.hpp"
#include "COSPricer.hpp"
#include <iostream>
#include <vector>

int main()
{
    //EuropeanOption Option1(200.,1.,0.04,182.,0.25, true);
    //std::cout << "The option price via BS formula is:" << Option1.BS_Price() << std::endl;
    ///*std::cout << Option1.Vega() << std::endl;*/
    //std::cout <<  "The implied volatility with market price 13.996 is:" << ImpliedVolatility(182,200,0.04,1,13.996,1,100,1e-6) << std::endl;


    //EuropeanOption Option2(212.5, 0.027, 0.0375, 185.6, 0.41, true);
    //std::cout << Option2.Vega();
    //std::cout << "The option price via BS formula is:" << Option2.BS_Price() << std::endl;
    //std::cout << "The implied volatility from NVIDIA option= 0.15 ,S0=185.65 is:" << ImpliedVolatility(185.75, 200, 0.0375, 0.03013, 0.88, 1, 100, 1e-10) << std::endl;


   /* std::vector<double> strikes = { 80,90,100,110,120 };
    std::vector<double> prices = { 24.5888,16.6994,10.4506,6.0401,3.2475 };

    for (int i = 0; i < strikes.size(); i++)
    {
        double iv = ImpliedVolatility(
            100,
            strikes[i],
            0.05,
            1,
            prices[i],
            true,
            100,
            1e-6
        );

        std::cout << strikes[i] << "  " << iv << std::endl;
    }*/

    


    for (const auto& q : SPYCall)
    {
        double iv = ImpliedVolatility(
            677.18,
            q.K,
            0.0375,
            q.T,
            q.mid,
            q.isCall,
            100,
            1e-6
        );

        std::cout << q.symbol
            << "  K=" << q.K
            << "  T=" << q.T
            << "  mid=" << q.mid
            << "  IV=" << iv
            << std::endl;
    }


    // COS
    double sigma = 0.2;
    double r = 0.05;
    double T = 1.0;
    double S0 = 100.0;

    auto BlackSholesCF = [=](double u)
        {
            std::complex<double> i(0.0, 1.0);
            return std::exp((r - 0.5 * sigma * sigma) * i * u * T
                - 0.5 * sigma * sigma * u * u * T);
        };



    std::vector<double> strikes = { 80.0, 90.0, 100.0, 110.0, 120.0 };

    std::vector<double> callPrices = COSPrice(BlackSholesCF, true, S0, r, T, strikes, 256, 10.0);
    std::vector<double> putPrices = COSPrice(BlackSholesCF, false, S0, r, T, strikes, 256, 10.0);

    std::cout << "Strike | COS Price | BS Price | Difference" << std::endl;

    for (size_t i = 0; i < strikes.size(); ++i)
    {
        double K = strikes[i];

        EuropeanOption optionRef(K, T, r, S0, sigma, true);
        double bsPrice = optionRef.BS_Price();

        double cosPrice = callPrices[i];

        double diff = cosPrice - bsPrice;

        std::cout << K << " | "
            << cosPrice << " | "
            << bsPrice << " | "
            << diff << std::endl;
    }


    auto HestonCF = [=](double u)
        {
            using Complex = std::complex<double>;
            const Complex i(0.0, 1.0);

            const double kappa = 2.0;
            const double vbar = 0.04;
            const double sigma = 0.3;   // vol of vol
            const double rho = -0.7;
            const double v0 = 0.04;

            Complex iu = i * u;

            Complex d = std::sqrt(
                std::pow(rho * sigma * iu - kappa, 2.0)
                + sigma * sigma * (iu + u * u)
            );

            Complex g = (kappa - rho * sigma * iu - d)
                / (kappa - rho * sigma * iu + d);

            Complex C =
                (kappa * vbar / (sigma * sigma)) *
                ((kappa - rho * sigma * iu - d) * T
                    - 2.0 * std::log((1.0 - g * std::exp(-d * T)) / (1.0 - g)));

            Complex D =
                ((kappa - rho * sigma * iu - d) / (sigma * sigma)) *
                ((1.0 - std::exp(-d * T)) / (1.0 - g * std::exp(-d * T)));

            return std::exp(iu * r * T + C + D * v0);
        };

    std::vector<double> HestonCallPrices = COSPrice(HestonCF, true, S0, r, T, strikes, 256, 10.0);
    for (auto p : HestonCallPrices)
    {
        std::cout << p << std::endl;
    }
}
