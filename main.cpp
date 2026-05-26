/**
 * @file main.cpp
 * @brief Entry point for the Quantitative Portfolio Simulator & Optimizer.
 *
 * Execution flow:
 *  1. Load asset metadata from data/metadata.csv.
 *  2. For each listed asset, read its price CSV and construct a typed Asset object.
 *  3. Map the efficient frontier via 50 000 random portfolio samples.
 *  4. Construct seven archetype portfolios (5 sampled + 2 deterministic).
 *  5. Run Monte Carlo simulation for each and print results to stdout.
 *
 * @note Memory for heap-allocated Asset objects is released before exit.
 */

#include "Asset.h"
#include "CSVHandler.h"
#include "Portfolio.h"
#include "Simulator.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>

namespace fs = std::filesystem;

// ─────────────────────────────────────────────────────────────────────────────
// Metadata types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @struct AssetMetadata
 * @brief Plain record parsed from one row of data/metadata.csv.
 */
struct AssetMetadata {
    std::string isin;      ///< ISIN identifier.
    std::string name;      ///< Human-readable display name.
    std::string currency;  ///< Denomination currency (e.g. "EUR").
    std::string type;      ///< Asset class: "Traditional" or "Crypto".
};

// ─────────────────────────────────────────────────────────────────────────────
// Metadata loader
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Parses data/metadata.csv into a ticker → AssetMetadata map.
 *
 * Expected CSV format (with header):
 * @code
 *   Ticker,ISIN,Name,Currency,Type
 *   BTC-EUR,,Bitcoin,EUR,Crypto
 *   VWCE.DE,IE00B3RBWM25,Vanguard FTSE All-World,EUR,Traditional
 * @endcode
 * Trailing carriage returns on the Type field are stripped for Windows compatibility.
 *
 * @param filepath Path to the metadata CSV file.
 * @return Map from ticker string to AssetMetadata; empty on failure.
 */
std::map<std::string, AssetMetadata> loadMetadata(const std::string& filepath) {
    std::map<std::string, AssetMetadata> metadata_map;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open file: " << filepath << std::endl;
        return metadata_map;
    }

    std::string line;

    // Skip header row
    if (std::getline(file, line)) {}

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string ticker, isin, name, currency, type;

        std::getline(ss, ticker,   ',');
        std::getline(ss, isin,     ',');
        std::getline(ss, name,     ',');
        std::getline(ss, currency, ',');
        std::getline(ss, type,     ',');

        // Strip trailing \r (Windows line endings)
        if (!type.empty() && type.back() == '\r')
            type.pop_back();

        metadata_map[ticker] = {isin, name, currency, type};
    }

    return metadata_map;
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Application entry point.
 *
 * Loads assets, maps the frontier, builds seven archetype portfolios,
 * and prints Monte Carlo simulation results for each.
 *
 * @param argc Argument count (unused).
 * @param argv Argument vector (unused).
 * @return 0 on success.
 */
int main(int argc, char* argv[]) {
    std::map<std::string, AssetMetadata> dictionary = loadMetadata("data/metadata.csv");
    std::vector<Asset*> portfolio_assets;

    // ── Load assets ──────────────────────────────────────────────────────────
    for (const auto& [ticker, meta] : dictionary) {
        std::string expected_filepath = "data/prices/" + ticker + ".csv";

        if (std::filesystem::exists(expected_filepath)) {
            auto [prices, dates, start_date, end_date] =
                CSVHandler::readPrices(expected_filepath);

            if (!prices.empty()) {
                Asset* new_asset = nullptr;

                if (meta.type == "Traditional")
                    new_asset = new TraditionalAsset(ticker, meta.isin, meta.name,
                                                      meta.currency, prices, dates);
                else if (meta.type == "Crypto")
                    new_asset = new CryptoAsset(ticker, meta.isin, meta.name,
                                                 meta.currency, prices, dates);

                if (new_asset) {
                    portfolio_assets.push_back(new_asset);
                    std::cout << "[+] " << meta.name
                              << " (" << ticker << ") | " << meta.currency << "\n";
                    std::cout << "    └─ Data range: " << start_date
                              << " ➔ " << end_date
                              << " (" << prices.size() << " days)\n\n";
                }
            }
        }
    }

    std::cout << "========================================\n";
    std::cout << "Assets loaded. Booting Quantitative Engine...\n";
    std::cout << "========================================\n\n";

    if (!portfolio_assets.empty()) {
        const double risk_free_rate = 0.02;

        // ── Step 1: Map the efficient frontier (50 000 random samples) ───────
        FrontierChampions champs = Simulator::generateEfficientFrontier(
            portfolio_assets, 50000, risk_free_rate, "output_frontier.csv");

        // ── Step 2: Construct the five sampled archetype portfolios ───────────
        Portfolio max_sharpe_port (portfolio_assets, champs.w_max_sharpe);
        Portfolio min_vol_port    (portfolio_assets, champs.w_min_vol);
        Portfolio max_cagr_port   (portfolio_assets, champs.w_max_cagr);
        Portfolio max_div_port    (portfolio_assets, champs.w_max_div);
        Portfolio target_vol_port (portfolio_assets, champs.w_target_vol);

        // ── Step 3: Construct the two deterministic portfolios ────────────────

        // A) Equal Weight (1/N) — trivial benchmark
        std::vector<double> equal_weights(portfolio_assets.size(),
                                           1.0 / portfolio_assets.size());
        Portfolio equal_weight_port(portfolio_assets, equal_weights);

        // B) Risk Parity (Inverse Volatility approximation of Equal Risk Contribution)
        std::vector<double> rp_weights(portfolio_assets.size());
        double inv_vol_sum = 0.0;
        for (size_t i = 0; i < portfolio_assets.size(); ++i) {
            rp_weights[i] = 1.0 / portfolio_assets[i]->calculateAnnualVolatility();
            inv_vol_sum  += rp_weights[i];
        }
        double rp_rounded_sum = 0.0;
        for (size_t i = 0; i < portfolio_assets.size(); ++i) {
            rp_weights[i] = std::round((rp_weights[i] / inv_vol_sum) * 100.0) / 100.0;
            rp_rounded_sum += rp_weights[i];
        }
        // Correct any rounding residual on the largest weight
        if (std::abs(1.0 - rp_rounded_sum) > 1e-9) {
            auto max_it  = std::max_element(rp_weights.begin(), rp_weights.end());
            *max_it     += (1.0 - rp_rounded_sum);
        }
        Portfolio risk_parity_port(portfolio_assets, rp_weights);

        // ── Step 4: Evaluate all seven archetypes ─────────────────────────────
        std::cout << ">>> EVALUATING THE 7 QUANTITATIVE ARCHETYPES <<<\n";

        std::cout << Simulator::runMonteCarlo("Equal Weight (1/N)",                   equal_weight_port,  risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Risk Parity (Equal Risk Contribution)", risk_parity_port,   risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Minimum Variance",                      min_vol_port,       risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Maximum Diversification",               max_div_port,       risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Target Volatility (10%)",               target_vol_port,    risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Maximum Sharpe (Tangency)",             max_sharpe_port,    risk_free_rate);
        std::cout << Simulator::runMonteCarlo("Maximum Return",                        max_cagr_port,      risk_free_rate);
    }

    // ── Memory cleanup ────────────────────────────────────────────────────────
    for (Asset* asset : portfolio_assets)
        delete asset;

    return 0;
}