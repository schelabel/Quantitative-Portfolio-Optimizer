/**
 * @file Simulator.h
 * @brief Monte Carlo simulation engine and efficient frontier sampler.
 *
 * Provides two independent capabilities:
 *  - runMonteCarlo()            — evaluates a single Portfolio under half-Kelly leverage
 *                                 via geometric Brownian motion simulation.
 *  - generateEfficientFrontier() — brute-force samples the weight simplex to map the
 *                                  efficient frontier and identify five archetype portfolios.
 */

#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "Portfolio.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// SimulationResult
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct SimulationResult
 * @brief Aggregated output from a single runMonteCarlo() call.
 *
 * Holds both the deterministic metrics (Sharpe, Kelly leverage, leveraged
 * return/volatility) and the stochastic 1-year percentile forecasts derived
 * from 10 000 GBM paths.
 */
struct SimulationResult {
    std::string name;               ///< Human-readable portfolio name.

    double expected_return;         ///< Annualised arithmetic return μ_p at 1× (unleveraged).
    double volatility;              ///< Annualised volatility σ_p at 1× (unleveraged).
    double leveraged_return;        ///< Arithmetic return at the applied leverage: r_f + L(μ_p − r_f).
    double leveraged_volatility;    ///< Volatility at the applied leverage: L × σ_p.
    double sharpe;                  ///< Sharpe ratio: (μ_p − r_f) / σ_p.
    double optimal_kelly;           ///< Full Kelly leverage L* = (μ_p − r_f) / σ_p².
    double applied_leverage;        ///< Actual leverage used in simulation (= L*/2).

    std::vector<double>      weights;  ///< Portfolio weights at 1% resolution.
    std::vector<std::string> tickers;  ///< Asset tickers in the same order as weights.

    /// @name Monte Carlo percentiles (1-year horizon, leveraged)
    /// Percentiles correspond to a normal distribution:
    ///   p_very_bad ≈ 2.27th, p_bad ≈ 15.87th, p_avg ≈ 50th,
    ///   p_good ≈ 84.13th, p_very_good ≈ 97.72nd.
    /// @{
    double p_very_bad;   ///< −2σ outcome.
    double p_bad;        ///< −1σ outcome.
    double p_avg;        ///< Median outcome.
    double p_good;       ///< +1σ outcome.
    double p_very_good;  ///< +2σ outcome.
    /// @}

