#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

struct PriceData {
    std::vector<double> prices;
    std::vector<std::string> dates;
    std::string start_date;
    std::string end_date;
};

class CSVHandler {
public:
    static PriceData readPrices(const std::string& filepath) {

        PriceData result;
        
        std::ifstream file(filepath);

        if (!file.is_open()) {
            std::cerr << "Error: Couldn't open file: " << filepath << std::endl;
            return result;
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
            std::cerr << "Error: 'Close' column not found in " << filepath << std::endl;
            return result;
        }
        
        bool is_first_line = true;

        while (std::getline(file, line)){
            std::stringstream ss(line);

            std::string token;
            std::vector<std::string> columns;

            while (std::getline(ss, token, ',')){
                columns.push_back(token);
            }

            if (columns.size() > close_idx) {
                
                std::string raw_date = columns[0];
                std::string clean_date = (raw_date.length() >= 10) ? raw_date.substr(0, 10) : raw_date;

                if (is_first_line) {
                    result.start_date = clean_date; 
                    is_first_line = false;
                }
                result.end_date = clean_date;
                
                try {
                    double close_price = std::stod(columns[close_idx]);
                    result.prices.push_back(close_price); 
                    result.dates.push_back(clean_date);
                } catch (...) {
                }
            }
        }
        
        file.close();
        return result;
    }
};