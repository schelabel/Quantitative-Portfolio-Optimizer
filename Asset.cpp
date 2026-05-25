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

double Asset::calculateDailyVolatility() {
    if (prices.size() < 3) {
        return 0.0;
    }

    double daily_mean = calculateDailyMeanReturn();

    std::vector<double> returns;

    for (size_t i = 1; i < prices.size(); ++i){
        double daily_return = (prices[i] - prices[i-1]) / prices[i-1];
        returns.push_back(daily_return);
    }

    double variance_sum = 0.0;
    for (size_t i = 0; i < returns.size(); ++i){
        variance_sum += (returns[i] - daily_mean) * (returns[i] - daily_mean);
    }

    double variance = variance_sum / (returns.size() - 1);

    return std::sqrt(variance);
}

double Asset::calculateSharpeRatio(double risk_free_rate){
    double expected_return = calculateAnnualExpectedReturn(); 
    double volatility = calculateAnnualVolatility();

    if (volatility == 0.0) {
        return 0.0; 
    }

    return (expected_return - risk_free_rate) / volatility;
}

double Asset::calculateKellyCriterion(double risk_free_rate){
    double expected_return = calculateAnnualExpectedReturn(); 
    double volatility = calculateAnnualVolatility();

    if (volatility == 0.0) {
        return 0.0; 
    }

    return (expected_return - risk_free_rate) / (volatility * volatility);
}


// === TRADITIONAL ASSET FUNCTIONS ===

double TraditionalAsset::calculateAnnualVolatility() {
    return calculateDailyVolatility() * std::sqrt(252);
}

double TraditionalAsset::calculateAnnualExpectedReturn() {
    return calculateDailyMeanReturn() * 252;
}


// === CRYPTO ASSET FUNCTIONS ===

double CryptoAsset::calculateAnnualVolatility() {
    return calculateDailyVolatility() * std::sqrt(365);
}

double CryptoAsset::calculateAnnualExpectedReturn() {
    return calculateDailyMeanReturn() * 365;
}