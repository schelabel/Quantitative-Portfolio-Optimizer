#include "Asset.h"
#include "CSVHandler.h"
#include "Portfolio.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <iomanip>

namespace fs = std::filesystem;

struct AssetMetadata {
    std::string isin;
    std::string name;
    std::string currency;
    std::string type;
};

std::map<std::string, AssetMetadata> loadMetadata(const std::string& filepath){
    std::map<std::string, AssetMetadata> metadata_map;

    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Error: Couldn't open file: " << filepath << std::endl;
    }

    std::string line;

    if (std::getline(file, line)){}

    while (std::getline(file, line)){
        std::stringstream ss(line);

        std::string ticker, isin, name, currency, type;

        std::getline(ss, ticker, ',');
        std::getline(ss, isin, ',');
        std::getline(ss, name, ',');
        std::getline(ss, currency, ',');
        std::getline(ss, type, ',');

        if (!type.empty() && type.back() == '\r') {
            type.pop_back();
        }

        metadata_map[ticker] = {isin, name, currency, type};
    }

    return metadata_map;
}

int main(int argc, char* argv[]) { 
    std::map<std::string, AssetMetadata> dictionary = loadMetadata("data/metadata.csv");
    std::vector<Asset*> portfolio_assets;
    
    for (const auto& [ticker, meta] : dictionary) {
        std::string expected_filepath = "data/prices/" + ticker + ".csv";

        if (std::filesystem::exists(expected_filepath)) {
            auto [prices, dates, start_date, end_date] = CSVHandler::readPrices(expected_filepath);
            
            if (!prices.empty()) {
                Asset* new_asset = nullptr;

                if (meta.type == "Traditional") {
                    new_asset = new TraditionalAsset(ticker, meta.isin, meta.name, meta.currency, prices, dates);
                } else if (meta.type == "Crypto") {
                    new_asset = new CryptoAsset(ticker, meta.isin, meta.name, meta.currency, prices, dates);
                }

                if (new_asset) {
                    portfolio_assets.push_back(new_asset);
                    std::cout << "[+] " << meta.name << " (" << ticker << ") | " << meta.currency << "\n";
                    std::cout << "    └─ Data range: " << start_date << " ➔ " << end_date 
                              << " (" << prices.size() << " days)\n\n";
                }
            }
        }
    }

    std::cout << "========================================\n";
    std::cout << "Assets loaded. Booting Quantitative Engine...\n";
    std::cout << "========================================\n\n";

    if (!portfolio_assets.empty()) {
        double equal_weight = 1.0 / portfolio_assets.size();
        std::vector<double> weights(portfolio_assets.size(), equal_weight);

        Portfolio benchmark(portfolio_assets, weights);

        double risk_free_rate = 0.02;

        std::cout << ">>> BENCHMARK: EQUAL WEIGHT (1/N) PORTFOLIO <<<\n";
        std::cout << "Common Trading Days: " << benchmark.getCommonTradingDays() << "\n\n";
        
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Expected Return (mu): " << benchmark.calculateExpectedReturn() * 100 << " %\n";
        std::cout << "Volatility (sigma):   " << benchmark.calculateVolatility() * 100 << " %\n";
        std::cout << "Sharpe Ratio:         " << (benchmark.calculateExpectedReturn() - risk_free_rate) / benchmark.calculateVolatility() << "\n";
        
        double kelly = benchmark.getKellyCriterion(risk_free_rate);
        std::cout << "Optimal Kelly (L*):   " << kelly << "x leverage\n";
        std::cout << "Half-Kelly (L*/2):    " << kelly / 2.0 << "x leverage\n";
        std::cout << "Kelly Geometric Mean: " << benchmark.getGeometricMean(risk_free_rate) * 100 << " %\n\n";
    }

    for (Asset* asset : portfolio_assets) {
        delete asset;
    }
    
    return 0;
}