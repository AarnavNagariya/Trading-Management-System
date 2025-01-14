#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <array>
#include <memory>
#include <regex>

// Function to generate a random number between min and max (inclusive)
int getRandomInRange(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Function to generate a random instrument name
std::string getRandomText() {
    std::vector<std::string> texts = {
        "BTC-PERPETUAL", "ETH-PERPETUAL", "DOGE_USDC-PERPETUAL", "ADA_USDC-PERPETUAL"
    };
    int randomIndex = rand() % texts.size();
    return texts[randomIndex];
}

// Function to execute a command and get its output
std::string executeCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    
    return result;
}

// Function to extract the order latency from the TradingClient output
int extractOrderLatencyFromOutput(const std::string& output) {
    std::cout << "Raw order output: " << output << std::endl;  // Debug print
    size_t pos = output.find("Order placed Latency : ");
    if (pos != std::string::npos) {
        size_t startPos = pos + 23;  // Length of "Order placed Latency : "
        size_t endPos = output.find(" ms", startPos);
        if (endPos != std::string::npos) {
            std::string latencyStr = output.substr(startPos, endPos - startPos);
            try {
                return std::stoi(latencyStr);
            } catch (const std::exception& e) {
                std::cerr << "Error converting order latency string: " << e.what() << std::endl;
                return -1;
            }
        }
    }
    return -1;  // Return -1 if latency is not found
}


// Function to place an order and log the latency
void putOrderAndLogLatency(std::ofstream& outputFile) {
    std::string instrument = getRandomText();
    int price = 10000;
    int amount = 10;

    std::stringstream cmd;
    cmd << "(echo 1 && echo " << instrument 
        << " && echo " << price 
        << " && echo " << amount 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractOrderLatencyFromOutput(output);
        outputFile << instrument << "," << price << "," << amount << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Order placed for " << instrument << " with latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error executing order command: " << e.what() << std::endl;
    }
}

void Calculate_Order_Latency(int n = 100, int delay = 100) {
    std::ofstream orderFile("order_placed_latency.csv");
    

    // Write headers
    orderFile << "Instrument,Price,Amount,Latency(ms)\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    // Perform measurements
    const int NUM_ITERATIONS = n;  // Adjust as needed
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        // Measure order latency
        putOrderAndLogLatency(orderFile);
        
        
        // Add a small delay between measurements
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    orderFile.close();
    std::cout << "Measurements complete." << std::endl;

    chdir("..");
}

std::vector<std::string> extractOrderIDs(const std::string& output) {
    std::vector<std::string> orderIDs;
    std::regex orderID_regex("Order ID: \"([0-9]+)\"");
    
    std::string::const_iterator searchStart(output.cbegin());
    std::smatch matches;
    
    while (std::regex_search(searchStart, output.cend(), matches, orderID_regex)) {
        orderIDs.push_back(matches[1].str());
        searchStart = matches.suffix().first;
    }
    
    return orderIDs;
}

// Function to fetch and save order IDs
void fetchAndSaveOrderIDs(std::ofstream& outputFile) {
    std::stringstream cmd;
    cmd << "(echo 2 && sleep 1 && echo 9) | ./TradingClient";  // Select option 2, wait for output, then exit

    try {
        std::string output = executeCommand(cmd.str());
        std::cout << "Raw output from order list:\n" << output << std::endl;  // Debug print
        
        std::vector<std::string> orderIDs = extractOrderIDs(output);
        
        // Save order IDs to file
        for (const auto& id : orderIDs) {
            outputFile << id << "\n";
        }
        outputFile.flush();
        
        std::cout << "Found " << orderIDs.size() << " order IDs" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error fetching order IDs: " << e.what() << std::endl;
    }
}


