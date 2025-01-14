# Trading System Latency Benchmark Tool

## Introduction
The benchmark.cpp implementation represents a comprehensive latency measurement tool designed for trading system. Its primary purpose is to evaluate and analyze the performance of various trading operations by measuring their execution latencies. The tool interfaces with a TradingManager program to conduct performance measurements across different trading operations including order placement, modification, cancellation, and market data processing.

## Purpose and Objectives
The main objectives of this benchmarking tool are:
- Measure and analyze latencies for critical trading operations
- Generate detailed performance metrics for system evaluation

## System Overview
The benchmark tool is designed to interact with a TradingManager application and measures five key types of latencies:
- Order placement latency
- Order modification latency
- Order cancellation latency
- Orderbook retrieval latency
- WebSocket message processing latency (market data updates)

## Core Components Analysis

### Utility Functions
- `getRandomInRange(int min, int max)`: Generates random numbers within specified bounds
- `getRandomText()`: Selects random trading instruments from a predefined list
- `executeCommand(const std::string& cmd)`: Executes shell commands and captures output
- `calculateLatencyAverage(const std::string& filename, int n)`: Processes CSV files to compute average latencies

### Order Management Functions

#### Order Placement
`Calculate_Order_Latency(int n, int delay, std::string filename)`
- Purpose: Measures latency for placing new orders
- Process:
  - Places n orders with specified parameters
  - Records latencies in CSV format
  - Calculates average placement latency
  - Supports configurable delays between operations

#### Order Modification
`Calculate_Modification_Latency(int delay, std::string filename)`
- Purpose: Measures order modification performance
- Features:
  - Modifies existing orders with new prices and amounts
  - Tracks modification latencies
  - Uses predefined constants NEW_PRICE and NEW_AMOUNT

#### Order Cancellation
`Calculate_Cancellation_Latency(int delay, std::string filename)`
- Purpose: Evaluates order cancellation performance
- Implementation:
  - Processes cancellations for existing orders
  - Records cancellation latencies
  - Supports configurable delays between operations

### Market Data Functions

#### Orderbook Processing
`Calculate_OrderBook_Latency(int numIterations, int delay, std::string filename)`
- Purpose: Measures orderbook retrieval performance
- Features:
  - Retrieves orderbook data with varying depths
  - Records retrieval latencies
  - Supports multiple iterations for statistical significance

#### WebSocket Processing
`Calculate_Web_Socket_Latency(int numIterations, int delay, std::string filename)`
- Purpose: Measures market data subscription and processing performance
- Implementation:
  - Subscribes to market data feeds
  - Measures message processing latencies in microseconds
  - Supports multiple subscription iterations

## Data Management

### File Operations
The system implements comprehensive file handling for:
- CSV generation for latency measurements
- Order ID management through text files
- Structured data output for analysis

### Data Formats
- Order placement data: Instrument, Price, Amount, Latency
- Modification data: OrderID, NewPrice, NewAmount, Latency
- Cancellation data: OrderID, CancellationLatency
- Orderbook data: Instrument, Depth, Latency
- WebSocket data: Timestamp, Instrument, UpdateNumber, Latency

## Implementation Details

### Error Handling
The implementation includes robust error handling for:
- File operations
- Command execution
- Data parsing
- Numeric conversions

### Performance Considerations
- Implements delays between operations to prevent system overload
- Uses efficient string processing for output parsing
- Provides configurable iteration counts for statistical significance
- Implements buffer management for command output capture

## Usage Interface
The tool provides a menu-driven interface with options for:
1. Order placement latency measurement
2. Orderbook latency measurement
3. Order modification latency measurement
4. Order cancellation latency measurement
5. WebSocket latency measurement
6. Order ID retrieval
7. Comprehensive benchmark suite execution

## Output and Reporting
The system generates:
- Detailed CSV files for each operation type
- Average latency calculations
- Comprehensive summary statistics
- Real-time progress reporting
- Aggregate performance metrics

## Results

Optimization was done on this tool to improve the performance of the trading system. The following table shows the latency values before and after optimization:

| Latency Type                | Before Optimization | After Optimization |
|-----------------------------|---------------------|---------------------|
| Orders Placed               | 502.89 ms           | 165.83 ms           |
| Orderbook / Market Data     | 506.03 ms           | 163.47 ms           |
| Modification                | 504.455 ms          | 166.886 ms          |
| Cancellation                | 493.326 ms          | 166.057 ms          |
| Web Socket Propagation Delay| 3744.423 µs (3.74 ms)| 2299.02 µs (2.3 ms) |
| Positions Latency           | 504.455 ms          | 144.32 ms        |
| Trading Loop Latency        | 501.86 ms           | 132.908 ms          |

The optimization has significantly reduced the latencies for all operations, leading to improved system performance and responsiveness.

In order to know more about optimization refer to [Optimization.md](Optimization.md)