# Quantitative Portfolio Simulator & Optimizer

An institutional-grade C++ quantitative engine designed to map the Efficient Frontier, evaluate seven canonical portfolio archetypes, and execute stochastic Monte Carlo simulations via Geometric Brownian Motion (GBM).

This project bridges advanced Modern Portfolio Theory (MPT) with rigorous Object-Oriented Programming principles — engineered for precise capital allocation, leverage derivation, and long-run risk management.

---

## Overview

The engine ingests historical price data from CSV files to construct heterogeneous, multi-asset universes. It dynamically computes arithmetic returns, annualised volatility, and full covariance matrices across aligned time series — then brute-force samples 50,000 weighted portfolios to map the complete Efficient Frontier.

Using the Kelly Criterion and Itô's Lemma (volatility drag correction), the simulator derives theoretically optimal leverage and scales each archetype into realistic, leveraged stochastic forecasts with margin-call liquidation floors.

---

## The 7 Portfolio Archetypes

The engine explicitly constructs and evaluates seven canonical portfolio construction strategies:

| # | Archetype | Method |
|---|---|---|
| 1 | **Equal Weight** | Trivial 1/N benchmark |
| 2 | **Risk Parity** | Inverse volatility approximation of Equal Risk Contribution |
| 3 | **Minimum Variance** | Global variance minimisation |
| 4 | **Maximum Diversification** | Maximises weighted-vol / portfolio-vol ratio |
| 5 | **Target Volatility (10%)** | Frontier point closest to 10% annualised σ |
| 6 | **Maximum Sharpe (Tangency)** | Optimal risk-adjusted return |
| 7 | **Maximum Return** | Maximum unleveraged geometric mean (CAGR) |

---

## Key Features

- **High-Resolution Frontier Sampling** — Generates 50,000 randomised portfolios in milliseconds using the Largest Remainder Method for resolution scaling: 1% for small universes, 0.1% for large ones. Guarantees exact 100.0% capital allocation without floating-point drift. Outputs all sampled metrics to `output_frontier.csv` for external visualisation.

- **Stochastic Monte Carlo Forecasting** — Runs 10,000 GBM simulation paths per portfolio at the Half-Kelly leverage. Derives exact 1-year quantile forecasts: Very Good (+2σ), Good (+1σ), Average (median), Bad (−1σ), Very Bad (−2σ). Hard liquidation floor enforced at −100%.

- **Kelly Criterion & Volatility Drag** — Computes full Kelly leverage $L^* = (\mu - r_f) / \sigma^2$ and practical Half-Kelly ($L^*/2$) for each archetype. Explicitly contrasts arithmetic return ($\mu$) against true compounded growth (geometric mean $g = \mu - \frac{1}{2}\sigma^2$), making the leverage-drag tradeoff transparent.

- **Polymorphic Asset Modelling** — Abstract `Asset` base class with virtual dispatch to `TraditionalAsset` (252 trading days) and `CryptoAsset` (365 calendar days). Heterogeneous collections managed through raw polymorphic pointers with explicit RAII cleanup.

- **Synchronised Covariance Engine** — `Portfolio` finds the intersection of all asset date sets, aligns return vectors to a common timeline, and computes the full N×N sample covariance matrix with Bessel's correction. Itô correction applied per-asset to convert log-means to arithmetic means.

- **Operator-Overloaded Terminal Reports** — Overloaded `operator<<` on `SimulationResult` generates structured, institution-style console output per archetype: weights, Sharpe, Kelly leverage, base vs. leveraged metrics, geometric mean, and the five-scenario Monte Carlo forecast.

---

## Tech Stack

| Component | Detail |
|---|---|
| Language | C++17 |
| Core libraries | STL, `<random>`, `<filesystem>`, `<fstream>` |
| Architecture | OOP — Polymorphism, Encapsulation, Abstract Interfaces |
| Documentation | Doxygen (HTML) with `@brief`, `@param`, `@return` and inline formula comments |

---

## Project Structure

```
.
├── main.cpp          # Entry point — asset loading, frontier sampling, archetype evaluation
├── Asset.h/.cpp      # Abstract Asset hierarchy: TraditionalAsset, CryptoAsset
├── Portfolio.h/.cpp  # Covariance matrix, volatility, expected return, Kelly criterion
├── Simulator.h       # Monte Carlo engine + Efficient Frontier sampler
├── CSVHandler.h      # Yahoo Finance CSV parser
├── data/
│   ├── metadata.csv  # Ticker, ISIN, name, currency, type
│   └── prices/       # Per-asset price CSVs (Yahoo Finance export format)
├── output_frontier.csv  # Generated — all 50,000 sampled portfolios
└── docs/             # Generated — Doxygen HTML documentation
```

---

## Building & Running

Compile with C++17:

```bash
g++ -std=c++17 *.cpp -o main
```

Run:

```bash
./main
```

Generate documentation (requires [Doxygen](https://www.doxygen.nl) and optionally [Graphviz](https://graphviz.org)):

```bash
doxygen Doxyfile
# open docs/html/index.html
```

---

## Data Format

**`data/metadata.csv`** — one row per asset:

```
Ticker,ISIN,Name,Currency,Type
BTC-EUR,,Bitcoin,EUR,Crypto
VWCE.DE,IE00B3RBWM25,Vanguard FTSE All-World,EUR,Traditional
4GLD.DE,DE000A2TEJB5,EUWAX Gold II,EUR,Traditional
```

**`data/prices/<Ticker>.csv`** — Yahoo Finance export format. The parser locates the `Close` column dynamically; column order does not matter.

---

## Output Example

```
==========================================
 Risk Parity (Equal Risk Contribution)
==========================================
 BTC-EUR       11%
 EWG2          47%
 VWCE          42%
------------------------------------------
 Sharpe Ratio          1.03
 Full Kelly (L*)       7.46x
 Half Kelly (L*/2)     3.73x
------------------------------------------
 Base (1x)
  Return (mu)           18.09 %
  Volatility (sigma)    13.74 %
  Geometric Mean        13.64 %
------------------------------------------
 Leveraged (3.73x Half-Kelly)
  Return                56.54 %
  Volatility            51.26 %
------------------------------------------
 1-Year Monte Carlo Forecast
  +2σ  (Very Good)      335.49 %
  +1σ  (Good)           160.03 %
   μ   (Average)         55.39 %
  -1σ  (Bad)             -7.45 %
  -2σ  (Very Bad)        -43.92 %
==========================================
```

---

## Theoretical Background

The simulator is grounded in the following:

- **Markowitz (1952)** — Mean-variance optimisation as the foundation of the Efficient Frontier
- **Kelly (1956)** — Leverage derived from $L^* = (\mu - r_f) / \sigma^2$ to maximise geometric growth
- **Itô's Lemma** — Volatility drag correction: $g = \mu - \frac{1}{2}\sigma^2$ for the true compounded rate
- **Geometric Brownian Motion** — $dS = \mu S \, dt + \sigma S \, dW_t$ as the stochastic model for price paths
- **Choueifeng & Coignard (2008)** — Maximum Diversification Ratio as a distinct optimisation objective