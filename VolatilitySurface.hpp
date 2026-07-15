//#ifndef VOLATILITYSURFACE_HPP
//#define VOLATILITYSURFACE_HPP
//
//#include <map>
//#include <vector>
//
//#include "OptionQuote.hpp"
//
//class VolatilitySurface
//{
//public:
//    explicit VolatilitySurface(
//        const std::vector<OptionQuote>& quotes
//    );
//
//    double GetVol(
//        double strike,
//        double maturity
//    ) const;
//
//private:
//    struct SmilePoint
//    {
//        double strike;
//        double impliedVol;
//    };
//
//    std::map<double, std::vector<SmilePoint>> smiles_;
//
//    double GetSmileVol(
//        const std::vector<SmilePoint>& smile,
//        double strike
//    ) const;
//
//    static double LinearInterpolate(
//        double x,
//        double x1,
//        double y1,
//        double x2,
//        double y2
//    );
//};
//
//#endif // !VOLATILITYSURFACE_HPP
