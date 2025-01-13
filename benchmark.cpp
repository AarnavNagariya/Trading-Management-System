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

# define PRICE 10000
# define AMOUNT 10
# define NEW_PRICE 2000
# define NEW_AMOUNT 20

int getRandomInRange(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Generate a random instrument name
std::string getRandomText() {
    std::vector<std::string> texts = {
        "BTC-PERPETUAL", "ETH-PERPETUAL", "ADA_USDC-PERPETUAL"
    };
    int randomIndex = rand() % texts.size();
    return texts[randomIndex];
}

// Execute a command in the terminal and return the output as a string.
std::string executeCommand(const std::string& cmd) {
    // Input arguments: cmd (const std::string&) - Command to execute
    // Output: (std::string) - Output of the command
    
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

// Calculate the average latency from a CSV file.
double calculateLatencyAverage(const std::string& filename,int n = 3) {
    // Input arguments: filename (const std::string&) - Name of the CSV file, n (int) - Column index of latency values
    // Output: (double) - Average latency

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return -1;
    }

    std::string line;
    std::getline(file, line);
    
    double totalLatency = 0.0;
    int count = 0;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;
        
        std::vector<std::string> columns;
        while (std::getline(ss, value, ',')) {
            columns.push_back(value);
        }

        // "Latency" column is present and in the nth column
        if (columns.size() > 1) {
            try {
                double latency = std::stod(columns[n]);
                totalLatency += latency;
                count++;
            } catch (const std::invalid_argument& e) {
                std::cerr << "Invalid latency value: " << columns[n] << std::endl;
                continue;
            }
        }
    }

    file.close();

    // Average latency
    if (count > 0) {
        return totalLatency / count;
    } else {
        std::cerr << "No data to calculate the average." << std::endl;
        return -1;
    }
}


// Extract the order latency from the TradingClient output.
int extractOrderLatencyFromOutput(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (int) - Order latency in milliseconds.

    std::cout << "Raw order output: " << output << std::endl;
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
    return -1;  // If latency is not found
}


// Place an order and log the latency to a CSV file.
void placeOrderAndLogLatency(std::ofstream& outputFile) {
    // Input arguments: outputFile (std::ofstream&) - Output file stream to write the data.

    std::string instrument = getRandomText();
    int price = PRICE;
    int amount = AMOUNT;

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

// Calculate order latency and stores the orders and latency in a CSV file.
void Calculate_Order_Latency(int n = 100, int delay = 100,std::string filename = "order_placed_latency.csv") {
    // Input arguments: n (int) - Number of iterations, delay (int) - Delay between measurements in milliseconds, filename (std::string) - Name of the output file.

    std::ofstream orderFile(filename);
    
    orderFile << "Instrument,Price,Amount,Latency(ms)\n"; // Headers

    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    const int NUM_ITERATIONS = n;
    
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        placeOrderAndLogLatency(orderFile);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay)); // Delay to wait and gaurantee the order is placed
    }

    orderFile.close();
    std::cout << "Measurements complete." << std::endl;

    chdir("..");


    double LatencyAverage = calculateLatencyAverage(filename,3);
    std::cout << "Average Order PLaced Latency: " << LatencyAverage << "ms" << std::endl;
}

// Extract order IDs from the TradingClient output.
std::vector<std::string> extractOrderIDs(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (std::vector<std::string>) - List of order IDs.

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

// Function to fetch and save order IDs to a txt file so that other test requiring order_ids can take it from there.
void fetchAndSaveOrderIDs(std::ofstream& outputFile) {
    // Input arguments: outputFile (std::ofstream&) - Output file stream to write the data.

    std::stringstream cmd;
    cmd << "(echo 2 && sleep 1 && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        std::cout << "Raw output from order list:\n" << output << std::endl;
        
        std::vector<std::string> orderIDs = extractOrderIDs(output);
        
        for (const auto& id : orderIDs) {
            outputFile << id << "\n";
        }
        outputFile.flush();
        
        std::cout << "Found " << orderIDs.size() << " order IDs" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error fetching order IDs: " << e.what() << std::endl;
    }
}

