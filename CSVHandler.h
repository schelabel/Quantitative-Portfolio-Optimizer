/**
 * @file CSVHandler.h
 * @brief CSV price-data reader for Yahoo Finance export format.
 *
 * Provides a single static method that parses a Yahoo Finance CSV file,
 * extracts closing prices and their associated dates, and returns them
 * in a plain PriceData struct ready for Asset construction.
 */

#ifndef CSVHANDLER_H
#define CSVHANDLER_H

#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

/**
 * @struct PriceData
 * @brief Plain data container returned by CSVHandler::readPrices().
 *
 * All four fields are populated together; if the file cannot be read,
 * prices and dates will be empty and the date strings will be empty.
 */
struct PriceData {
    std::vector<double>      prices;      ///< Chronological closing prices.
    std::vector<std::string> dates;       ///< ISO-8601 date string for each price.
    std::string              start_date;  ///< Date string of the first row.
    std::string              end_date;    ///< Date string of the last row.
};

/**
 * @class CSVHandler
 * @brief Utility class for reading Yahoo Finance CSV price files.
 *
 * All functionality is exposed through a single static method; the class
 * is not intended to be instantiated.
 */
class CSVHandler {
public:
    /**
     * @brief Parses a Yahoo Finance CSV file and extracts closing prices.
     *
     * The method dynamically locates the "Close" column from the header row,
     * so it is robust to column reordering. Non-numeric price rows are silently
     * skipped. Dates are truncated to their first 10 characters (YYYY-MM-DD).
     *
     * @param filepath Path to the CSV file (e.g. "data/prices/BTC-EUR.csv").
     * @return A PriceData struct. If the file cannot be opened or the "Close"
     *         column is absent, prices and dates will be empty.
     */
    static PriceData readPrices(const std::string& filepath) {
        PriceData result;

        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Couldn't open file: " << filepath << std::endl;
            return result;
        }

        // --- Locate the "Close" column index from the header row ---
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

        // --- Parse data rows ---
        bool is_first_line = true;

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string token;
            std::vector<std::string> columns;

            while (std::getline(ss, token, ',')) {
                columns.push_back(token);
            }

            if (columns.size() > static_cast<size_t>(close_idx)) {
                // Truncate timestamp to YYYY-MM-DD
                std::string raw_date   = columns[0];
                std::string clean_date = (raw_date.length() >= 10)
                                         ? raw_date.substr(0, 10)
                                         : raw_date;

                if (is_first_line) {
                    result.start_date = clean_date;
                    is_first_line     = false;
                }
                result.end_date = clean_date;

                try {
                    double close_price = std::stod(columns[close_idx]);
                    result.prices.push_back(close_price);
                    result.dates.push_back(clean_date);
                } catch (...) {
                    // Skip malformed or non-numeric rows silently
                }
            }
        }

        file.close();
        return result;
    }
};

#endif // CSVHANDLER_H