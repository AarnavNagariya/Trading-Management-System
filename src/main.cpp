#include <iostream>
#include <string>
#include <unordered_set>
#include <deque>
#include <queue>
#include <array>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/common/thread.hpp>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <immintrin.h>
#include <atomic>


#define CLIENT_ID "lCQBtKlm"
#define CLIENT_SECRET "9SqADBb7qhVSMRzFdLhX0SIT7s_9kiK5w8a3pIBJRS8"

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
using json = nlohmann::json;

//  Used by cURL to write the response from the server into a string.
size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

// Network optimization components
class ConnectionPool {
private:
    std::vector<CURL*> connections;
    std::mutex pool_mutex;
    size_t max_connections;

public:
    ConnectionPool(size_t max_size = 10) : max_connections(max_size) {
        for(size_t i = 0; i < max_size; ++i) {
            CURL* curl = curl_easy_init();
            if(curl) {
                setupCurlOptions(curl);
                connections.push_back(curl);
            }
        }
    }

    ~ConnectionPool() {
        for(auto curl : connections) {
            curl_easy_cleanup(curl);
        }
    }

    void setupCurlOptions(CURL* curl) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 60L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "gzip, deflate");
        curl_easy_setopt(curl, CURLOPT_HTTP_CONTENT_DECODING, 1L);
        curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip, deflate");
    }

    CURL* acquire() {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if(connections.empty()) return nullptr;
        
        CURL* conn = connections.back();
        connections.pop_back();
        return conn;
    }

    void release(CURL* conn) {
        std::lock_guard<std::mutex> lock(pool_mutex);
        if(connections.size() < max_connections) {
            connections.push_back(conn);
        } else {
            curl_easy_cleanup(conn);
        }
    }
};

class OrderBook {
private:
    std::map<double, double> bids;  // price -> volume
    std::map<double, double> asks;  // price -> volume

public:
    void update(const json& data) {
        if (data.contains("type") && data["type"] == "change") {
            // Process bids
            if (data.contains("bids")) {
                for (const auto& bid : data["bids"]) {
                    if (bid.is_array() && bid.size() >= 3) {
                        const auto& action = bid[0];
                        if (action == "delete") {
                            if (bid[1].is_number()) {
                                double price = bid[1].get<double>();
                                bids.erase(price);
                            }
                        } else {
                            // New or update
                            if (bid[1].is_number() && bid[2].is_number()) {
                                double price = bid[1].get<double>();
                                double volume = bid[2].get<double>();
                                if (volume > 0) {
                                    bids[price] = volume;
                                } else {
                                    bids.erase(price);
                                }
                            }
                        }
                    }
                }
            }

            // Process asks
            if (data.contains("asks")) {
                for (const auto& ask : data["asks"]) {
                    if (ask.is_array() && ask.size() >= 3) {
                        const auto& action = ask[0];
                        if (action == "delete") {
                            if (ask[1].is_number()) {
                                double price = ask[1].get<double>();
                                asks.erase(price);
                            }
                        } else {
                            // New or update
                            if (ask[1].is_number() && ask[2].is_number()) {
                                double price = ask[1].get<double>();
                                double volume = ask[2].get<double>();
                                if (volume > 0) {
                                    asks[price] = volume;
                                } else {
                                    asks.erase(price);
                                }
                            }
                        }
                    }
                }
            }
        } else if (data.contains("type") && data["type"] == "snapshot") {
            // Handle initial snapshot
            bids.clear();
            asks.clear();

            if (data.contains("bids")) {
                for (const auto& bid : data["bids"]) {
                    if (bid.is_array() && bid.size() >= 2 && 
                        bid[0].is_number() && bid[1].is_number()) {
                        double price = bid[0].get<double>();
                        double volume = bid[1].get<double>();
                        if (volume > 0) {
                            bids[price] = volume;
                        }
                    }
                }
            }

            if (data.contains("asks")) {
                for (const auto& ask : data["asks"]) {
                    if (ask.is_array() && ask.size() >= 2 &&
                        ask[0].is_number() && ask[1].is_number()) {
                        double price = ask[0].get<double>();
                        double volume = ask[1].get<double>();
                        if (volume > 0) {
                            asks[price] = volume;
                        }
                    }
                }
            }
        }

        // Print current state
        std::cout << "Current Order Book State:" << std::endl;
        std::cout << "Top Bids:" << std::endl;
        int count = 0;
        for (auto it = bids.rbegin(); it != bids.rend() && count < 5; ++it, ++count) {
            std::cout << "Price: " << it->first << ", Volume: " << it->second << std::endl;
        }

        std::cout << "\nTop Asks:" << std::endl;
        count = 0;
        for (auto it = asks.begin(); it != asks.end() && count < 5; ++it, ++count) {
            std::cout << "Price: " << it->first << ", Volume: " << it->second << std::endl;
        }
    }