// Wrapper Function to save order IDs to a file.
void SaveOrderIds()
{
    std::ofstream orderIDFile("order_ids.txt");
    
    if (!orderIDFile) {
        std::cerr << "Failed to open order_ids.txt for writing." << std::endl;
        return ;
    }

    orderIDFile << "OrderID\n";

    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    fetchAndSaveOrderIDs(orderIDFile);

    orderIDFile.close();
    std::cout << "Order IDs have been saved to order_ids.txt" << std::endl;

    chdir("..");
}

// Function to read order IDs from a file. So that other tests can use it.
std::vector<std::string> readOrderIDs(const std::string& filename) {
    // Input arguments: filename (const std::string&) - Name of the file containing order IDs.
    // Output: (std::vector<std::string>) - List of order IDs.

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

// Extract modification latency from output
int extractModificationLatencyFromOutput(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (int) - Modification latency in milliseconds.

    std::cout << "Raw modification output: " << output << std::endl;
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
    return -1;  // If latency is not found
}

// To modify an order and log the latency
void modifyOrderAndLogLatency(const std::string& orderID, std::ofstream& outputFile) {
    // Input arguments: orderID (const std::string&) - Order ID to modify, outputFile (std::ofstream&) - Output file stream to write the data.

    int newPrice = NEW_PRICE;
    int newAmount = NEW_AMOUNT;

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

// Process all order modifications and log the latencies. Its like wrapper function to execute modifications.
void Calculate_Modification_Latency(int delay = 100,std::string filename = "order_modification_latency.csv") {
    // Input arguments: delay (int) - Delay between modifications in milliseconds, filename (std::string) - Name of the output file.

    std::ofstream modificationFile(filename);
    
    if (!modificationFile) {
        std::cerr << "Failed to open order_modifications.txt for writing." << std::endl;
        return;
    }

    modificationFile << "OrderID,NewPrice,NewAmount,Latency(ms)\n";

    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        std::vector<std::string> orderIDs = readOrderIDs("../order_ids.txt");
        
        if (orderIDs.empty()) {
            std::cerr << "No order IDs found in order_ids.txt" << std::endl;
            return;
        }

        std::cout << "Found " << orderIDs.size() << " orders to modify" << std::endl;

        // Processing on each order ID
        for (const auto& orderID : orderIDs) {
            modifyOrderAndLogLatency(orderID, modificationFile);

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small delay between modifications
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during order modifications: " << e.what() << std::endl;
    }

    modificationFile.close();
    chdir("..");

    double LatencyAverage = calculateLatencyAverage(filename,3);
    std::cout << "Average Modification Latency: " << LatencyAverage << "ms" << std::endl;
}

// Extract cancellation latency from output
int extractCancellationLatencyFromOutput(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (int) - Cancellation latency in milliseconds.

    std::cout << "Raw cancellation output: " << output << std::endl; 
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
    return -1;  // If latency is not found
}

// Function to cancel an order and log the latency
void cancelOrderAndLogLatency(const std::string& orderID, std::ofstream& outputFile) {
    // Input arguments: orderID (const std::string&) - Order ID to cancel, outputFile (std::ofstream&) - Output file stream to write the data.

    std::stringstream cmd;
    cmd << "(echo 5 && echo " << orderID 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractCancellationLatencyFromOutput(output);
        
        outputFile << orderID << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Order " << orderID << " cancelled with latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error cancelling order: " << e.what() << std::endl;
    }
}

// Process all order cancellations (Wrapper Function)
void Calculate_Cancellation_Latency(int delay = 100,std::string filename = "order_cancel_latency.csv") {
    // Input arguments: filename (std::string) - Name of the output file. delay (int) - Delay between cancellations in milliseconds.

    std::ofstream cancellationFile(filename);
    
    if (!cancellationFile) {
        std::cerr << "Failed to open order_cancellations.txt for writing." << std::endl;
        return;
    }

    cancellationFile << "OrderID,CancellationLatency(ms)\n"; // Headers

    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        std::vector<std::string> orderIDs = readOrderIDs("../order_ids.txt");
        
        if (orderIDs.empty()) {
            std::cerr << "No order IDs found in order_ids.txt" << std::endl;
            return;
        }

        std::cout << "Found " << orderIDs.size() << " orders to cancel" << std::endl;

        for (const auto& orderID : orderIDs) {
            cancelOrderAndLogLatency(orderID, cancellationFile);

            std::this_thread::sleep_for(std::chrono::milliseconds(delay)); // Small delay between cancellations
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during order cancellations: " << e.what() << std::endl;
    }

    cancellationFile.close();
    chdir("..");

    double LatencyAverage = calculateLatencyAverage(filename,1);
    std::cout << "Average Cancellation Latency: " << LatencyAverage << "ms" << std::endl;
}

// Extract orderbook latency from output
int extractOrderbookLatencyFromOutput(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (int) - Orderbook latency in milliseconds.

    std::cout << "Raw orderbook output: " << output << std::endl; 
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
    return -1;  // If latency is not found
}

// To get orderbook and log latency to a file
void getOrderbookAndLogLatency(std::ofstream& outputFile) {
    // Input arguments: outputFile (std::ofstream&) - Output file stream to write the data.

    std::string instrument = getRandomText();
    int depth = getRandomInRange(1, 5);

    std::stringstream cmd;
    cmd << "(echo 3 && echo " << instrument 
        << " && echo " << depth 
        << " && echo 9) | ./TradingClient";

    try {
        std::string output = executeCommand(cmd.str());
        int latency = extractOrderbookLatencyFromOutput(output);
        
        outputFile << instrument << "," << depth << "," << latency << "\n";
        outputFile.flush();
        
        std::cout << "Orderbook retrieved for " << instrument << " with depth " 
                  << depth << ", latency: " << latency << "ms" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error getting orderbook: " << e.what() << std::endl;
    }
}

// Wrapper function to process multiple orderbook requests
void Calculate_OrderBook_Latency(int numIterations = 100,int delay = 100, std::string filename = "orderbook_latency.csv") {
    // Input arguments: numIterations (int) - Number of iterations, delay (int) - Delay between requests in milliseconds, filename (std::string) - Name of the output file.

    std::ofstream orderbookFile(filename);
    
    if (!orderbookFile) {
        std::cerr << "Failed to open orderbook_latency.csv for writing." << std::endl;
        return;
    }

    orderbookFile << "Instrument,Depth,Latency(ms)\n";

    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        for (int i = 0; i < numIterations; ++i) {
            getOrderbookAndLogLatency(orderbookFile);

            std::this_thread::sleep_for(std::chrono::milliseconds(delay)); // Small delay between requests
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during orderbook requests: " << e.what() << std::endl;
    }

    orderbookFile.close();
    chdir("..");
    double LatencyAverage = calculateLatencyAverage(filename,2);
    std::cout << "Average Orderbook Latency: " << LatencyAverage << "ms" << std::endl;
}

// Extract Websocket Message latencies from output
std::vector<int> extractMarketDataLatencies(const std::string& output) {
    // Input arguments: output (const std::string&) - Output from TradingClient.
    // Output: (std::vector<int>) - List of latencies in microseconds.

    std::vector<int> latencies;
    std::regex latency_regex("Message processing latency: (\\d+)µs");
    
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
void subscribeMarketDataAndLogLatency(std::ofstream& outputFile,int suscribed_for = 1) {
    // Input arguments: outputFile (std::ofstream&) - Output file stream to write the data. suscribed_for (int) - Duration to subscribe in seconds.

    std::string instrument = getRandomText();
    int duration = suscribed_for; // Duration to subscribe

    std::stringstream cmd;
    cmd << "(echo 7 && echo " << instrument 
        << " && echo " << duration 
        << " && sleep " << (duration + 1) // Sleep for duration + 1 second to ensure we capture all data
        << " && echo 9) | ./TradingClient";

    try {
        auto timestamp = std::chrono::system_clock::now();
        auto timestamp_t = std::chrono::system_clock::to_time_t(timestamp);
        std::string time_str = std::ctime(&timestamp_t);
        time_str.pop_back(); 

        std::string output = executeCommand(cmd.str()); // Execute the command
        std::cout << "Raw subscription output:\n" << output << std::endl;

        // Extract all latencies
        std::vector<int> latencies = extractMarketDataLatencies(output);

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

// Wrapper function to process multiple market data subscriptions
void Calculate_Web_Socket_Latency(int numIterations=10,int delay = 100,std::string filename = "web_socket_latency.csv") {
    // Open file for market data latencies
    std::ofstream marketDataFile(filename);
    
    if (!marketDataFile) {
        std::cerr << "Failed to open web_socket_latency.csv for writing." << std::endl;
        return;
    }

    marketDataFile << "Timestamp,Instrument,UpdateNumber,Latency(µs)\n"; // Header


    if (chdir("build") != 0) {
        std::cerr << "Error: Failed to change directory to 'build'." << std::endl;
        return;
    }

    try {
        for (int i = 0; i < numIterations; ++i) {
            std::cout << "\nStarting subscription " << (i + 1) << " of " << numIterations << std::endl;
            subscribeMarketDataAndLogLatency(marketDataFile);

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Small delay between subscriptions
        }

    } catch (const std::exception& e) {
        std::cerr << "Error during market data subscriptions: " << e.what() << std::endl;
    }

    marketDataFile.close();
    chdir("..");

    double LatencyAverage = calculateLatencyAverage(filename,3);
    std::cout << "Average Web-Socket Message Latency: " << LatencyAverage << "us" << std::endl;
}


void displayMenu() {
    std::cout << "\nTrading System Latency Benchmark Tool\n";
    std::cout << "====================================\n";
    std::cout << "1. Measure Order Placement Latency\n";
    std::cout << "2. Measure Order Book Latency\n";
    std::cout << "3. Measure Order Modification Latency\n";
    std::cout << "4. Measure Order Cancellation Latency\n";
    std::cout << "5. Measure Web Socket Latency\n";
    std::cout << "6. Fetch Order IDs\n";
    std::cout << "7. Run All Benchmarks\n";
    std::cout << "8. Exit\n";
    std::cout << "Choice: ";
}

int main() {

    while (true)
    {
        displayMenu();
        int choice,n,n2;
        double Latency_Avg;
        double Total_avg;
        std::cin >> choice;
        switch (choice)
        {
        case 1:
            std::cout << "Enter number of orders to place: ";
            std::cin >> n;
            Calculate_Order_Latency(n);
            SaveOrderIds();
            break;
        case 2:
            std::cout << "Enter number of iterations to place: ";
            std::cin >> n;
            Calculate_OrderBook_Latency(n);
            break;
        case 3:
            Calculate_Modification_Latency();
            break;
        case 4:
            Calculate_Cancellation_Latency();
            SaveOrderIds();
            break;
        case 5:
            std::cout << "Enter number of iterations to place : ";
            std::cin >> n2;
            Calculate_Web_Socket_Latency(n2);
            break;
        case 6:
            SaveOrderIds();
            break;
        case 7:
            std::cout << "Enter number of orders to test upon: ";
            std::cin >> n;

            std::cout << "Enter number of iterations to place for Market Data subscription: ";
            std::cin >> n2;

            Calculate_Order_Latency(n);
            SaveOrderIds();
            Calculate_OrderBook_Latency(n);
            Calculate_Modification_Latency();
            Calculate_Cancellation_Latency();
            SaveOrderIds();
            Calculate_Web_Socket_Latency(n2);

            Total_avg = 0;
            std::cout << "Summary of all latencies\n" << std::endl;
            Latency_Avg = calculateLatencyAverage("order_placed_latency.csv",3);
            Total_avg += Latency_Avg;
            std::cout << "Average Order PLaced Latency: " << Latency_Avg << "ms" << std::endl;
            Latency_Avg = calculateLatencyAverage("orderbook_latency.csv",2);
            Total_avg += Latency_Avg;
            std::cout << "Average Orderbook Latency: " << Latency_Avg << "ms" << std::endl;
            Latency_Avg = calculateLatencyAverage("order_modification_latency.csv",3);
            Total_avg += Latency_Avg;
            std::cout << "Average Modification Latency: " << Latency_Avg << "ms" << std::endl;
            Latency_Avg = calculateLatencyAverage("order_cancel_latency.csv",1);
            Total_avg += Latency_Avg;
            std::cout << "Average Cancellation Latency: " << Latency_Avg << "ms" << std::endl;
            Latency_Avg = calculateLatencyAverage("web_socket_latency.csv",3);
            Total_avg += Latency_Avg/1000;
            std::cout << "Average Web Socket Latency: " << Latency_Avg << "us" << std::endl;
            Total_avg = Total_avg/5;
            std::cout << "Trading Loop Latency: " << Total_avg << "ms" << std::endl;

            break;
        case 8:
            return 0;
        default:
            std::cout << "Invalid choice\n";
            break;
        }
    }

    return 0;
}