# Documentation of Trading Manager System

## Table of Contents

[Documentation of Trading Manager System](#documentation-of-trading-manager-system)
   - [Optimization Classes](#optimization-classes)
     - [ConnectionPool](#connectionpool)
       - [`ConnectionPool::ConnectionPool()`](#connectionpoolconnectionpool)
       - [`ConnectionPool::~ConnectionPool()`](#connectionpoolconnectionpool-1)
       - [Methods](#methods)
     - [OrderBook](#orderbook)
       - [Methods](#methods-1)
       - [Key Features](#key-features)
     - [RateLimiter](#ratelimiter)
       - [`RateLimiter(size_t max_req, std::chrono::seconds win)`](#ratelimitersize-t-max_req-stdchrono-seconds-win)
       - [Methods](#methods-2)
       - [Key Features](#key-features-1)
     - [CircuitBreaker](#circuitbreaker)
       - [Template Method](#template-method)
       - [Key Features](#key-features-2)
     - [ThreadPool](#threadpool)
       - [`ThreadPool::ThreadPool(size_t threads)`](#threadpoolthreadpoolsize-t-threads)
       - [`~ThreadPool()`](#threadpool)
       - [Methods](#methods-3)
       - [Key Features](#key-features-3)
   - [TradingManager: Helper Functions](#tradingmanager-helper-functions)
     - [Constructors and Destructors](#constructors-and-destructors)
     - [Class Overview](#class-overview)
     - [Core Methods](#core-methods)
       - [`send_request`](#send_request) 
       - [`on_message`](#on_message)
       - [`processWebSocketMessage`](#processwebsocketmessage)
       - [`processOrderBookData`](#processorderbookdata)
       - [`debugPrint`](#debugprint)
     - [Core Functionality](#core-functionality)
       - [WebSocket Management](#websocket-management)
       - [Subscription Management](#subscription-management)
       - [Authentication](#authentication)
     - [Core Features](#core-features)
   - [TradingManager: Wrapper Functions](#tradingmanager-wrapper-functions)
     - [`putOrder`](#putOrder)
     - [`allOpenOrders`](#allOpenOrders)
     - [`removeOrder`](#removeOrder)
     - [`modifyOrder`](#modifyorder)
     - [`getOrderBook`](#getorderbook)
     - [`fetchPositions`](#fetchPositions)

## Optimization Classes

### ConnectionPool
- **Purpose**: Manages a pool of reusable connections to trading venues, optimizing network operations and reducing the overhead of creating/destroying connections.
  
#### `ConnectionPool::ConnectionPool()`
- **Constructor**: Initializes a pool of `CURL` connections with predefined size and configurations.
  - **Parameters**:
    - `max_size` *(default: 10)*: Maximum number of connections in the pool.
  - **Operations**:
    - Allocates and initializes `CURL` handles.
    - Configures each handle with optimized options for connection timeout, keep-alive, and compression.

#### `ConnectionPool::~ConnectionPool()`
- **Destructor**: Cleans up all `CURL` connections in the pool to avoid memory leaks.

#### **Methods**:
- `setupCurlOptions(CURL* curl)`:
  - Configures common options for `CURL` handles, such as timeouts and compression settings.

- `CURL* acquire()`:
  - Retrieves a connection from the pool. Returns `nullptr` if the pool is empty.

- `void release(CURL* conn)`:
  - Returns a connection to the pool or cleans it up if the pool is full.

---

### OrderBook
- **Purpose**: Maintains and updates the bid and ask prices for an asset, providing a real-time view of the order book.

#### **Methods**:
- `void update(const json& data)`:
  - Updates the order book state based on incoming data (`snapshot` or `change`).
  - **Parameters**:
    - `data`: JSON object containing bid/ask updates or a full snapshot.
  - Handles additions, updates, and deletions of bids and asks.

- `std::vector<double> getBidPrices() / getBidVolumes() / getAskPrices() / getAskVolumes()`:
  - Returns vectors of bid/ask prices or volumes for analysis, potentially for SIMD optimization.

#### **Key Features**:
- Supports efficient updates and snapshot handling.
- Prints a summary of the top bids and asks for debugging or analysis purposes.

---

### RateLimiter
- **Purpose**: Controls the rate of requests sent to trading venues, preventing exceeding API rate limits.

#### `RateLimiter(size_t max_req, std::chrono::seconds win)`:
- **Constructor**: Initializes the rate limiter with maximum allowed requests and time window.
  - **Parameters**:
    - `max_req`: Maximum number of requests allowed within the specified window.
    - `win`: Time window for rate limiting.

#### **Methods**:
- `bool shouldThrottle()`:
  - Determines if the current request should be throttled.
  - Returns `true` if the request exceeds the rate limit.

#### **Key Features**:
- Automatically purges outdated request timestamps.
- Thread-safe implementation using a mutex.

---

### CircuitBreaker
- **Purpose**: Monitors the success and failure of operations, preventing cascading failures by temporarily disabling operations after repeated failures.

#### **Template Method**:
- `template<typename Func> auto execute(Func operation)`:
  - Executes the provided operation if the circuit breaker is closed.
  - If failures exceed the threshold, opens the circuit breaker and prevents further execution.

#### **Key Features**:
- Failure threshold: 5 consecutive failures.
- Reset timeout: 30 seconds after the last failure.
- Automatically resets after the timeout if no further failures occur.

---


### ThreadPool
- **Purpose**: Provides a mechanism to manage and execute tasks concurrently across multiple threads, optimizing CPU utilization and enabling efficient handling of asynchronous operations.

#### `ThreadPool::ThreadPool(size_t threads)`
- **Constructor**: Initializes the thread pool with a specified number of worker threads.
  - **Parameters**:
    - `threads`: Number of worker threads to create.
  - **Operations**:
    - Spawns the specified number of threads.
    - Each thread waits for tasks to be added to the task queue and executes them sequentially.

#### `~ThreadPool()`
- **Destructor**: Gracefully shuts down the thread pool.
  - Signals all threads to stop.
  - Waits for all worker threads to complete execution before destroying the thread pool.

#### **Methods**:
- **template<class F> void enqueue(F&& f)**:
  - Adds a new task to the task queue for execution by the worker threads.
  - **Parameters**:
    - `f`: A callable object (e.g., a lambda or function) representing the task to execute.
  - **Thread Safety**:
    - Uses a mutex to ensure safe access to the task queue.
    - Notifies one worker thread to pick up the new task.

---

#### **Key Features**:
1. **Dynamic Task Execution**:
   - Tasks are enqueued dynamically and processed by the next available thread.
   - Supports a wide range of callable types using templates.

2. **Thread Safety**:
   - Protects the task queue using a mutex.
   - Prevents data races when multiple threads interact with the shared queue.

3. **Graceful Shutdown**:
   - Ensures all tasks in the queue are completed before shutting down the thread pool.
   - Uses a condition variable to signal worker threads when tasks are available or when the thread pool is stopping.

4. **Efficient Resource Utilization**:
   - Enables concurrent execution of tasks, making full use of available CPU cores.
   - Reduces the overhead of frequent thread creation and destruction for each task.

---

## TradingManager : Helper Functions

The **TradingManager** class is a high-level component designed for managing trading operations. It facilitates secure communication with a trading platform's REST and WebSocket APIs, manages order book updates, and handles concurrency through robust threading and optimization mechanisms.

---

### **Constructors and Destructors**

1. **Constructor**:
   - **Signature**: `TradingManager(const std::string &id, const std::string &secretId)`
   - **Parameters**:
     - `id`: Client ID for authentication.
     - `secretId`: Client secret for authentication.
   - **Features**:
     - Initializes `clientId` and `clientSecretId`.
     - Sets up a thread pool with a size equal to the hardware concurrency.
     - Configures:
       - `ConnectionPool` (with a pool size of 10).
       - `RateLimiter` (100 requests per second).
       - `CircuitBreaker`.
     - Configures WebSocket client (`wsClient`) with appropriate handlers for open, message, and close events.

2. **Destructor**:
   - **Signature**: `~TradingManager()`
   - **Features**:
     - Stops the WebSocket client if running and joins the thread.
     - Closes WebSocket connection if connected.

---

### **Class Overview**
#### **Private Member Variables**
1. **Authentication Details**:
   - `clientId` & `clientSecretId`: Used for authenticating API requests.
   - `accessToken`: Token for secure communication with the API.

2. **API Endpoints**:
   - `baseUrl`: REST API base URL.
   - `wsUrl`: WebSocket API URL.

3. **WebSocket Management**:
   - `wsClient`: Manages WebSocket communication.
   - `hdl`: WebSocket connection handle.
   - `wsThread`: Dedicated thread for WebSocket operations.

4. **Concurrency & State Management**:
   - `isConnected` & `shouldStop`: Atomic flags for connection state.
   - `update_counter`: Atomic counter for tracking updates.
   - `threadPool`: A thread pool for offloading computationally intensive tasks.

5. **Utilities**:
   - `connPool`: Connection pool for managing CURL connections.
   - `rateLimiter`: Enforces request rate limits.
   - `circuitBreaker`: Prevents excessive retries during API failures.
   - `orderBook`: Maintains and updates the order book.

---

### **Core Methods**

#### `send_request`
- Sends a REST API request to the specified endpoint.
- **Parameters**:
  - `endpoint`: API endpoint relative to `baseUrl`.
  - `payload`: JSON payload for the request.
  - `token` (optional): Authorization token.
- **Features**:
  - Rate-limiting with `RateLimiter`.
  - Circuit-breaker protection using `CircuitBreaker`.
  - Connection pooling via `ConnectionPool`.
  - Error handling for CURL operations.

#### `on_message`
- Handles incoming WebSocket messages.
- **Parameters**:
  - `hdl`: WebSocket connection handle.
  - `msg`: Message received from the WebSocket.
- **Features**:
  - Parses JSON messages efficiently.
  - Offloads processing to `ThreadPool` for scalability.
  - Measures and logs processing latency.

#### `processWebSocketMessage`
- Processes WebSocket messages containing order book data.
- **Parameters**:
  - `response`: JSON message parsed from WebSocket.
- **Features**:
  - Updates `update_counter` for tracking.
  - Processes and debugs received order book data.
  - Catches and logs processing errors.

#### `processOrderBookData`
- Updates the order book with new data.
- **Parameters**:
  - `data`: JSON structure containing order book updates.
- **Features**:
  - Validates and applies updates to `OrderBook`.
  - Handles any exceptions during updates.

#### **debugPrint**
- Prints JSON structures with a debug prefix.
- **Parameters**:
  - `j`: JSON object to print.
  - `prefix`: Optional prefix for the debug message.

---

### **Core Functionality**

#### **WebSocket Management**

1. `connectWebSocket`:
   - **Purpose**: Establishes a WebSocket connection.
   - **Steps**:
     - Stops and cleans up any existing connection.
     - Reinitializes the WebSocket client (`wsClient`) and sets handlers.
     - Configures TLS for secure connections.
     - Creates and connects a WebSocket session.
     - Runs the WebSocket client in a separate thread.

2. `sendWebSocketMessage`:
   - **Purpose**: Sends a message over the WebSocket connection.
   - **Parameters**:
     - `message`: The message payload to send.
   - **Features**:
     - Ensures the WebSocket is connected before sending.
     - Logs an error if the WebSocket is not connected.

3. `ws_onOpen`:
   - **Purpose**: Handler triggered when the WebSocket connection is established.
   - **Parameters**:
     - `hdl`: WebSocket connection handle.
   - **Features**:
     - Sets the connection handle (`hdl`).
     - Marks the WebSocket as connected.

4. `ws_onClose`:
   - **Purpose**: Handler triggered when the WebSocket connection is closed.
   - **Parameters**:
     - `hdl`: WebSocket connection handle.
   - **Features**:
     - Marks the WebSocket as disconnected.

---

#### **Subscription Management**

1. `subOrderBook`:
   - **Purpose**: Subscribes to an order book for a specified instrument.
   - **Parameters**:
     - `instrument`: The instrument to subscribe to.
     - `duration_seconds`: Duration to maintain the subscription.
   - **Features**:
     - Sends a subscription request over WebSocket.
     - Starts a detached thread to close the connection after the specified duration.

2. `showSubscriptions`:
   - **Purpose**: Displays all current subscriptions.
   - **Features**:
     - Iterates through and prints all subscribed instruments.

---

#### **Authentication**

1. `authenticate`:
   - **Purpose**: Authenticates with the API and retrieves an access token.
   - **Steps**:
     - Sends a request with `clientId` and `clientSecretId` for authentication.
     - Parses the response to extract the `access_token`.
     - Logs success or failure with error details if authentication fails.

2. `getAccessToken`:
   - **Purpose**: Retrieves the stored access token.
   - **Return Type**: `const std::string&`
---

### Core Features

This trading client is designed for robust, high-performance interaction with the Deribit exchange. Key features include:

*   **Optimized API Interactions:** Combines rate limiting, connection pooling, and circuit breaking to ensure efficient and fault-tolerant REST API calls. This approach minimizes latency, maximizes throughput, and prevents cascading failures.
*   **Scalable WebSocket Processing:** Employs a thread pool for asynchronous handling of high-volume WebSocket messages. This ensures low latency, prevents blocking of the main thread, and maintains responsiveness.
*   **Real-Time Order Book Management:** Efficiently updates and maintains a local order book representation based on real-time market data received via WebSockets. This provides up-to-the-second market information.
*   **Secure Communication:** Implements secure WebSocket connections with TLS encryption and robust error handling to protect sensitive data and ensure connection stability.
*   **Robust Error Handling:** Provides comprehensive exception handling across all operations, including API requests, WebSocket communication, and order book updates, ensuring application stability and informative error reporting.
*   **Automated Subscription Management:** Streamlines the subscription and disconnection process for market data streams with clear logging for monitoring and debugging.
*   **Secure Authentication:** Implements a secure authentication mechanism to obtain access tokens for authorized API access, with detailed error handling for authentication failures.
*   **Thread Safety:** Utilizes atomic variables and appropriate locking mechanisms to guarantee thread safety and prevent race conditions in concurrent operations.

---


## TradingManager : Wrapper Functions

### `putOrder`

#### Description:
Places a limit order for a specified instrument with a given price and amount.

#### Parameters:
- `instrument` (const std::string&): The name of the instrument to trade.
- `accessToken` (const std::string&): The access token for authentication.
- `price` (double): The price at which to place the order.
- `amount` (double): The quantity of the instrument to trade.

#### Behavior:
- Constructs a JSON payload for the order request.
- Sends the request using `send_request` and measures latency.
- Parses the response to check for errors or success.

#### Output:
- Logs success or error details with latency for placing the order.

---

### `allOpenOrders`

#### Description:
Retrieves a list of all open orders.

#### Parameters:
None.

#### Behavior:
- Sends a request to fetch open orders.
- Parses the response and logs detailed information about each order, including:
  - Order ID
  - Instrument name
  - Price
  - Quantity

#### Output:
- Prints the details of all open orders or logs if no orders are found.

---

### `removeOrder`

#### Description:
Cancels an order using its ID.

#### Parameters:
- `accesstoken` (const std::string&): The access token for authentication.
- `orderId` (const std::string&): The ID of the order to cancel.

#### Behavior:
- Constructs a JSON payload for the cancel request.
- Sends the request and measures latency.
- Logs success or error details.

#### Output:
- Logs the success or failure of the cancellation operation along with latency.

---

### `modifyOrder`

#### Description:
Modifies an existing order by updating its price and amount.

#### Parameters:
- `accesstoken` (const std::string&): The access token for authentication.
- `orderId` (const std::string&): The ID of the order to modify.
- `newPrice` (double): The new price for the order.
- `newAmount` (double): The new quantity for the order.

#### Behavior:
- Constructs a JSON payload for the modification request.
- Sends the request and measures latency.
- Parses the response for success or error details.

#### Output:
- Logs success or failure messages, including latency.

---

### `getOrderBook`

#### Description:
Fetches the order book for a specific instrument up to a specified depth.

#### Parameters:
- `instrument` (const std::string&): The name of the instrument.
- `depth` (int): The depth of the order book to retrieve.

#### Behavior:
- Sends a request to fetch the order book.
- Parses and logs bid and ask details, along with general metadata.
- Measures and logs the latency.

#### Output:
- Prints detailed order book information or logs errors.

---

### `fetchPositions`

#### Description:
Retrieves the positions for a specified currency and kind.

#### Parameters:
- `accessToken` (const std::string&): The access token for authentication.
- `currency` (const std::string&): The currency to query.
- `kind` (const std::string&): The type of position (e.g., "option", "future").

#### Behavior:
- Sends a request to fetch positions and measures latency.
- Parses the response to log position details or error messages.

#### Output:
- Prints the position details or logs error messages.

---