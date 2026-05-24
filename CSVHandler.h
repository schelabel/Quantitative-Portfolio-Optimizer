#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

class CSVHandler {
public:
    static std::vector<double> readPrices(const std::string& filename) {
        std::vector<double> extracted_prices;
        std::ifstream file(filename);

        if (!file.is_open()) {
            std::cerr << "Error: Couldn't open file: " << filename << std::endl;
            return extracted_prices;
        }

        std::string line;

        int close_idx = -1;

        if (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            int current_idx = 0;

            while (std::getline(ss, token, ',')) {
                if (token.find("Close") != std::string::npos) {
                    close_idx = current_idx;
                    break;
                }
                current_idx++;
            }
        }

        if (close_idx == -1) {
            std::cerr << "Error: 'Close' column not found in " << filename << std::endl;
            return extracted_prices;
        }
        
        while (std::getline(file, line)){
            std::stringstream ss(line);

            std::string token;
            std::vector<std::string> columns;

            while (std::getline(ss, token, ',')){
                columns.push_back(token);
            }

            if (columns.size() > close_idx) {
                try {
                    double close_price = std::stod(columns[close_idx]);
                    extracted_prices.push_back(close_price); 
                }catch (...) {}
            }

        }
        
        file.close();
        return extracted_prices;
    }
};