void SaveOrderIds()
{
    std::ofstream orderIDFile("order_ids.txt");
    
    if (!orderIDFile) {
        std::cerr << "Failed to open order_ids.txt for writing." << std::endl;
        return ;
    }

    // Write header
    orderIDFile << "OrderID\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    // Fetch and save order IDs
    fetchAndSaveOrderIDs(orderIDFile);

    orderIDFile.close();
    std::cout << "Order IDs have been saved to order_ids.txt" << std::endl;

    chdir("..");
}

std::vector<std::string> readOrderIDs(const std::string& filename) {
    std::vector<std::string> orderIDs;
    std::ifstream file(filename);
    std::string line;
    
    // Skip header line
    std::getline(file, line);
    
    // Read order IDs
    while (std::getline(file, line)) {
        if (!line.empty()) {
            orderIDs.push_back(line);
        }
    }
    
    return orderIDs;
}

// Function to extract modification latency from output
int extractModificationLatencyFromOutput(const std::string& output) {
    std::cout << "Raw modification output: " << output << std::endl;  // Debug print
    size_t pos = output.find("Order Modified Latency : ");
    if (pos != std::string::npos) {
        size_t startPos = pos + 24;  // Length of "Order Modified Latency : "
        size_t endPos = output.find(" ms", startPos);
        if (endPos != std::string::npos) {
            std::string latencyStr = output.substr(startPos, endPos - startPos);
            try {
                return std::stoi(latencyStr);
            } catch (const std::exception& e) {
                std::cerr << "Error converting modification latency string: " << e.what() << std::endl;
                return -1;
            }
        }
    }
    return -1;  // Return -1 if latency is not found
}

// Function to modify an order and log the latency
void modifyOrderAndLogLatency(const std::string& orderID, std::ofstream& outputFile) {
    int newPrice = 1000;
    int newAmount = 10;

    std::stringstream cmd;
    cmd << "(echo 4 && echo " << orderID 
        << " && echo " << newPrice 
        << " && echo " << newAmount 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractModificationLatencyFromOutput(output);
        
        // Log the modification details and latency
        outputFile << orderID << "," << newPrice << "," << newAmount << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Order " << orderID << " modified with latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error modifying order: " << e.what() << std::endl;
    }
}

// Function to process all order modifications
void Calculate_Modification_Latency() {
    // Open files
    std::ofstream modificationFile("order_modification_latency.csv");
    
    if (!modificationFile) {
        std::cerr << "Failed to open order_modifications.txt for writing." << std::endl;
        return;
    }

    // Write header
    modificationFile << "OrderID,NewPrice,NewAmount,Latency(ms)\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        // Read order IDs from file
        std::vector<std::string> orderIDs = readOrderIDs("../order_ids.txt");
        
        if (orderIDs.empty()) {
            std::cerr << "No order IDs found in order_ids.txt" << std::endl;
            return;
        }

        std::cout << "Found " << orderIDs.size() << " orders to modify" << std::endl;

        // Process each order ID
        for (const auto& orderID : orderIDs) {
            modifyOrderAndLogLatency(orderID, modificationFile);
            // Add a small delay between modifications
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during order modifications: " << e.what() << std::endl;
    }

    modificationFile.close();
    chdir("..");
}


int extractCancellationLatencyFromOutput(const std::string& output) {
    std::cout << "Raw cancellation output: " << output << std::endl;  // Debug print
    size_t pos = output.find("Order Cancelled Latency : ");
    if (pos != std::string::npos) {
        size_t startPos = pos + 25;  // Length of "Order Cancelled Latency : "
        size_t endPos = output.find(" ms", startPos);
        if (endPos != std::string::npos) {
            std::string latencyStr = output.substr(startPos, endPos - startPos);
            try {
                return std::stoi(latencyStr);
            } catch (const std::exception& e) {
                std::cerr << "Error converting cancellation latency string: " << e.what() << std::endl;
                return -1;
            }
        }
    }
    return -1;  // Return -1 if latency is not found
}

