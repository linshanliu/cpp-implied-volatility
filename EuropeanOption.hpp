#ifndef EUROPEANOPTION_hpp
#define EUROPEANOPTION_hpp

class EuropeanOption
{
private:
	double K_;
	double T_;
	bool isCall_;

public:
	EuropeanOption();
	EuropeanOption(double K, double T, bool isCall);
	EuropeanOption(const EuropeanOption& source);
	~EuropeanOption();



	double BS_Price(double S0, double r, double sigma, double q = 0.0) const;
	double Vega(double S0, double r, double sigma, double q = 0.0) const;

	void SetK(double K);
	void SetT(double T);

	double ImpliedVolatility(double S0, double r, double q, double marketPrice, int maxIteration = 100, double tol = 1e-10);



};

#endif // !EUROPEANOPTION_hpp
