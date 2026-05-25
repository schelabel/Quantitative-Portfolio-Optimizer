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
    std::vector<std::string> dates;
    std::vector<double> log_returns;

    void calculateLogReturns(); 
    double calculateDailyMeanReturn() const; 
    double calculateDailyVolatility() const; 

public:
    Asset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices, const std::vector<std::string>& dates)
        : ticker(ticker), isin(isin), name(name), currency(currency), prices(prices), dates(dates) {}

    virtual ~Asset() = default;

    virtual double calculateAnnualVolatility() const = 0;
    virtual double calculateAnnualExpectedReturn() const = 0; 

    double calculateSharpeRatio(double risk_free_rate) const;

    std::string getTicker() const { return ticker; }
    std::string getName() const { return name; }
    std::string getCurrency() const { return currency; }
    
    const std::vector<double>& getLogReturns() const { return log_returns; } 
    const std::vector<double>& getPrices() const { return prices; }
    const std::vector<std::string>& getDates() const { return dates; }
};

class TraditionalAsset : public Asset {
public:
    TraditionalAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices, const std::vector<std::string>& dates);

    double calculateAnnualVolatility() const override;
    double calculateAnnualExpectedReturn() const override;
};

class CryptoAsset : public Asset {
public:
    CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices, const std::vector<std::string>& dates);

    double calculateAnnualVolatility() const override;
    double calculateAnnualExpectedReturn() const override;
};

#endif