// Function to cancel an order and log the latency
void removeOrderAndLogLatency(const std::string& orderID, std::ofstream& outputFile) {
    std::stringstream cmd;
    cmd << "(echo 5 && echo " << orderID 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractCancellationLatencyFromOutput(output);
        
        // Log the cancellation details and latency
        outputFile << orderID << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Order " << orderID << " cancelled with latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error cancelling order: " << e.what() << std::endl;
    }
}

// Function to process all order cancellations
void Calculate_Cancellation_Latency() {
    // Open files
    std::ofstream cancellationFile("order_cancel_latency.csv");
    
    if (!cancellationFile) {
        std::cerr << "Failed to open order_cancellations.txt for writing." << std::endl;
        return;
    }

    // Write header
    cancellationFile << "OrderID,CancellationLatency(ms)\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        // Read order IDs from file
        std::vector<std::string> orderIDs = readOrderIDs("../order_ids.txt");
        
        if (orderIDs.empty()) {
            std::cerr << "No order IDs found in order_ids.txt" << std::endl;
            return;
        }

        std::cout << "Found " << orderIDs.size() << " orders to cancel" << std::endl;

        // Process each order ID
        for (const auto& orderID : orderIDs) {
            removeOrderAndLogLatency(orderID, cancellationFile);
            // Add a small delay between cancellations
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during order cancellations: " << e.what() << std::endl;
    }

    cancellationFile.close();
    chdir("..");
}

int extractOrderbookLatencyFromOutput(const std::string& output) {
    std::cout << "Raw orderbook output: " << output << std::endl;  // Debug print
    size_t pos = output.find("Order Book Latency : ");
    if (pos != std::string::npos) {
        size_t startPos = pos + 20;  // Length of "Order Book Latency : "
        size_t endPos = output.find(" ms", startPos);
        if (endPos != std::string::npos) {
            std::string latencyStr = output.substr(startPos, endPos - startPos);
            try {
                return std::stoi(latencyStr);
            } catch (const std::exception& e) {
                std::cerr << "Error converting orderbook latency string: " << e.what() << std::endl;
                return -1;
            }
        }
    }
    return -1;  // Return -1 if latency is not found
}

// Function to get orderbook and log latency
void getOrderbookAndLogLatency(std::ofstream& outputFile) {
    std::string instrument = getRandomText();
    int depth = getRandomInRange(1, 5);

    std::stringstream cmd;
    cmd << "(echo 3 && echo " << instrument 
        << " && echo " << depth 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractOrderbookLatencyFromOutput(output);
        
        // Log the orderbook request details and latency
        outputFile << instrument << "," << depth << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Orderbook retrieved for " << instrument << " with depth " 
                  << depth << ", latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error getting orderbook: " << e.what() << std::endl;
    }
}

// Function to process multiple orderbook requests
void Calculate_OrderBook_Latency(int numIterations = 100) {
    // Open files
    std::ofstream orderbookFile("orderbook_latency.csv");
    
    if (!orderbookFile) {
        std::cerr << "Failed to open orderbook_latency.csv for writing." << std::endl;
        return;
    }

    // Write header
    orderbookFile << "Instrument,Depth,Latency(ms)\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        // Process multiple orderbook requests
        for (int i = 0; i < numIterations; ++i) {
            getOrderbookAndLogLatency(orderbookFile);
            // Add a small delay between requests
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during orderbook requests: " << e.what() << std::endl;
    }

    orderbookFile.close();
    chdir("..");
}

std::vector<int> extractMarketDataLatencies(const std::string& output) {
    std::vector<int> latencies;
    std::regex latency_regex("Market data processing Latency: (\\d+)µs");
    
    std::string::const_iterator searchStart(output.cbegin());
    std::smatch matches;
    
    while (std::regex_search(searchStart, output.cend(), matches, latency_regex)) {
        try {
            latencies.push_back(std::stoi(matches[1]));
        } catch (const std::exception& e) {
            std::cerr << "Error converting latency: " << e.what() << std::endl;
        }
        searchStart = matches.suffix().first;
    }
    
    return latencies;
}