    // Getter methods for SIMD processing if needed
    std::vector<double> getBidPrices() const {
        std::vector<double> prices;
        prices.reserve(bids.size());
        for (const auto& bid : bids) {
            prices.push_back(bid.first);
        }
        return prices;
    }

    std::vector<double> getBidVolumes() const {
        std::vector<double> volumes;
        volumes.reserve(bids.size());
        for (const auto& bid : bids) {
            volumes.push_back(bid.second);
        }
        return volumes;
    }

    std::vector<double> getAskPrices() const {
        std::vector<double> prices;
        prices.reserve(asks.size());
        for (const auto& ask : asks) {
            prices.push_back(ask.first);
        }
        return prices;
    }

    std::vector<double> getAskVolumes() const {
        std::vector<double> volumes;
        volumes.reserve(asks.size());
        for (const auto& ask : asks) {
            volumes.push_back(ask.second);
        }
        return volumes;
    }

};
// Rate Limiter
class RateLimiter {
private:
    std::deque<std::chrono::steady_clock::time_point> request_times;
    std::mutex mutex;
    const size_t max_requests;
    const std::chrono::seconds window;

public:
    RateLimiter(size_t max_req = 100, std::chrono::seconds win = std::chrono::seconds(1))
        : max_requests(max_req), window(win) {}

    bool shouldThrottle() {
        std::lock_guard<std::mutex> lock(mutex);
        auto now = std::chrono::steady_clock::now();
        
        while(!request_times.empty() && now - request_times.front() > window) {
            request_times.pop_front();
        }
        
        if(request_times.size() >= max_requests) {
            return true;
        }
        
        request_times.push_back(now);
        return false;
    }
};

// Circuit Breaker
class CircuitBreaker {
private:
    std::atomic<int> failure_count{0};
    std::atomic<bool> is_open{false};
    std::chrono::steady_clock::time_point last_failure_time;
    static constexpr int FAILURE_THRESHOLD = 5;
    static constexpr auto RESET_TIMEOUT = std::chrono::seconds(30);

public:
    template<typename Func>
    auto execute(Func operation) -> decltype(operation()) {
        if(is_open.load()) {
            auto now = std::chrono::steady_clock::now();
            if(now - last_failure_time < RESET_TIMEOUT) {
                throw std::runtime_error("Circuit breaker is open");
            }
            is_open.store(false);
        }
        
        try {
            auto result = operation();
            failure_count.store(0);
            return result;
        } catch(const std::exception& e) {
            last_failure_time = std::chrono::steady_clock::now();
            if(++failure_count >= FAILURE_THRESHOLD) {
                is_open.store(true);
            }
            throw;
        }
    }
};

// Optimized Trading Client
class TradingManager {
private:
    std::string clientId;
    std::string clientSecretId;
    std::string accessToken;
    const std::string baseUrl = "https://test.deribit.com/api/v2/";
    const std::string wsUrl = "wss://test.deribit.com/ws/api/v2/";
    std::unique_ptr<client> wsClient = std::make_unique<client>(); // 1. Unique Pointer Added
    websocketpp::connection_hdl hdl;
    std::unique_ptr<std::thread> wsThread = std::make_unique<std::thread>(); // 1. Unique Pointer Added

    std::atomic<bool> isConnected{false}; // 2. Atomic Added
    std::atomic<bool> shouldStop{false}; // 2. Atomic Added
    std::unordered_set<std::string> subscribed_instruments;
    static std::atomic<int> update_counter; // 2. Atomic Added
    
    
    std::unique_ptr<ConnectionPool> connPool;
    std::unique_ptr<RateLimiter> rateLimiter;
    std::unique_ptr<CircuitBreaker> circuitBreaker;
    OrderBook orderBook;
    
    
    // Thread Pool
    class ThreadPool {
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;

