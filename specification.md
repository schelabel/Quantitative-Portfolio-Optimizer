# Specification: Quantitative Portfolio Simulator & Optimizer

**Topic:** Quantitative Portfolio Simulator & Optimizer

## Description

A command-line tool that reads historical asset price data from CSV files, computes per-asset and portfolio-level risk and return metrics (expected return μ, standard deviation σ, Sharpe Ratio), derives optimal leverage via the Kelly Criterion, samples the Efficient Frontier by generating randomized portfolio weight combinations, and runs Monte Carlo simulations to forecast the distribution of future portfolio returns across different percentiles.

## Chosen Criteria & Implementation Plan

### 1. Heterogeneous Collection (Heterogene Sammlung)

I will define an abstract base class `Asset` with a virtual `calculateAnnualVolatility()` method. Two derived classes will override this: `TraditionalAsset`, which annualizes daily volatility using √252 (standard trading days), and `CryptoAsset`, which uses √365 (no market closures on weekends). The portfolio will be managed as a `std::vector<Asset*>`, making use of runtime polymorphism.

### 2. File I/O (File Ein/Ausgabe)

Historical price data will be read from CSV files to calculate the mean return (μ), standard deviation (σ), Sharpe Ratio, and optimal Kelly fraction for each asset. The program will also write the results of the Efficient Frontier sampling — 150 randomly generated weight combinations with their corresponding risk/return profiles — to an `output_frontier.csv` file for external plotting.

### 3. Operator Overloading (Operatorüberladen)

I will overload `operator<<` for the simulation results class to handle formatted console output. This will display the computed asset metrics (CAGR, Kelly) alongside portfolio-level metrics (μ, σ, Sharpe Ratio), and the five key Monte Carlo percentiles derived from 600 simulated portfolio return paths: Very Good (+2σ), Good (+1σ), Average (mean), Bad (−1σ), and Very Bad (−2σ).
