#include "Asset.h"
#include <numeric>
#include <iostream>

// === ASSET BASE CLASS FUNCTIONS ===

void Asset::calculateLogReturns() {
    if (prices.size() < 2) return;

    log_returns.reserve(prices.size() - 1);
    for (size_t i = 1; i < prices.size(); ++i) {
        log_returns.push_back(std::log(prices[i] / prices[i - 1]));
    }
}

double Asset::calculateDailyMeanReturn() const {
    if (log_returns.empty()) return 0.0;
    double sum = std::accumulate(log_returns.begin(), log_returns.end(), 0.0);
    return sum / log_returns.size();
}

double Asset::calculateDailyVolatility() const {
    if (log_returns.size() < 2) return 0.0;
    
    double mean = calculateDailyMeanReturn();
    double variance_sum = 0.0;
    
    for (double r : log_returns) {
        variance_sum += (r - mean) * (r - mean);
    }
    
    return std::sqrt(variance_sum / (log_returns.size() - 1));
}

double Asset::calculateSharpeRatio(double risk_free_rate) const {
    double vol = calculateAnnualVolatility();
    if (vol == 0.0) return 0.0;
    
    return (calculateAnnualExpectedReturn() - risk_free_rate) / vol;
}

// === TRADITIONAL ASSET FUNCTIONS ===

TraditionalAsset::TraditionalAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices, const std::vector<std::string>& dates)
    : Asset(ticker, isin, name, currency, prices, dates) 
{
    calculateLogReturns(); 
}

double TraditionalAsset::calculateAnnualVolatility() const {
    return calculateDailyVolatility() * std::sqrt(252);
}

double TraditionalAsset::calculateAnnualExpectedReturn() const {
    return calculateDailyMeanReturn() * 252;
}

// === CRYPTO ASSET FUNCTIONS ===

CryptoAsset::CryptoAsset(const std::string& ticker, const std::string& isin, const std::string& name, const std::string& currency, const std::vector<double>& prices, const std::vector<std::string>& dates)
    : Asset(ticker, isin, name, currency, prices, dates) 
{
    calculateLogReturns(); 
}

double CryptoAsset::calculateAnnualVolatility() const {
    return calculateDailyVolatility() * std::sqrt(365);
}

double CryptoAsset::calculateAnnualExpectedReturn() const {
    return calculateDailyMeanReturn() * 365;
}