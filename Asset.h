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
    std::string currency;
    std::vector<double> prices;

    double calculateDailyMeanReturn();
    double calculateDailyVolatility();

public:
    Asset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices)
        : ticker(ticker), isin(isin), name(name), currency(currency), prices(prices) {}

    virtual ~Asset() = default;

    virtual double calculateAnnualVolatility() = 0;
    virtual double calculateAnnualExpectedReturn() = 0; 

    double calculateSharpeRatio(double risk_free_rate);
    double calculateKellyCriterion(double risk_free_rate);

    std::string getTicker() const { return ticker; }
    std::string getName() const { return name; }
    std::string getCurrency() const { return currency; }
};

class TraditionalAsset : public Asset {
public:
    TraditionalAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices)
        : Asset(ticker, isin, name, currency, prices){}

    double calculateAnnualVolatility() override;
    double calculateAnnualExpectedReturn() override;
};

class CryptoAsset : public Asset {
public:
    CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices)
        : Asset(ticker, isin, name, currency, prices){}

    double calculateAnnualVolatility() override;
    double calculateAnnualExpectedReturn() override;
};

#endif