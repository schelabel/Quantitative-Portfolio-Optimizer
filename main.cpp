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
            
            auto [prices, start_date, end_date] = CSVHandler::readPrices(expected_filepath);
            
            if (!prices.empty()) {
                Asset* new_asset = nullptr;

                if (meta.type == "Traditional") {
                    new_asset = new TraditionalAsset(ticker, meta.isin, meta.name, meta.currency, prices);
                } else if (meta.type == "Crypto") {
                    new_asset = new CryptoAsset(ticker, meta.isin, meta.name, meta.currency, prices);
                }

                if (new_asset) {
                    portfolio.push_back(new_asset);
                    
                    std::cout << "[+] " << meta.name << " (" << ticker << ") | " << meta.currency << "\n";
                    std::cout << "    └─ Data range: " << start_date << " ➔ " << end_date 
                            << " (" << prices.size() << " days)\n\n";
                } else {
                    std::cerr << "[!] Error: Unknown type in metadata: " << meta.type << "\n";
                }
            } else {
                std::cerr << "[!] Warning: File exists but no data found -> " << expected_filepath << "\n";
            }
        } else {
            std::cerr << "[!] Error: Missing file -> " << expected_filepath << "\n";
        }
    }    

    for (Asset* asset : portfolio) {
        delete asset;
    }
    
    return 0;
}