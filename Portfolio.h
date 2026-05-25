#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "Asset.h"
#include <vector>
#include <string>

class Portfolio {
private:
    std::vector<Asset*> assets;
    std::vector<double> weights;

    std::vector<std::string> common_dates;
    
    std::vector<std::vector<double>> aligned_log_returns; 
    
    std::vector<std::vector<double>> cov_matrix;

    void synchronizeData();
    void calculateCovarianceMatrix();

public:
    Portfolio(const std::vector<Asset*>& assets, const std::vector<double>& weights);

    double calculateExpectedReturn() const;
    double calculateVolatility() const; 
    
    double getKellyCriterion(double risk_free_rate) const;
    double getGeometricMean(double risk_free_rate) const;

    const std::vector<double>& getWeights() const { return weights; }
    const std::vector<Asset*>& getAssets() const { return assets; }
    size_t getCommonTradingDays() const { return common_dates.size(); }
};

#endif