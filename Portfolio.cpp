/**
 * @file Portfolio.cpp
 * @brief Implementation of Portfolio — multi-asset covariance, statistics, and Kelly criterion.
 */

#include "Portfolio.h"
#include <map>
#include <cmath>
#include <numeric>
#include <iostream>

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Constructs a Portfolio, aligns return histories, and builds the covariance matrix.
 *
 * Asset return series often span different date ranges. synchronizeData() finds the
 * intersection of all date sets so that every statistical calculation operates on a
 * consistent, matched time series.
 *
 * @param assets  Pointers to the constituent Asset objects (not owned by Portfolio).
 * @param weights Fractional portfolio weights; must satisfy Σw = 1 and |assets| = |weights|.
 */
Portfolio::Portfolio(const std::vector<Asset*>& assets,
                     const std::vector<double>&  weights)
    : assets(assets), weights(weights)
{
    if (assets.size() != weights.size()) {
        std::cerr << "Error: Number of assets must match number of weights!\n";
        return;
    }

    synchronizeData();
    calculateCovarianceMatrix();
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Finds the intersection of all asset date sets and aligns return vectors.
 *
 * Iterates over every asset's log-return/date pairs, counts how many assets
 * have an observation on each date, and retains only dates present in all
 * assets. The resulting aligned_log_returns matrix has dimensions
 * [num_assets × |common_dates|].
 */
void Portfolio::synchronizeData() {
    if (assets.empty()) return;

    size_t num_assets = assets.size();
    std::map<std::string, int>                         date_counts;
    std::vector<std::map<std::string, double>>         asset_returns_map(num_assets);

    // Build per-asset date → return lookup tables
    for (size_t i = 0; i < num_assets; ++i) {
        const auto& dates   = assets[i]->getDates();
        const auto& returns = assets[i]->getLogReturns();

        for (size_t k = 0; k < returns.size(); ++k) {
            std::string date = dates[k + 1]; // log_return[k] corresponds to dates[k+1]
            if (asset_returns_map[i].find(date) == asset_returns_map[i].end()) {
                asset_returns_map[i][date] = returns[k];
                date_counts[date]++;
            }
        }
    }

    // Retain only dates where all assets have data
    for (const auto& [date, count] : date_counts) {
        if (count == static_cast<int>(num_assets)) {
            common_dates.push_back(date);
        }
    }

    // Populate aligned return matrix
    aligned_log_returns.resize(num_assets);
    for (size_t i = 0; i < num_assets; ++i) {
        aligned_log_returns[i].reserve(common_dates.size());
        for (const std::string& date : common_dates) {
            aligned_log_returns[i].push_back(asset_returns_map[i][date]);
        }
    }
}

/**
 * @brief Computes the full (N × N) sample covariance matrix from aligned log-returns.
 *
 * Uses the unbiased estimator (Bessel's correction, divides by T-1).
 * Results are stored in the daily covariance matrix cov_matrix; callers
 * multiply by 252 to annualise.
 */
void Portfolio::calculateCovarianceMatrix() {
    size_t num_assets = assets.size();
    size_t num_days   = common_dates.size();

    cov_matrix.assign(num_assets, std::vector<double>(num_assets, 0.0));
    if (num_days < 2) return;

    // Sanity check — all aligned series must have equal length
    for (size_t i = 0; i < num_assets; ++i) {
        if (aligned_log_returns[i].size() != num_days) {
            std::cerr << "CRITICAL ERROR: Data length mismatch for asset " << i << "!\n";
            return;
        }
    }

    // Compute per-asset means
    std::vector<double> means(num_assets, 0.0);
    for (size_t i = 0; i < num_assets; ++i) {
        double sum = std::accumulate(aligned_log_returns[i].begin(),
                                     aligned_log_returns[i].end(), 0.0);
        means[i] = sum / num_days;
    }

    // Fill the symmetric covariance matrix
    for (size_t i = 0; i < num_assets; ++i) {
        for (size_t j = 0; j < num_assets; ++j) {
            double cov_sum = 0.0;
            for (size_t t = 0; t < num_days; ++t) {
                cov_sum += (aligned_log_returns[i][t] - means[i])
                         * (aligned_log_returns[j][t] - means[j]);
            }
            cov_matrix[i][j] = cov_sum / (num_days - 1);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Public statistics
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Computes the annualised portfolio volatility σ_p.
 *
 * Uses the full quadratic form:
 * @code
 *   σ_p² = wᵀ Σ w    (daily)
 *   σ_p  = √(wᵀ Σ w × 252)
 * @endcode
 *
 * @return Annualised portfolio volatility, or 0.0 if the covariance matrix is empty.
 */
double Portfolio::calculateVolatility() const {
    if (cov_matrix.empty()) return 0.0;

    double variance_daily = 0.0;
    size_t num_assets = assets.size();

    for (size_t i = 0; i < num_assets; ++i)
        for (size_t j = 0; j < num_assets; ++j)
            variance_daily += weights[i] * weights[j] * cov_matrix[i][j];

    return std::sqrt(variance_daily * 252);
}

/**
 * @brief Computes the annualised arithmetic expected return μ_p.
 *
 * Converts each asset's log-mean to an arithmetic mean via the Itô
 * correction (μ_arith = μ_log + σ²/2), then takes the weighted sum:
 * @code
 *   μ_p = 252 × Σ_i w_i × (μ_log_i + σ_i²/2)
 * @endcode
 *
 * @return Annualised arithmetic portfolio return, or 0.0 if no common dates exist.
 */
double Portfolio::calculateExpectedReturn() const {
    if (common_dates.empty()) return 0.0;

    double expected_daily = 0.0;
    size_t num_assets     = assets.size();

    for (size_t i = 0; i < num_assets; ++i) {
        double sum      = std::accumulate(aligned_log_returns[i].begin(),
                                          aligned_log_returns[i].end(), 0.0);
        double log_mean = sum / common_dates.size();
        double variance = cov_matrix[i][i];               // diagonal = asset variance

        double arithmetic_mean = log_mean + (variance / 2.0); // Itô correction
        expected_daily += weights[i] * arithmetic_mean;
    }

    return expected_daily * 252;
}

/**
 * @brief Computes the continuous Kelly-optimal leverage L*.
 *
 * Derived from maximising the log-utility of a leveraged position:
 * @code
 *   L* = (μ_p − r_f) / σ_p²
 * @endcode
 * This is the leverage at which the geometric mean return is maximised.
 * In practice, half-Kelly (L* / 2) is used to account for parameter uncertainty.
 *
 * @param risk_free_rate Annualised risk-free rate as a decimal (e.g. 0.04).
 * @return Kelly-optimal leverage, or 0.0 if portfolio volatility is zero.
 */
double Portfolio::getKellyCriterion(double risk_free_rate) const {
    double mu_p    = calculateExpectedReturn();
    double sigma_p = calculateVolatility();

    if (sigma_p == 0.0) return 0.0;

    return (mu_p - risk_free_rate) / (sigma_p * sigma_p);
}

/**
 * @brief Computes the maximum achievable geometric mean return under full Kelly leverage.
 *
 * At the Kelly-optimal leverage L*, the long-run compounded growth rate is:
 * @code
 *   g* = r_f + (μ_p − r_f)² / (2 σ_p²)
 * @endcode
 * This is the ceiling on CAGR for this portfolio regardless of leverage applied,
 * and is useful for comparing portfolios on their true compounding potential
 * rather than their arithmetic Sharpe ratio alone.
 *
 * @param risk_free_rate Annualised risk-free rate as a decimal (e.g. 0.04).
 * @return Maximum geometric mean return, or μ_p if volatility is zero.
 */
double Portfolio::getGeometricMean(double risk_free_rate) const {
    double mu_p    = calculateExpectedReturn();
    double sigma_p = calculateVolatility();

    if (sigma_p == 0.0) return mu_p;

    double excess_return = mu_p - risk_free_rate;
    return risk_free_rate
           + (excess_return * excess_return)
           / (2.0 * sigma_p * sigma_p);
}