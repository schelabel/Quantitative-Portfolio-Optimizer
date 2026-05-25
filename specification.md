# Specification: Quantitative Portfolio Simulator & Optimizer

**Topic:** Quantitative Portfolio Simulator & Optimizer

## Description

A command-line tool that reads historical asset price data from CSV files, computes risk and return metrics (expected return μ, standard deviation σ, Sharpe Ratio), evaluates 7 distinct portfolio strategies, derives portfolio-level optimal leverage via the Kelly Criterion, samples the Efficient Frontier, and runs Monte Carlo simulations (with historical and stress-tested correlations) to forecast the distribution of future portfolio returns.

## Chosen Criteria & Implementation Plan

### 1. Heterogeneous Collection (Heterogene Sammlung)

I will define an abstract base class `Asset` with a virtual `calculateAnnualVolatility()` method. Two derived classes will override this: `TraditionalAsset`, which annualizes daily volatility using √252 (standard trading days), and `CryptoAsset`, which uses √365 (no market closures on weekends). The portfolio will be managed as a `std::vector<Asset*>`, making use of runtime polymorphism.

### 2. File I/O (File Ein/Ausgabe)

Historical price data will be read from CSV files to calculate the mean return (μ), standard deviation (σ), and Sharpe Ratio for each asset. The program will also write the results of the Efficient Frontier sampling — alongside the risk, return, optimal Kelly leverage (L\*), and Geometric Mean of 7 key portfolio archetypes (e.g., Risk Parity, Target Volatility, Equal Weight) — to an `output_frontier.csv` file for external plotting.

### 3. Operator Overloading (Operatorüberladen)

I will overload `operator<<` for the simulation results class to handle formatted console output. This will display baseline asset metrics alongside the constructed portfolios' metrics (μp, σp, Sharpe Ratio, Half-Kelly L\*/2, and Geometric Mean). It will also format the five key Monte Carlo percentiles derived from simulated standard and stress-tested return paths: Very Good (+2σ), Good (+1σ), Average (mean), Bad (−1σ), and Very Bad (−2σ).