/**
 * @file Asset.h
 * @brief Abstract base class and concrete asset type definitions.
 *
 * Defines the Asset hierarchy used throughout the Quantitative Portfolio
 * Optimizer. TraditionalAsset and CryptoAsset differ only in their
 * annualisation factor (252 trading days vs 365 calendar days).
 */

#ifndef ASSET_H
#define ASSET_H

#include <string>
#include <vector>
#include <cmath>

/**
 * @class Asset
 * @brief Abstract base class representing a single investable asset.
 *
 * Stores raw price data and derived log-returns. Subclasses implement
 * asset-class-specific annualisation via the pure-virtual methods
 * calculateAnnualVolatility() and calculateAnnualExpectedReturn().
 */
class Asset {
protected:
    std::string ticker;    ///< Exchange ticker symbol (e.g. "BTC-EUR").
    std::string isin;      ///< International Securities Identification Number.
    std::string name;      ///< Human-readable display name.
    std::string currency;  ///< Denomination currency (e.g. "EUR").

    std::vector<double>      prices;       ///< Raw closing prices, chronological order.
    std::vector<std::string> dates;        ///< ISO-8601 date strings matching prices.
    std::vector<double>      log_returns;  ///< Daily log-returns: ln(P_t / P_{t-1}).

    /**
     * @brief Populates log_returns from the stored price series.
     *
     * Computes ln(prices[i] / prices[i-1]) for i = 1 … N-1.
     * Called once in each concrete subclass constructor.
     */
    void calculateLogReturns();

    /**
     * @brief Returns the arithmetic mean of daily log-returns.
     * @return Mean daily log-return, or 0.0 if log_returns is empty.
     */
    double calculateDailyMeanReturn() const;

    /**
     * @brief Returns the sample standard deviation of daily log-returns.
     *
     * Uses Bessel's correction (divides by N-1).
     * @return Daily volatility, or 0.0 if fewer than two observations exist.
     */
    double calculateDailyVolatility() const;

public:
    /**
     * @brief Constructs an Asset with raw price data.
     * @param ticker    Exchange ticker symbol.
     * @param isin      ISIN identifier string.
     * @param name      Human-readable name.
     * @param currency  Denomination currency.
     * @param prices    Chronological closing prices.
     * @param dates     ISO-8601 date strings, same length as prices.
     */
    Asset(const std::string& ticker,
          const std::string& isin,
          const std::string& name,
          const std::string& currency,
          const std::vector<double>&      prices,
          const std::vector<std::string>& dates)
        : ticker(ticker), isin(isin), name(name), currency(currency),
          prices(prices), dates(dates) {}

    /// Virtual destructor — required for correct polymorphic deletion.
    virtual ~Asset() = default;

    /**
     * @brief Returns the annualised volatility (σ) for this asset class.
     *
     * Annualisation factor is asset-class-specific:
     *   - TraditionalAsset: √252  (trading days)
     *   - CryptoAsset:      √365  (calendar days)
     * @return Annualised volatility as a decimal (e.g. 0.65 = 65%).
     */
    virtual double calculateAnnualVolatility() const = 0;

    /**
     * @brief Returns the annualised expected (arithmetic) return (μ) for this asset class.
     * @return Annualised arithmetic mean return as a decimal.
     */
    virtual double calculateAnnualExpectedReturn() const = 0;

    /**
     * @brief Computes the annualised Sharpe ratio.
     * @param risk_free_rate Annualised risk-free rate as a decimal (e.g. 0.04).
     * @return Sharpe ratio, or 0.0 if volatility is zero.
     */
    double calculateSharpeRatio(double risk_free_rate) const;

    /// @name Accessors
    /// @{
    std::string getTicker()   const { return ticker;   }
    std::string getName()     const { return name;     }
    std::string getCurrency() const { return currency; }

    /** @brief Read-only access to daily log-returns. */
    const std::vector<double>&      getLogReturns() const { return log_returns; }
    /** @brief Read-only access to raw closing prices. */
    const std::vector<double>&      getPrices()     const { return prices;      }
    /** @brief Read-only access to date strings. */
    const std::vector<std::string>& getDates()      const { return dates;       }
    /// @}
};


/**
 * @class TraditionalAsset
 * @brief Concrete asset for equities, ETFs, and commodities.
 *
 * Annualises statistics using 252 trading days per year.
 */
class TraditionalAsset : public Asset {
public:
    /**
     * @brief Constructs a TraditionalAsset and computes log-returns.
     * @param ticker    Exchange ticker symbol.
     * @param isin      ISIN identifier string.
     * @param name      Human-readable name.
     * @param currency  Denomination currency.
     * @param prices    Chronological closing prices.
     * @param dates     ISO-8601 date strings, same length as prices.
     */
    TraditionalAsset(const std::string& ticker,
                     const std::string& isin,
                     const std::string& name,
                     const std::string& currency,
                     const std::vector<double>&      prices,
                     const std::vector<std::string>& dates);

    /**
     * @brief Annualised volatility using a 252 trading-day factor.
     * @return σ_daily × √252.
     */
    double calculateAnnualVolatility()     const override;

    /**
     * @brief Annualised expected return using a 252 trading-day factor.
     * @return μ_daily × 252.
     */
    double calculateAnnualExpectedReturn() const override;
};


/**
 * @class CryptoAsset
 * @brief Concrete asset for cryptocurrencies and digital assets.
 *
 * Annualises statistics using 365 calendar days per year to reflect
 * continuous 24/7 trading.
 */
class CryptoAsset : public Asset {
public:
    /**
     * @brief Constructs a CryptoAsset and computes log-returns.
     * @param ticker    Exchange ticker symbol.
     * @param isin      ISIN identifier string.
     * @param name      Human-readable name.
     * @param currency  Denomination currency.
     * @param prices    Chronological closing prices.
     * @param dates     ISO-8601 date strings, same length as prices.
     */
    CryptoAsset(const std::string& ticker,
                const std::string& isin,
                const std::string& name,
                const std::string& currency,
                const std::vector<double>&      prices,
                const std::vector<std::string>& dates);

    /**
     * @brief Annualised volatility using a 365 calendar-day factor.
     * @return σ_daily × √365.
     */
    double calculateAnnualVolatility()     const override;

    /**
     * @brief Annualised expected return using a 365 calendar-day factor.
     * @return μ_daily × 365.
     */
    double calculateAnnualExpectedReturn() const override;
};

#endif // ASSET_H