// Function to subscribe to market data and log latencies
void subscribeMarketDataAndLogLatency(std::ofstream& outputFile) {
    std::string instrument = getRandomText();
    int duration = 1; // 1 second duration

    // Create command to subscribe to market data
    std::stringstream cmd;
    cmd << "(echo 7 && echo " << instrument 
        << " && echo " << duration 
        << " && sleep " << (duration + 1) // Sleep for duration + 1 second to ensure we capture all data
        << " && echo 9) | ./TradingClient";

    try {
        // Get timestamp for this subscription
        auto timestamp = std::chrono::system_clock::now();
        auto timestamp_t = std::chrono::system_clock::to_time_t(timestamp);
        std::string time_str = std::ctime(&timestamp_t);
        time_str.pop_back(); // Remove newline

        // Execute command and get output
        std::string output = executeCommand(cmd.str());
        std::cout << "Raw subscription output:\n" << output << std::endl;

        // Extract all latencies
        std::vector<int> latencies = extractMarketDataLatencies(output);

        // Log all latencies with timestamp and instrument
        for (size_t i = 0; i < latencies.size(); ++i) {
            outputFile << time_str << ","
                      << instrument << ","
                      << i + 1 << "," // Update number
                      << latencies[i] << "\n";
        }
        outputFile.flush();

        std::cout << "Captured " << latencies.size() 
                  << " market data updates for " << instrument << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during market data subscription: " << e.what() << std::endl;
    }
}

// Function to process multiple market data subscriptions
void Calculate_Market_Data_Latency(int numIterations) {
    // Open file for market data latencies
    std::ofstream marketDataFile("market_data_latencies.csv");
    
    if (!marketDataFile) {
        std::cerr << "Failed to open market_data_latencies.csv for writing." << std::endl;
        return;
    }

    // Write header
    marketDataFile << "Timestamp,Instrument,UpdateNumber,Latency(µs)\n";

    // Change to build directory
    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        // Process multiple market data subscriptions
        for (int i = 0; i < numIterations; ++i) {
            std::cout << "\nStarting subscription " << (i + 1) << " of " << numIterations << std::endl;
            subscribeMarketDataAndLogLatency(marketDataFile);
            // Add a small delay between subscriptions
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during market data subscriptions: " << e.what() << std::endl;
    }

    marketDataFile.close();
    chdir("..");
}

double calculateLatencyAverage(const std::string& filename,int n = 3) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return -1;
    }

    std::string line;
    std::getline(file, line);  // Skip the header line
    
    double totalLatency = 0.0;
    int count = 0;

    // Read the file line by line
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        
        // Extract columns from the CSV line
        std::vector<std::string> columns;
        while (std::getline(ss, value, ',')) {
            columns.push_back(value);
        }

        // Assuming the "Latency" column is present and is the second column (index 1)
        if (columns.size() > 1) {
            try {
                double latency = std::stod(columns[n]);  // Convert the latency value to double
                totalLatency += latency;
                count++;
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid latency value: " << columns[n] << std::endl;
                continue;
            }
        }
    }

    file.close();

    // Calculate and return the average latency
    if (count > 0) {
        return totalLatency / count;
    } else {
        std::cerr << "No data to calculate the average." << std::endl;
        return -1;
    }
}


int main() {

    // SaveOrderIds();

    // Calculate_Modification_Latency();

    // Calculate_Order_Latency(100);  // 100 iterations

    // Calculate_OrderBook_Latency(100);

    // Calculate_Cancellation_Latency();

    // Calculate_Market_Data_Latency(50);

    double LatencyAverage = calculateLatencyAverage("market_data_latencies.csv",3);

    std::cout << "Average Latency: " << LatencyAverage << "us" << std::endl;
    return   0;
}