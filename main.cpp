#include "Asset.h"
#include "CSVHandler.h"

#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>

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
    
    std::vector<Asset*> portfolio;
    
    for (const auto& [ticker, meta] : dictionary) {
        std::string expected_filepath = "data/prices/" + ticker + ".csv";

        if (fs::exists(expected_filepath)) {
            
            std::vector<double> prices = CSVHandler::readPrices(expected_filepath);
            
            if (!prices.empty()) {
                if (meta.type == "Traditional") {
                    Asset* new_asset = new TraditionalAsset(ticker, meta.isin, meta.name, meta.currency, prices);
                    portfolio.push_back(new_asset);
                    std::cout << "Added Traditional Asset: " << meta.name << " (" << ticker << ")\n";
                } 
                else if (meta.type == "Crypto") {
                    Asset* new_asset = new CryptoAsset(ticker, meta.isin, meta.name, meta.currency, prices);
                    portfolio.push_back(new_asset);
                    std::cout << "Added Crypto Asset: " << meta.name << " (" << ticker << ")\n";
                }
                else {
                    std::cerr << "Error: Unknown type in metadata.csv: " << meta.type << "\n";
                }

                std::cout << "  -> Successfully read " << prices.size() << " prices.\n";

            } else {
                std::cerr << "Warning: The file exists but is empty -> " << expected_filepath << "\n";
            }
        } 
        else {
            std::cerr << "Error: Missing CSV file for " << meta.name << " (" << expected_filepath << ")\n";
        }
    }

    std::cout << "\n========================================\n";
    std::cout << "Portfolio loaded with " << portfolio.size() << " assets." << std::endl;
    std::cout << "========================================\n\n";

    for (Asset* asset : portfolio) {
        delete asset;
    }
    
    return 0;
}