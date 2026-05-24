#include "Asset.h"
#include "CSVHandler.h"
#include <iostream>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

int main() {
    std::vector<Asset*> portfolio;
    std::vector<std::string> folders = {"data/traditional", "data/crypto"};

    for (const std::string& current_folder : folders) {
        if (fs::exists(current_folder) && fs::is_directory(current_folder)) {

            for (const auto& entry : fs::directory_iterator(current_folder)) {

                if (entry.is_regular_file()) {
                    std::string full_path = entry.path().string();
                    std::string filename = entry.path().filename().string();
                    std::vector<double> prices = CSVHandler::readPrices(full_path);
                    
                    if (!prices.empty()) {
                        std::string ticker = filename.substr(0, filename.find_last_of('.'));

                        if (current_folder == "data/traditional") {
                            Asset* new_asset = new TraditionalAsset(ticker, "N/A", ticker, prices);

                            portfolio.push_back(new_asset);

                            std::cout << "Added Traditional Asset: " << ticker << std::endl;
                        } 
                        else if (current_folder == "data/crypto") {
                            Asset* new_asset = new CryptoAsset(ticker, "N/A", ticker, prices);

                            portfolio.push_back(new_asset);
                            
                            std::cout << "Added Crypto Asset: " << ticker << std::endl;
                        }
                        
                        std::cout << "Successfully read " << prices.size() << " prices from " << filename << std::endl;
                    } else {
                        std::cerr << "Warning: No data extracted from " << filename << std::endl;
                    }
                }
            }

        } else {
            std::cerr << "Error: Directory '" << current_folder << "' not found!" << std::endl;
        }

    }

    for (Asset* asset : portfolio) {
        delete asset;
    }
    
    return 0;
}