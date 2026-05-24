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

    double calculateDailyVolatility(){
        std::vector<double> returns;

        if (prices.size() < 3) {
            return 0.0;
        }

        for (size_t i = 1; i < prices.size(); ++i){
            double daily_return = (prices[i] - prices[i-1]) / prices[i-1];
            returns.push_back(daily_return);
        }

        double returns_sum = 0.0;

        for (size_t i = 0; i < returns.size(); ++i){
            returns_sum += returns[i];
        }

        double daily_mean = returns_sum / returns.size();

        double variance_sum = 0.0;

        for (size_t i = 0; i < returns.size(); ++i){
            variance_sum += (returns[i] - daily_mean) * (returns[i] - daily_mean);
        }

        double variance = variance_sum / (returns.size() - 1);

        return std::sqrt(variance);
    }

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
        return calculateDailyVolatility() * std::sqrt(252);
    }
};

class CryptoAsset : public Asset {
public:
    CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::vector<double>& prices)
        : Asset(ticker, isin, name, prices){}

    double calculateAnnualVolatility() override {
        return calculateDailyVolatility() * std::sqrt(365);
    }
};