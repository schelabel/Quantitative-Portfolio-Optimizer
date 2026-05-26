/**
 * @file Portfolio.h
 * @brief Multi-asset portfolio with covariance-based risk/return analytics.
 *
 * Portfolio owns a non-owning view over a set of Asset pointers and a
 * matching weight vector. On construction it synchronises the assets to a
 * common date intersection, builds the full N×N sample covariance matrix,
 * and exposes the statistical methods used by Simulator.
 *
 * @note Asset pointers are **not** owned by Portfolio — memory management
 *       is the caller's responsibility (see main.cpp cleanup loop).
 */

#ifndef PORTFOLIO_H
#define PORTFOLIO_H

#include "Asset.h"
#include <vector>
#include <string>

/**
 * @class Portfolio
 * @brief Aggregates multiple assets into a weighted portfolio and computes
 *        core quantitative finance metrics.
 *
 * ### Computation pipeline (constructor)
 * 1. synchronizeData()       — find the date intersection of all assets and
 *                              build aligned_log_returns[asset][day].
 * 2. calculateCovarianceMatrix() — compute the N×N sample covariance matrix
 *                              from the aligned series (Bessel's correction).
 *
 * ### Key outputs
 * - Arithmetic expected return μ_p (Itô-corrected from log-means)
 * - Annualised portfolio volatility σ_p (quadratic form wᵀΣw)
 * - Sharpe-derived Kelly leverage L* = (μ_p − r_f) / σ_p²
 * - Maximum geometric mean g* = r_f + (μ_p − r_f)² / (2σ_p²)
 *
 * @see Simulator for Monte Carlo and frontier sampling that consume these outputs.
 */
class Portfolio {
private:
    std::vector<Asset*>                  assets;               ///< Non-owning pointers to constituent assets.
    std::vector<double>                  weights;              ///< Fractional portfolio weights; must sum to 1.0.
    std::vector<std::string>             common_dates;         ///< Date strings present in every asset's history.
    std::vector<std::vector<double>>     aligned_log_returns;  ///< [asset_index][day_index] aligned log-return matrix.
    std::vector<std::vector<double>>     cov_matrix;           ///< N×N daily sample covariance matrix (annualise × 252).

    /**
     * @brief Finds the date intersection of all assets and populates aligned_log_returns.
     *
     * Iterates every asset's date/return pairs, counts how many assets share
     * each date, and retains only dates present in all assets. The resulting
     * aligned_log_returns matrix has dimensions [num_assets × |common_dates|].
     *
     * Called once during construction before calculateCovarianceMatrix().
     */
    void synchronizeData();

    /**
     * @brief Builds the N×N sample covariance matrix from the aligned return series.
     *
     * Uses the unbiased estimator (divides by T−1, Bessel's correction).
     * Stored as a daily matrix; callers multiply by 252 to annualise.
     *
     * Requires synchronizeData() to have been called first.
     */
    void calculateCovarianceMatrix();

public:
    /**
     * @brief Constructs a Portfolio, aligns date histories, and builds the covariance matrix.
     *
     * @param assets   Non-owning pointers to the constituent Asset objects.
     *                 Must remain valid for the lifetime of this Portfolio.
     * @param weights  Fractional weights in the same order as assets.
     *                 Should satisfy Σw = 1; |assets| must equal |weights|.
     */
    Portfolio(const std::vector<Asset*>& assets, const std::vector<double>& weights);

    /**
     * @brief Computes the annualised arithmetic expected return μ_p.
     *
     * Converts each asset's log-mean to an arithmetic mean via the Itô
     * correction (μ_arith = μ_log + σ²/2), then computes the weighted sum:
     * @code
     *   μ_p = 252 × Σ_i  w_i × (μ_log_i + σ_i²/2)
     * @endcode
     *
     * @return Annualised arithmetic portfolio return as a decimal (e.g. 0.18 = 18%).
     *         Returns 0.0 if no common dates exist.
     */
    double calculateExpectedReturn() const;

    /**
     * @brief Computes the annualised portfolio volatility σ_p.
     *
     * Evaluates the quadratic form over the annualised covariance matrix:
     * @code
     *   σ_p = √( wᵀ Σ w × 252 )
     * @endcode
     *
     * @return Annualised portfolio volatility as a decimal (e.g. 0.137 = 13.7%).
     *         Returns 0.0 if the covariance matrix is empty.
     */
    double calculateVolatility() const;

    /**
     * @brief Replaces the current weight vector without rebuilding the covariance matrix.
     *
     * Used by Simulator::generateEfficientFrontier() to reuse a single Portfolio
     * instance across 50,000 sampling iterations, avoiding repeated O(N²T)
     * covariance construction.
     *
     * @param new_weights New weight vector. Silently ignored if its size does
     *                    not match the number of assets.
     */
    void setWeights(const std::vector<double>& new_weights) {
        if (new_weights.size() == assets.size()) {
            weights = new_weights;
        }
    }

    /**
     * @brief Computes the continuous Kelly-optimal leverage L*.
     *
     * Derived by maximising the expected log-utility of a leveraged position:
     * @code
     *   L* = (μ_p − r_f) / σ_p²
     * @endcode
     * At L*, the long-run geometric mean growth rate is maximised.
     * In practice, Half-Kelly (L*/2) is used to account for parameter
     * uncertainty and gap risk not captured by the GBM assumption.
     *
     * @param risk_free_rate Annualised risk-free rate as a decimal (e.g. 0.04).
     * @return Kelly-optimal leverage multiplier. Returns 0.0 if σ_p = 0.
     */
    double getKellyCriterion(double risk_free_rate) const;

    /**
     * @brief Computes the maximum achievable geometric mean return at full Kelly leverage.
     *
     * At L*, the long-run compounded growth rate (CAGR ceiling) is:
     * @code
     *   g* = r_f + (μ_p − r_f)² / (2σ_p²)
     * @endcode
     * This is the theoretical upper bound on CAGR for this portfolio regardless
     * of leverage applied, and reranks portfolios by true compounding power
     * rather than arithmetic Sharpe alone.
     *
     * @param risk_free_rate Annualised risk-free rate as a decimal (e.g. 0.04).
     * @return Maximum geometric mean return. Returns μ_p if σ_p = 0.
     */
    double getGeometricMean(double risk_free_rate) const;

    /// @name Accessors
    /// @{

    /**
     * @brief Read-only access to the current weight vector.
     * @return Reference to the internal weights vector.
     */
    const std::vector<double>& getWeights() const { return weights; }

    /**
     * @brief Read-only access to the constituent asset pointers.
     * @return Reference to the internal asset pointer vector.
     */
    const std::vector<Asset*>& getAssets() const { return assets; }

    /**
     * @brief Returns the number of trading days in the common date intersection.
     *
     * Useful for assessing data quality: a short common history (e.g. < 500 days)
     * means covariance estimates are unreliable and Kelly leverage will be noisy.
     *
     * @return Number of dates shared by all assets after synchronisation.
     */
    size_t getCommonTradingDays() const { return common_dates.size(); }

    /// @}
};

#endif // PORTFOLIO_H