    public:
        ThreadPool(size_t threads) : stop(false) {
            for(size_t i = 0; i < threads; ++i) {
                workers.emplace_back([this] {
                    while(true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] { 
                                return stop || !tasks.empty(); 
                            });
                            if(stop && tasks.empty()) return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        template<class F>
        void enqueue(F&& f) {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                tasks.emplace(std::forward<F>(f));
            }
            condition.notify_one();
        }

        ~ThreadPool() {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                stop = true;
            }
            condition.notify_all();
            for(std::thread &worker: workers) {
                worker.join();
            }
        }
    };

    ThreadPool threadPool;

    // Optimized request sending
    std::string send_request(const std::string &endpoint, const json &payload, const std::string &token = "") {
        if(rateLimiter->shouldThrottle()) {
            throw std::runtime_error("Rate limit exceeded");
        }

        return circuitBreaker->execute([&]() {
            std::string readBuffer;
            CURL* curl = connPool->acquire();
            
            if(!curl) {
                throw std::runtime_error("No available connections");
            }

            CURLcode res;
            struct curl_slist *headers = NULL;
            
            try {
                std::string url = baseUrl + endpoint;
                curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(curl, CURLOPT_POST, 1L);

                std::string jsonStr = payload.dump();
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

                headers = curl_slist_append(headers, "Content-Type: application/json");
                if (!token.empty()) {
                    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
                }
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
                curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
                curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

                res = curl_easy_perform(curl);
                
                if (res != CURLE_OK) {
                    throw std::runtime_error(std::string("CURL Error: ") + curl_easy_strerror(res));
                }

                curl_slist_free_all(headers);
                connPool->release(curl);
                
                return readBuffer;
            } catch (...) {
                if(headers) curl_slist_free_all(headers);
                connPool->release(curl);
                throw;
            }
        });
    }

