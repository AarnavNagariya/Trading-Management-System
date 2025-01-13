# Deribit Trading Management System

## Latency

To calculate this run `benchmark.cpp`. In this you can modify the parameters (instrumens,price,amount,iterations etc.) to run to calculate the latency. 

The latency is calculated by taking the difference between the time at which the request was sent and the time at which the response was received.

### Orders Placed

The output is stored in `order_placed_latency.csv`. Then the average is calculated.

The average latency for orders placed comes out to be `502.89 ms`.

### Modification 

The output is stored in `order_modification_latency.csv`. Then the average is calculated.

The average latency for modification comes out to be `504.455 ms`.

### Cancellation

The output is stored in `order_cancel_latency.csv`. Then the average is calculated.

The average latency for cancellation comes out to be `493.326 ms`.

### Orderbook / Market Data

The output is stored in `order_modification_latency.csv`. Then the average is calculated.
The output is stored in `orderbook_latency.csv`. Then the average is calculated.

The average latency for orderbook comes out to be `506.03 ms`.

### Web Socket - Message Propogation Delay

The output is stored in `web_socket_latency.csv`. Then the average is calculated.

The average latency from websocket comes out to be `744.423 µs` or `0.74 ms`.

### Positions Latency

The positions latency was found to be around `504.455 ms`.

### Trading Loop Latency

The average latency for trading loop comes out to be `501.86 ms`.

The sum of all the latencies comes out to be `2.58 s`.



Average Order PLaced Latency: 165.83ms
Average Orderbook Latency: 163.47ms
Average Modification Latency: 166.886ms
Average Cancellation Latency: 166.057ms
Average Web Socket Latency: 2299.02us
Trading Loop Latency: 132.908ms