    /**
     * @brief Stream-insertion operator for formatted console output.
     *
     * Prints a self-contained block showing weights, core risk metrics,
     * base vs. leveraged comparison, the 1× geometric mean, and the
     * five-scenario Monte Carlo forecast.
     *
     * @param os  Output stream.
     * @param res SimulationResult to print.
     * @return Reference to os for chaining.
     */
    // Overloaded operator<< for formatted console output
    friend std::ostream& operator<<(std::ostream& os, const SimulationResult& res) {
        os << "\n========================================\n";
        os << " PORTFOLIO: " << res.name << "\n";
        os << "========================================\n";
        
        os << " Target Weights: ";
        // Dynamic precision: 0 decimal places for <= 10 assets, 1 decimal place for > 10
        int precision = (res.weights.size() <= 10) ? 0 : 1; 
        
        for (size_t i = 0; i < res.weights.size(); ++i) {
            os << res.tickers[i] << ": " << std::fixed << std::setprecision(precision) << res.weights[i] * 100.0 << "%";
            if (i < res.weights.size() - 1) os << " | ";
        }
        os << "\n----------------------------------------\n";
        os << std::fixed << std::setprecision(2);
        os << " Sharpe Ratio:              " << res.sharpe << "\n";
        os << " Optimal Full Kelly (L*):   " << res.optimal_kelly << "x leverage\n";
        os << " Target Half-Kelly (L*/2):  " << res.optimal_kelly / 2.0 << "x leverage\n";
        os << "----------------------------------------\n";
        
        // Calculate Geometric Means dynamically for the printout
        double base_geo = res.expected_return - (0.5 * res.volatility * res.volatility);
        double lev_geo = res.leveraged_return - (0.5 * res.leveraged_volatility * res.leveraged_volatility);

        os << " METRIC COMPARISON\n";
        os << " Base (1x)\n";
        os << "  Return:           " << res.expected_return * 100 << " %\n";
        os << "  Volatility:       " << res.volatility * 100 << " %\n";
        os << "  Geometric Mean:   " << base_geo * 100 << " %\n";
        os << "----------------------------------------\n";
        os << " Leveraged (" << res.applied_leverage << "x applied)\n";
        os << "  Return:           " << res.leveraged_return * 100 << " %\n";
        os << "  Volatility:       " << res.leveraged_volatility * 100 << " %\n";
        os << "  Geometric Mean:   " << lev_geo * 100 << " %\n";
        os << "----------------------------------------\n";
        os << " 1-Year Leveraged Monte Carlo Forecast:\n";
        os << "  Very Good (+2σ):  " << res.p_very_good * 100 << " %\n";
        os << "  Good      (+1σ):  " << res.p_good * 100 << " %\n";
        os << "  Average   (Mean): " << res.p_avg * 100 << " %\n";
        os << "  Bad       (-1σ):  " << res.p_bad * 100 << " %\n";
        os << "  Very Bad  (-2σ):  " << res.p_very_bad * 100 << " %\n";
        
        if (res.p_very_bad <= -0.99) {
            os << " [!] WARNING: High risk of total liquidation / margin call at -2σ!\n";
        }
        os << "========================================\n";
        return os;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FrontierChampions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct FrontierChampions
 * @brief Weight vectors for the five archetype portfolios found during frontier sampling.
 *
 * Each field is populated by generateEfficientFrontier() as the brute-force sampler
 * iterates over random weight combinations.
 */
struct FrontierChampions {
    std::vector<double> w_max_sharpe;  ///< Weights achieving the highest Sharpe ratio.
    std::vector<double> w_min_vol;     ///< Weights achieving the lowest portfolio volatility.
    std::vector<double> w_max_cagr;    ///< Weights maximising the unleveraged geometric mean.
    std::vector<double> w_max_div;     ///< Weights maximising the diversification ratio.
    std::vector<double> w_target_vol;  ///< Weights closest to a 10% annualised volatility target.
};

// ─────────────────────────────────────────────────────────────────────────────
// Simulator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @class Simulator
 * @brief Stateless utility class providing Monte Carlo simulation and frontier mapping.
 *
 * All methods are static; the class is not intended to be instantiated.
 */
class Simulator {
public:
    /**
     * @brief Runs a 1-year Monte Carlo simulation for a portfolio under half-Kelly leverage.
     *
     * The simulation models the leveraged portfolio return as a single-step GBM:
     * @code
     *   R = exp( drift + σ_L × Z ) − 1,    Z ~ N(0,1)
     *   drift = μ_L − ½ σ_L²              (Itô correction)
     *   μ_L   = r_f + L(μ_p − r_f)
     *   σ_L   = L × σ_p
     * @endcode
     * Simulated returns are floored at −1.0 to reflect hard margin liquidation.
     * 10 000 paths are generated with a fixed seed (42) for reproducibility.
     * Percentiles are extracted at the ±1σ and ±2σ quantiles of the normal distribution.
     *
     * @param name                  Display name for the portfolio.
     * @param port                  Portfolio whose statistics drive the simulation.
     * @param risk_free_rate        Annualised risk-free rate (e.g. 0.04).
     * @param use_half_kelly_leverage If true (default), applies L = L* / 2; otherwise 1×.
     * @return Fully populated SimulationResult.
     */
    static SimulationResult runMonteCarlo(const std::string& name,
                                          const Portfolio&    port,
                                          double              risk_free_rate,
                                          bool                use_half_kelly_leverage = true)
    {
        SimulationResult res;
        res.name            = name;
        res.expected_return = port.calculateExpectedReturn();
        res.volatility      = port.calculateVolatility();
        res.sharpe          = (res.volatility == 0.0)
                              ? 0.0
                              : (res.expected_return - risk_free_rate) / res.volatility;
        res.optimal_kelly   = port.getKellyCriterion(risk_free_rate);

        for (const auto& asset : port.getAssets())
            res.tickers.push_back(asset->getTicker());
        res.weights = port.getWeights();

        // Determine applied leverage
        double L = 1.0;
        if (use_half_kelly_leverage && res.optimal_kelly > 0.0)
            L = res.optimal_kelly / 2.0;
        res.applied_leverage     = L;
        res.leveraged_return     = risk_free_rate + L * (res.expected_return - risk_free_rate);
        res.leveraged_volatility = L * res.volatility;

        // GBM simulation
        const int num_simulations = 10000;
        std::vector<double> final_returns(num_simulations);

        std::mt19937 generator(42);
        std::normal_distribution<double> distribution(0.0, 1.0);

        double drift = res.leveraged_return
                     - 0.5 * res.leveraged_volatility * res.leveraged_volatility;

        for (int i = 0; i < num_simulations; ++i) {
            double Z = distribution(generator);
            double r = std::exp(drift + res.leveraged_volatility * Z) - 1.0;
            final_returns[i] = (r < -1.0) ? -1.0 : r; // hard liquidation floor
        }

        std::sort(final_returns.begin(), final_returns.end());

        // Extract ±1σ and ±2σ percentiles from the sorted sample
        res.p_very_bad  = final_returns[static_cast<int>(0.0227 * num_simulations)];
        res.p_bad       = final_returns[static_cast<int>(0.1586 * num_simulations)];
        res.p_avg       = final_returns[static_cast<int>(0.5000 * num_simulations)];
        res.p_good      = final_returns[static_cast<int>(0.8413 * num_simulations)];
        res.p_very_good = final_returns[static_cast<int>(0.9772 * num_simulations)];

        return res;
    }

    /**
     * @brief Brute-force samples the weight simplex to map the efficient frontier.
     *
     * Generates @p num_portfolios random weight vectors, rounds them to 1% (or 0.1%
     * for large universes) resolution using the Largest Remainder Method, evaluates
     * each on return, volatility, Sharpe, Kelly, diversification ratio, and unleveraged
     * CAGR, and tracks the best portfolio for each of five criteria.
     *
     * All sampled metrics are written to @p filename as CSV for external visualisation.
     *
     * @param assets         Assets in the investable universe.
     * @param num_portfolios Number of random weight vectors to sample (e.g. 50 000).
     * @param risk_free_rate Annualised risk-free rate used for Sharpe and Kelly.
     * @param filename       Output CSV path (e.g. "output_frontier.csv").
     * @return FrontierChampions struct containing the five archetype weight vectors.
     *
     * @note A single "engine" Portfolio is reused across iterations via setWeights()
     *       to avoid O(N) construction overhead per sample.
     * @note Random seed is fixed at 1337 for reproducibility.
     */
    static FrontierChampions generateEfficientFrontier(const std::vector<Asset*>& assets,
                                                        int                        num_portfolios,
                                                        double                     risk_free_rate,
                                                        const std::string&         filename)
    {
        std::ofstream file(filename);
        FrontierChampions champs;

        if (!file.is_open()) {
            std::cerr << "Error: Failed to open " << filename << " for writing.\n";
            return champs;
        }

        file << "Return,Volatility,Sharpe,Kelly,UnleveragedCAGR\n";

        std::mt19937 gen(1337);
        std::uniform_real_distribution<double> dist(0.0, 1.0);

        // Pre-compute individual volatilities for diversification ratio
        std::vector<double> individual_vols(assets.size());
        for (size_t i = 0; i < assets.size(); ++i)
            individual_vols[i] = assets[i]->calculateAnnualVolatility();

        // Tracking variables initialised to extreme limits
        double max_sharpe        = -std::numeric_limits<double>::max();
        double min_vol           =  std::numeric_limits<double>::max();
        double max_cagr          = -std::numeric_limits<double>::max();
        double max_div_ratio     = -std::numeric_limits<double>::max();
        double min_target_vol_diff =  std::numeric_limits<double>::max();

        // Dynamic resolution: 1% for ≤10 assets, 0.1% for larger universes
        bool        is_small    = assets.size() <= 10;
        double      round_factor = is_small ? 100.0 : 1000.0;
        std::string res_string  = is_small ? "1%"  : "0.1%";

        std::cout << ">> Sampling " << num_portfolios
                  << " random portfolios (" << res_string
                  << " resolution) to map the Efficient Frontier...\n";

        // Reusable engine portfolio — avoids repeated construction overhead
        std::vector<double> initial_weights(assets.size(), 1.0 / assets.size());
        Portfolio engine(assets, initial_weights);

        for (int i = 0; i < num_portfolios; ++i) {
            // 1. Draw raw weights
            std::vector<double> weights(assets.size());
            double sum = 0.0;
            for (size_t w = 0; w < weights.size(); ++w) {
                weights[w] = dist(gen);
                sum        += weights[w];
            }

            // 2. Normalise to sum = 1.0
            for (size_t w = 0; w < weights.size(); ++w)
                weights[w] /= sum;

            // 3. Round to resolution (Largest Remainder Method)
            double rounded_sum = 0.0;
            for (size_t w = 0; w < weights.size(); ++w) {
                weights[w]  = std::round(weights[w] * round_factor) / round_factor;
                rounded_sum += weights[w];
            }

            // 4. Assign rounding remainder to the largest weight
            double diff = 1.0 - rounded_sum;
            if (std::abs(diff) > 1e-9) {
                auto max_it  = std::max_element(weights.begin(), weights.end());
                *max_it     += diff;
            }

            engine.setWeights(weights);

            double ret   = engine.calculateExpectedReturn();
            double vol   = engine.calculateVolatility();
            double sharpe = (vol == 0.0) ? 0.0 : (ret - risk_free_rate) / vol;
            double cagr  = ret - 0.5 * vol * vol;   // unleveraged geometric mean
            double kelly = engine.getKellyCriterion(risk_free_rate);

            // Diversification ratio: Σ w_i σ_i / σ_p
            double weighted_vol_sum = 0.0;
            for (size_t w = 0; w < weights.size(); ++w)
                weighted_vol_sum += weights[w] * individual_vols[w];
            double div_ratio = (vol > 0.0) ? (weighted_vol_sum / vol) : 0.0;

            double vol_diff = std::abs(vol - 0.10); // distance from 10% target vol

            // Update champions
            if (sharpe    > max_sharpe)        { max_sharpe        = sharpe;    champs.w_max_sharpe   = weights; }
            if (vol       < min_vol)            { min_vol           = vol;       champs.w_min_vol      = weights; }
            if (cagr      > max_cagr)           { max_cagr          = cagr;      champs.w_max_cagr     = weights; }
            if (div_ratio > max_div_ratio)      { max_div_ratio     = div_ratio; champs.w_max_div      = weights; }
            if (vol_diff  < min_target_vol_diff){ min_target_vol_diff = vol_diff; champs.w_target_vol  = weights; }

            file << ret << "," << vol << "," << sharpe << ","
                 << kelly << "," << cagr << "\n";
        }

        std::cout << ">> Efficient Frontier mapped! Data written to: " << filename << "\n\n";
        return champs;
    }
};

#endif // SIMULATOR_H