    // Optimized WebSocket message handling
    void ws_message(websocketpp::connection_hdl hdl, client::message_ptr msg) {
        static thread_local json parser;
        static thread_local std::string buffer;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        buffer = msg->get_payload();
        try {
            const auto& response = parser.parse(buffer);
            if (response.contains("params")) {
                threadPool.enqueue([this, response]() {
                    processWebSocketMessage(response);
                });
            }
        } catch (...) {
            parser = json();
            throw;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        std::cout << "Message processing latency: " << duration.count() << "µs" << std::endl;
    }

    void debugPrint(const json& j, const std::string& prefix = "") {
    std::cout << prefix << j.dump(2) << std::endl;
}

// Replace processWebSocketMessage with this version:
void processWebSocketMessage(const json& response) {
    try {
        update_counter++;
        std::cout << "Update #" << update_counter << std::endl;
        
        if (response.contains("params") && response["params"].contains("data")) {
            const auto& data = response["params"]["data"];
            // Debug print
            std::cout << "Received data structure:" << std::endl;
            debugPrint(data);
            processOrderBookData(data);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing WebSocket message: " << e.what() << std::endl;
    }
}

void processOrderBookData(const json& data) {
    try {
        orderBook.update(data);
    } catch (const std::exception& e) {
        std::cerr << "Error processing order book data: " << e.what() << std::endl;
    }
}

public:
    const std::string &getAccessToken() const
    {
        return accessToken;
    }
    TradingManager(const std::string &id, const std::string &secretId)
        : clientId(id), clientSecretId(secretId), 
          threadPool(std::thread::hardware_concurrency()) {
        
        connPool = std::make_unique<ConnectionPool>(10);
        rateLimiter = std::make_unique<RateLimiter>(100, std::chrono::seconds(1));
        circuitBreaker = std::make_unique<CircuitBreaker>();

       wsClient->clear_access_channels(websocketpp::log::alevel::all);
        wsClient->clear_error_channels(websocketpp::log::elevel::all);
        wsClient->init_asio();
        wsClient->set_open_handler(std::bind(&TradingManager::ws_onOpen, this, std::placeholders::_1));
        wsClient->set_message_handler(std::bind(&TradingManager::ws_message, this, std::placeholders::_1, std::placeholders::_2));
        wsClient->set_close_handler(std::bind(&TradingManager::ws_onClose, this, std::placeholders::_1));
    }
    // Destructor
    ~TradingManager()
    {
        if (wsThread->joinable())
        {
            wsClient->stop();
            wsThread->join();
        }

        if (isConnected)
        {
            wsClient->close(hdl, websocketpp::close::status::normal, "Closing connection");
        }

    }

    void ws_onOpen(websocketpp::connection_hdl hdl)
    {
        this->hdl = hdl;
        isConnected = true;
        std::cout << "WebSocket connection established." << std::endl;
    }

    void ws_onClose(websocketpp::connection_hdl hdl)
    {
        isConnected = false;
        std::cout << "WebSocket connection closed." << std::endl;
    }
    // Function to connect websocket
    void connectWebSocket() 
    {
        // Clean up the previous connection if any
        if (wsThread && wsThread->joinable()) {
            wsClient->stop();  // Stop the previous WebSocket client before reusing it
            wsThread->join();  // Join the previous thread
            std::cout << "Previous connection stopped and thread joined." << std::endl;
        }

        // Reinitialize wsClient to ensure it starts fresh for each connection attempt
        wsClient = std::make_unique<client>();  // Reset the WebSocket client object

        // Clear and reset all WebSocket settings
        wsClient->clear_access_channels(websocketpp::log::alevel::all);
        wsClient->clear_error_channels(websocketpp::log::elevel::all);
        wsClient->init_asio();
        wsClient->set_open_handler(std::bind(&TradingManager::ws_onOpen, this, std::placeholders::_1));
        wsClient->set_message_handler(std::bind(&TradingManager::ws_message, this, std::placeholders::_1, std::placeholders::_2));
        wsClient->set_close_handler(std::bind(&TradingManager::ws_onClose, this, std::placeholders::_1));

        websocketpp::lib::error_code ec;

        // Set TLS initialization handler
        wsClient->set_tls_init_handler([](websocketpp::connection_hdl hdl) -> websocketpp::lib::shared_ptr<boost::asio::ssl::context> {
            websocketpp::lib::shared_ptr<boost::asio::ssl::context> ctx = 
                websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
            try {
                ctx->set_verify_mode(boost::asio::ssl::context::verify_none);
            } catch (const std::exception &e) {
                std::cerr << "Error initializing SSL context: " << e.what() << std::endl;
            }
            return ctx;
        });

        // Create connection and check for errors
        client::connection_ptr con = wsClient->get_connection(wsUrl, ec);
        if (ec) {
            std::cerr << "WebSocket connection error: " << ec.message() << std::endl;
            return;
        }

        // Connect and start WebSocket client in a new thread
        wsClient->connect(con);
        wsThread = std::make_unique<std::thread>([this]() {
            try {
                std::cout << "WebSocket client running in a new thread." << std::endl;
                wsClient->run();
            } catch (const std::exception &e) {
                std::cerr << "Error in WebSocket thread: " << e.what() << std::endl;
            }
        });
    }


    // Function to send message through websocket
    void sendWebSocketMessage(const std::string &message)
    {
        if (isConnected)
        {
            wsClient->send(hdl, message, websocketpp::frame::opcode::text);
        }
        else
        {
            std::cerr << "Cannot send message. WebSocket not connected." << std::endl;
        }
    }
    // Function for Subscribing to orderBook
    void subOrderBook(const std::string &instrument, int duration_seconds)
    {
        std::cout << "Subscribed to:" << instrument << std::endl;
        subscribed_instruments.insert(instrument);
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "public/subscribe"},
            {"params", {{"channels", {"book." + instrument + ".100ms"}}}},
            {"id", 1}};
        sendWebSocketMessage(payload.dump());
        std::thread([this, duration_seconds]
                    {
            std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
            std::cout << "closing WebSocket connection after " << duration_seconds << " seconds." << std::endl;
            wsClient->close(hdl, 1000, "Closing after timeout"); })
            .detach();
    }
    // Function to show subscription
    void showSubscriptions()
    {
        std::cout << "Subscribed to:" << std::endl;
        for (const auto &instrument : subscribed_instruments)
        {
            std::cout << instrument << std::endl;
        }
    }
    // Function to authenticate and get accesstoken
    void authenticate()
    {
        json payload = {
            {"id", 0},
            {"method", "public/auth"},
            {"params", {{"grant_type", "client_credentials"}, {"client_id", clientId}, {"client_secret", clientSecretId}}},
            {"jsonrpc", "2.0"}};

        std::string res = send_request("public/auth", payload);
        auto responseJson = json::parse(res);
        if (responseJson.contains("result") && responseJson["result"].contains("access_token"))
        {
            accessToken = responseJson["result"]["access_token"];
            std::cout << "Access token retrieved successfully." << std::endl;
        }
        else
        {
            std::cerr << "Failed to authenticate." << std::endl;
            if (responseJson.contains("error"))
            {
                std::cerr << "Error Details: " << responseJson["error"].dump() << std::endl;
            }
        }
    }

