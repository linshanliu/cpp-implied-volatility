#ifndef CHF_HPP
#define CHF_HPP

//auto BlackSholesCF = [=](double u)
//    {
//        std::complex<double> i(0.0, 1.0);
//        return std::exp((r - 0.5 * sigma * sigma) * i * u * T
//            - 0.5 * sigma * sigma * u * u * T);
//    };
//
//
//
//auto HestonCF = [=](double u)
//    {
//        using Complex = std::complex<double>;
//        const Complex i(0.0, 1.0);
//
//        const double kappa = 2.0;
//        const double vbar = 0.04;
//        const double sigma = 0.3;   // vol of vol
//        const double rho = -0.7;
//        const double v0 = 0.04;
//
//        Complex iu = i * u;
//
//        Complex d = std::sqrt(
//            std::pow(rho * sigma * iu - kappa, 2.0)
//            + sigma * sigma * (iu + u * u)
//        );
//
//        Complex g = (kappa - rho * sigma * iu - d)
//            / (kappa - rho * sigma * iu + d);
//
//        Complex C =
//            (kappa * vbar / (sigma * sigma)) *
//            ((kappa - rho * sigma * iu - d) * T
//                - 2.0 * std::log((1.0 - g * std::exp(-d * T)) / (1.0 - g)));
//
//        Complex D =
//            ((kappa - rho * sigma * iu - d) / (sigma * sigma)) *
//            ((1.0 - std::exp(-d * T)) / (1.0 - g * std::exp(-d * T)));
//
//        return std::exp(iu * r * T + C + D * v0);
//    };
//
//std::vector<double> HestonCallPrices = COSPrice(HestonCF, true, S0, r, T, strikes, 256, 10.0);
//for (auto p : HestonCallPrices)
//{
//    std::cout << p << std::endl;
//}

#endif // !CHF_HPP
