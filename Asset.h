#ifndef ASSET_H  
#define ASSET_H

#include <string>
#include <vector>
#include <cmath>

class Asset{
protected:
    std::string ticker;
    std::string isin;
    std::string name;
    std::vector<double> prices;

    double calculateDailyMeanReturn();
    double calculateDailyVolatility();

public:
    Asset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : ticker(ticker), isin(isin), name(name), prices(prices) {}

    virtual ~Asset() = default;

    virtual double calculateAnnualVolatility() = 0;
    virtual double calculateAnnualExpectedReturn() = 0; 

    double calculateSharpeRatio(double risk_free_rate);
    double calculateKellyCriterion(double risk_free_rate);
};

class TraditionalAsset : public Asset {
public:
    TraditionalAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : Asset(ticker, isin, name, prices){}

    double calculateAnnualVolatility() override;
    double calculateAnnualExpectedReturn() override;
};

class CryptoAsset : public Asset {
public:
    CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : Asset(ticker, isin, name, prices){}

    double calculateAnnualVolatility() override;
    double calculateAnnualExpectedReturn() override;
};

#endif