    // For placing order
    void putOrder(const std::string &instrument, const std::string &accessToken, double price, double amount)
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/buy"},
            {"params", {
                           {"instrument_name", instrument},
                           {"type", "limit"},
                           {"price", price},
                           {"amount", amount},
                       }},
            {"id", 1}};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string response = send_request("private/buy", payload, accessToken);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

        if (!response.empty())
        {
            try
            {
                auto responseJson = json::parse(response);
                if (responseJson.contains("error") && responseJson["error"].contains("data") && responseJson["error"]["data"].contains("reason"))
                {
                    std::cerr << "Error Details: " << responseJson["error"]["data"]["reason"] << std::endl;
                }
                else if (responseJson.contains("message"))
                {
                    std::cerr << "Error Details: " << responseJson["message"] << std::endl;
                }
                else
                {
                    std::cout << "Order placed successfully." << std::endl;
                    std::cout << "Order placed Latency : " << duration.count() << " ms" << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "No response received or error occurred." << std::endl;
        }
    }
    // Function to get all orders
    void allOpenOrders()
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_open_orders"},
            {"params", {}},
            {"id", 2}};

        std::string res = send_request("private/get_open_orders", payload, accessToken);
        try
        {
            auto responseJson = json::parse(res);

            if (responseJson.contains("result"))
            {
                if (responseJson["result"].empty())
                {
                    std::cout << "No open orders found." << std::endl;
                }
                else
                {
                    auto orders = responseJson["result"];
                    std::cout << "All Open Orders in detail: " << orders.dump(4) << std::endl;
                    // Loop through orders safely
                    for (const auto &order : orders)
                    {

                        if (order.contains("order_id"))
                            std::cout << "Order ID: " << order["order_id"] << std::endl;

                        if (order.contains("instrument_name"))
                            std::cout << "Instrument: " << order["instrument_name"] << std::endl;

                        if (order.contains("price"))
                            std::cout << "Price: " << order["price"] << std::endl;

                        if (order.contains("quantity"))
                            std::cout << "Quantity: " << order["quantity"] << std::endl;
                        else
                            std::cout << "Quantity: N/A\n"
                                      << std::endl;
                    }
                }
            }
            else
            {
                std::cerr << "Failed to retrieve open orders: " << responseJson.dump(4) << std::endl;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "An error occurred: " << e.what() << std::endl;
        }
    }
    // Function to cancel order
    void removeOrder(const std::string &accesstoken, const std::string &orderId)
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/cancel"},
            {"params", {{"order_id", orderId}}},
            {"id", 3}};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string response = send_request("private/cancel", payload, accessToken);
        auto responseJson = json::parse(response);
        if (responseJson.contains("error"))
        {
            std::cerr << "Error cancelling order: " << responseJson["error"]["message"] << std::endl;
        }
        else
        {
            std::cout << "Cancelled Order: " << std::endl;
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "Order Cancelled Latency : " << duration.count() << " ms" << std::endl;
        }
    }
    // Function to modify order
    void modifyOrder(const std::string &accesstoken, const std::string &orderId, double newPrice, double newAmount)
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/edit"},
            {"params", {{"order_id", orderId}, {"price", newPrice}, {"amount", newAmount}}},
            {"id", 4}};
        
        auto start_time = std::chrono::high_resolution_clock::now();

        std::string response = send_request("private/edit", payload, accessToken);
        if (!response.empty())
        {
            try
            {
                auto responseJson = json::parse(response);
                if (responseJson.contains("error") && responseJson["error"].contains("data") && responseJson["error"]["data"].contains("reason"))
                {
                    std::cerr << "Error Details: " << responseJson["error"]["data"]["reason"] << std::endl;
                }
                else if (responseJson.contains("message"))
                {
                    std::cerr << "Error Details: " << responseJson["message"] << std::endl;
                }
                else
                {
                    std::cout << "Order modified successfully." << std::endl;
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    std::cout << "Order Modified Latency : " << duration.count() << " ms" << std::endl;
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Error parsing JSON response: " << e.what() << std::endl;
            }
        }
        else
        {
            std::cout << "No response received or error occurred." << std::endl;
        }
    }
    // Function to get orderbook
    void getOrderBook(const std::string &instrument, int depth)
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "public/get_order_book"},
            {"params", {{"instrument_name", instrument}, {"depth", depth}}},
            {"id", 5}};

        auto start_time = std::chrono::high_resolution_clock::now();

        std::string response = send_request("public/get_order_book", payload);
        auto responseJson = json::parse(response);

        if (responseJson.contains("result"))
        {
            const auto &result = responseJson["result"];
            std::cout << "Order Book for " << instrument << ":\n";
            // Print general details
            std::cout << result.dump(4) << std::endl;
            // Print bids
            if (result.contains("bids"))
            {
                std::cout << "\nBids:\n";
                for (const auto &bid : result["bids"])
                {
                    std::cout << "Price: " << bid[0] << ", Amount: " << bid[1] << '\n';
                }
            }
            // Print asks
            if (result.contains("asks"))
            {
                std::cout << "\nAsks:\n";
                for (const auto &ask : result["asks"])
                {
                    std::cout << "Price: " << ask[0] << ", Amount: " << ask[1] << '\n';
                }
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
            std::cout << "Order Book Latency : " << duration.count() << " ms" << std::endl;
        }
        else
        {
            std::cerr << "Failed to retrieve order book." << std::endl;
            if (responseJson.contains("error"))
            {
                std::cerr << "Error Details: " << responseJson["error"].dump() << std::endl;
            }
        }
    }
    // Function to get position
    void fetchPositions(const std::string &accessToken, const std::string &currency, const std::string &kind)
    {
        json payload = {
            {"jsonrpc", "2.0"},
            {"method", "private/get_positions"},
            {"params", {
                           {"currency", currency},
                           {"kind", kind},
                       }},
            {"id", 6}};
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string response = send_request("private/get_positions", payload, accessToken);
        auto end_time = std::chrono::high_resolution_clock::now();

        if (response.empty())
        {
            std::cout << "Currency is unavailable.\n";
            return;
        }
        else
        {
            try
            {
                auto responseJson = json::parse(response);

                if (responseJson.contains("result"))
                {
                    auto positions = responseJson["result"];
                    std::cout << positions.dump(4) << std::endl;
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    std::cout << "Positions Latency : " << duration.count() << " ms" << std::endl;
                }
                else if (responseJson.contains("error") && responseJson["error"].contains("data") && responseJson["error"]["data"].contains("reason"))
                {
                    std::cerr << "Error Details: " << responseJson["error"]["data"]["reason"] << std::endl;
                }
                else if (responseJson.contains("message"))
                {
                    std::cerr << "Error Details: " << responseJson["message"] << std::endl;
                }
                else
                {
                    std::cerr << "Unexpected response format: " << response << "\n";
                }
            }
            catch (const std::exception &e)
            {
                std::cerr << "Failed to parse response: " << e.what() << "\n";
                std::cerr << "Raw response: " << response << "\n";
            }
        }

    }
};
std::atomic<int> TradingManager::update_counter = 0;


