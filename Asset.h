#include <string>
#include <vector>
#include <cmath>

class Asset{
protected:
    std::string ticker;
    std::string isin;
    std::string name;
    std::vector<double> prices;
    
    double mu = 0.0;
    double stdev = 0.0;
    double sharpe = 0.0;
    double kelly = 0.0;

public:
    Asset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : ticker(ticker), isin(isin), name(name), prices(prices) {}

    virtual ~Asset() = default;

    virtual double calculateAnnualVolatility() = 0;
    
};

class TraditionalAsset : public Asset {
public:
    TraditionalAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : Asset(ticker, isin, name, prices){}

    double calculateAnnualVolatility() override {

        return 0.0;
    }
};

class CryptoAsset : public Asset {
public:
    CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : Asset(ticker, isin, name, prices){}

    double calculateAnnualVolatility() override {

        return 0.0;
    }
};