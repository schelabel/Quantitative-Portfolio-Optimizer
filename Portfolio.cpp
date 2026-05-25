#include "Portfolio.h"
#include <map>
#include <cmath>
#include <numeric>
#include <iostream>

Portfolio::Portfolio(const std::vector<Asset*>& assets, const std::vector<double>& weights)
    : assets(assets), weights(weights) 
{
    if (assets.size() != weights.size()) {
        std::cerr << "Error: Number of assets must match number of weights!\n";
        return;
    }

    synchronizeData();
    calculateCovarianceMatrix();
}


void Portfolio::synchronizeData() {
    if (assets.empty()) return;
    size_t num_assets = assets.size();

    std::map<std::string, int> date_counts;
    
    std::vector<std::map<std::string, double>> asset_returns_map(num_assets);

    for (size_t i = 0; i < num_assets; ++i) {
        const auto& dates = assets[i]->getDates();
        const auto& returns = assets[i]->getLogReturns();
        
        for (size_t k = 0; k < returns.size(); ++k) {
            std::string date = dates[k + 1];
            
            if (asset_returns_map[i].find(date) == asset_returns_map[i].end()) {
                asset_returns_map[i][date] = returns[k];
                date_counts[date]++;
            }
        }
    }

    for (const auto& [date, count] : date_counts) {
        if (count == num_assets) {
            common_dates.push_back(date);
        }
    }

    aligned_log_returns.resize(num_assets);
    for (size_t i = 0; i < num_assets; ++i) {
        aligned_log_returns[i].reserve(common_dates.size());
        for (const std::string& date : common_dates) {
            aligned_log_returns[i].push_back(asset_returns_map[i][date]);
        }
    }
}

void Portfolio::calculateCovarianceMatrix() {
    size_t num_assets = assets.size();
    size_t num_days = common_dates.size();
    
    cov_matrix.assign(num_assets, std::vector<double>(num_assets, 0.0));
    
    if (num_days < 2) return;

    for (size_t i = 0; i < num_assets; ++i) {
        if (aligned_log_returns[i].size() != num_days) {
            std::cerr << "CRITICAL ERROR: Data length mismatch for asset " << i << "!\n";
            return;
        }
    }

    std::vector<double> means(num_assets, 0.0);
    for (size_t i = 0; i < num_assets; ++i) {
        double sum = std::accumulate(aligned_log_returns[i].begin(), aligned_log_returns[i].end(), 0.0);
        means[i] = sum / num_days;
    }

    for (size_t i = 0; i < num_assets; ++i) {
        for (size_t j = 0; j < num_assets; ++j) {
            double cov_sum = 0.0;
            for (size_t t = 0; t < num_days; ++t) {
                cov_sum += (aligned_log_returns[i][t] - means[i]) * (aligned_log_returns[j][t] - means[j]);
            }
            cov_matrix[i][j] = cov_sum / (num_days - 1);
        }
    }
}

double Portfolio::calculateVolatility() const {
    if (cov_matrix.empty()) return 0.0;

    double variance_daily = 0.0;
    size_t num_assets = assets.size();

    for (size_t i = 0; i < num_assets; ++i) {
        for (size_t j = 0; j < num_assets; ++j) {
            variance_daily += weights[i] * weights[j] * cov_matrix[i][j];
        }
    }

    return std::sqrt(variance_daily * 252);
}

double Portfolio::calculateExpectedReturn() const {
    if (common_dates.empty()) return 0.0;

    double expected_portfolio_return_daily = 0.0;
    size_t num_assets = assets.size();

    for (size_t i = 0; i < num_assets; ++i) {
        double sum = std::accumulate(aligned_log_returns[i].begin(), aligned_log_returns[i].end(), 0.0);
        double log_mean = sum / common_dates.size();
        
        double variance = cov_matrix[i][i];

        double arithmetic_mean = log_mean + (variance / 2.0);
        
        expected_portfolio_return_daily += weights[i] * arithmetic_mean;
    }

    return expected_portfolio_return_daily * 252;
}

double Portfolio::getKellyCriterion(double risk_free_rate) const {
    double mu_p = calculateExpectedReturn();
    double sigma_p = calculateVolatility();
    
    if (sigma_p == 0.0) return 0.0;

    return (mu_p - risk_free_rate) / (sigma_p * sigma_p);
}

double Portfolio::getGeometricMean(double risk_free_rate) const {
    double mu_p = calculateExpectedReturn();
    double sigma_p = calculateVolatility();
    
    if (sigma_p == 0.0) return mu_p;

    //Max CAGR with Kelly criteria
    double excess_return = mu_p - risk_free_rate;
    return risk_free_rate + (std::pow(excess_return, 2) / (2.0 * std::pow(sigma_p, 2)));
}