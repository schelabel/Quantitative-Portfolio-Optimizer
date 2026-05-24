# Quantitative Portfolio Simulator & Optimizer

A C++ based command-line tool designed to analyze historical asset prices, compute key financial metrics, and simulate optimal portfolio allocations. This project demonstrates quantitative finance concepts alongside modern Object-Oriented Programming (OOP) principles in C++.

## Overview

This engine reads historical price data from CSV files and evaluates both individual assets and entire portfolios. It calculates core risk and return metrics, derives optimal leverage using the Kelly Criterion, samples the Efficient Frontier, and runs Monte Carlo simulations to forecast the distribution of future portfolio returns.

## Key Features

* **Polymorphic Asset Modeling:** Uses a heterogeneous collection (abstract base classes and virtual functions) to manage different asset classes. It dynamically handles the distinct annualization factors of Traditional Assets (252 trading days) and Cryptocurrencies (365 trading days).
* **Risk & Return Metrics:** Calculates Expected Return (μ), Standard Deviation / Volatility (σ), and the Sharpe Ratio for individual assets and the aggregate portfolio.
* **Kelly Criterion Optimization:** Computes the optimal theoretical capital allocation fraction for each asset to maximize long-term growth.
* **Efficient Frontier Sampling:** Generates 150 randomized portfolio weight combinations and evaluates their risk/return profiles, outputting the results to `output_frontier.csv` for external plotting.
* **Monte Carlo Simulation:** Runs 600 simulated portfolio return paths to derive key future percentiles: Very Good (+2σ), Good (+1σ), Average (Mean), Bad (-1σ), and Very Bad (-2σ).
* **Robust I/O & Operator Overloading:** Features custom CSV parsing for historical data and overloaded stream operators (`<<`) for clean, formatted console reporting.

## Tech Stack

* **Language:** C++ (Standard Template Library)
* **Architecture:** Object-Oriented Programming (Inheritance, Polymorphism, Encapsulation)
* **Data Handling:** File I/O (CSV reading/writing)