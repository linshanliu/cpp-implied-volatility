#ifndef EUROPEANOPTION_hpp
#define EUROPEANOPTION_hpp

class EuropeanOption
{
private:
	double K;
	double T;
	double r;
	double S0;
	double sigma;
	bool call_put; // call_put ==1 if it is a call, else put

public:
	EuropeanOption();
	EuropeanOption(double K_in, double T_in, double r_in, double S0_in, double sigma_in, bool call_put);
	EuropeanOption(const EuropeanOption& source);
	~EuropeanOption();

	double BS_Price() const;
	double Vega() const;

	void SetK(double K_in);
	void SetT(double T_in);
	void SetSigma(double sigma_in);



};

#endif // !EUROPEANOPTION_hpp