int main()
{
    std::string clientId, clientSecret;

    // Input for public and private IDs
    // std::cout << "Enter your publicId: ";
    // std::cin >> clientId;
    // std::cout << "Enter your privateId: ";
    // std::cin >> clientSecret;

    clientId = CLIENT_ID;
    clientSecret = CLIENT_SECRET;

    // Creating client object
    TradingManager client(clientId, clientSecret);

    // Authenticating
    client.authenticate();

    // Check for successful authentication
    const std::string accessToken = client.getAccessToken();
    if (accessToken.empty())
    {
        std::cerr << "Access token not retrieved. Exiting." << std::endl;
        return 1;
    }

    // Main menu loop
    while (true)
    {
        int choice;
        
        std::cout << "\n====================================\n";
        std::cout << "       Trading Menu for Derebit       \n";
        std::cout << "====================================\n";
        std::cout << "\nSelect an option:\n";
        std::cout << "1. Place an Order\n";
        std::cout << "2. Get all Orders\n";
        std::cout << "3. Modify Order\n";
        std::cout << "4. Remove an Order\n";
        std::cout << "5. Get Order Book\n";
        std::cout << "6. Get Current Positions\n";
        std::cout << "7. Subscribe to an Orderbook\n";
        std::cout << "8. Show all subscriptions\n";
        std::cout << "9. Exit\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice))
        {
            std::cerr << "Invalid input. Please enter a number between 1 and 9.\n";
            std::cin.clear();                                                   // Clear error state
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
            continue;
        }

        switch (choice)
        {
        case 1:
        {
            // Place Order
            std::string instrument;
            double price, amount;
            std::cout << "Enter instrument name: ";
            std::cin >> instrument;
            std::cout << "Enter price: ";
            std::cin >> price;
            std::cout << "Enter amount: ";
            std::cin >> amount;

            if (!std::cin.fail())
            {
                client.putOrder(instrument, accessToken, price, amount);
            }
            else
            {
                std::cerr << "Invalid input for price or amount. Please try again.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            break;
        }
        case 2:
            // Get All Open Orders
            client.allOpenOrders();
            break;
        case 3:
        {
            // Modify Order
            std::string orderId;
            double price, amount;
            std::cout << "Enter orderId: ";
            std::cin >> orderId;
            std::cout << "Enter new price: ";
            std::cin >> price;
            std::cout << "Enter new amount: ";
            std::cin >> amount;

            if (!std::cin.fail())
            {
                client.modifyOrder(accessToken, orderId, price, amount);
            }
            else
            {
                std::cerr << "Invalid input for price or amount. Please try again.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            break;
        }
        case 4:
        {
            // Cancel Order
            std::string orderId;
            std::cout << "Enter order ID: ";
            std::cin >> orderId;
            client.removeOrder(accessToken, orderId);
            break;
        }
        case 5:
        {
            // Get Order Book
            std::string instrument;
            int depth;
            std::cout << "Enter instrument name: ";
            std::cin >> instrument;
            std::cout << "Enter depth: ";
            std::cin >> depth;

            if (!std::cin.fail())
            {
                client.getOrderBook(instrument, depth);
            }
            else
            {
                std::cerr << "Invalid input for depth. Please try again.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            break;
        }
        
        
        case 6:
        {
            // Get Positions
            std::string currency, kind;
            std::cout << "Enter currency (e.g., BTC, ETH, EUR): ";
            std::cin >> currency;
            std::cout << "Enter kind (e.g., future, option): ";
            std::cin >> kind;
            client.fetchPositions(accessToken, currency, kind);
            break;
        }
        case 7:
        {
            // Subscribe to Orderbook
            std::string instrument;
            int duration;
            std::cout << "Enter instrument name: ";
            std::cin >> instrument;
            std::cout << "Enter subscription duration (seconds): ";
            std::cin >> duration;

            if (!std::cin.fail())
            {
                client.connectWebSocket();
                std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for connection
                client.subOrderBook(instrument, duration);
                std::this_thread::sleep_for(std::chrono::seconds(duration));
            }
            else
            {
                std::cerr << "Invalid input for duration. Please try again.\n";
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            }
            
            break;
        }
        case 8:
            // Show all subscriptions
            client.showSubscriptions();
            break;

        case 9:
            // Exit
            std::cout << "Exiting program...\n";
            return 0;

        default:
            std::cerr << "Invalid choice. Please select a number between 1 and 9.\n";
            break;
        }
    }
    return 0;
}
