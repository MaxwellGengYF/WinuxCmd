# Performance

```markdown
# Performance


## Connection Pooling Benefits

```cpp
#include "requests/requests.h"
#include <chrono>
#include <iostream>

void benchmark_pooling() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Without pooling - creates new connection each time
    for (int i = 0; i < 10; ++i) {
        auto response = requests::get("https://example.com");
    }
    
    auto without_pool = std::chrono::high_resolution_clock::now() - start;
    
    start = std::chrono::high_resolution_clock::now();
    
    // With pooling - reuses connections
    requests::Session session;
    for (int i = 0; i < 10; ++i) {
        auto response = session.get("https://example.com");
    }
    
    auto with_pool = std::chrono::high_resolution_clock::now() - start;
    
    std::cout << "Without pooling: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(without_pool).count()
              << "ms\n";
    std::cout << "With pooling: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(with_pool).count()
              << "ms\n";
}
```

## Memory Allocation Patterns

```cpp
#include "requests/requests.h"
#include <vector>

// Pre-allocate buffers for better performance
void efficient_download() {
    requests::Session session;
    
    // Get content length first
    auto head_response = session.head("https://example.com/large-file");
    size_t content_length = std::stoul(head_response.headers["Content-Length"]);
    
    // Pre-allocate buffer
    std::vector<char> buffer;
    buffer.reserve(content_length);
    
    // Stream download into pre-allocated buffer
    auto response = session.get("https://example.com/large-file",
                                 requests::stream(true));
    for (auto chunk : response.iter_content(65536)) { // Larger chunks
        buffer.insert(buffer.end(), chunk.begin(), chunk.end());
    }
}
```

## Chunk Size Optimization

```cpp
#include "requests/requests.h"
#include <chrono>

void benchmark_chunk_sizes() {
    requests::Session session;
    
    // Test different chunk sizes
    std::vector<size_t> chunk_sizes = {1024, 4096, 16384, 65536, 262144};
    
    for (size_t chunk_size : chunk_sizes) {
        auto start = std::chrono::high_resolution_clock::now();
        
        auto response = session.get("https://example.com/large-file",
                                     requests::stream(true));
        std::vector<char> data;
        for (auto chunk : response.iter_content(chunk_size)) {
            data.insert(data.end(), chunk.begin(), chunk.end());
        }
        
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        std::cout << "Chunk size " << chunk_size << ": "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                  << "ms\n";
    }
}
```

## Compression Handling

```cpp
#include "requests/requests.h"

// Enable compression for bandwidth savings
void compressed_requests() {
    requests::Session session;
    
    // Request compressed content
    session.set_headers({
        {"Accept-Encoding", "gzip, deflate"}
    });
    
    auto response = session.get("https://example.com/large-response");
    // Library automatically decompresses
    
    std::cout << "Compressed size: " << response.headers["Content-Length"] << "\n";
    std::cout << "Actual size: " << response.content.size() << "\n";
}
```

## Parallel Requests

```cpp
#include "requests/requests.h"
#include <thread>
#include <vector>
#include <future>

void parallel_requests() {
    std::vector<std::string> urls = {
        "https://api1.example.com",
        "https://api2.example.com",
        "https://api3.example.com"
    };
    
    std::vector<std::future<nlohmann::json>> futures;
    
    for (const auto& url : urls) {
        futures.push_back(std::async(std::launch::async, [url]() {
            requests::Session session; // Each thread gets its own session
            auto response = session.get(url);
            return response.json();
        }));
    }
    
    for (auto& future : futures) {
        auto result = future.get();
        // Process result
    }
}
```

## Connection Limits

```cpp
#include "requests/requests.h"

void configure_connection_pool() {
    requests::Session session;
    
    // Configure connection pool
    session.set_pool_connections(10);  // Max connections in pool
    session.set_pool_maxsize(20);      // Max total connections
    
    // Adjust keep-alive settings
    session.set_keep_alive(true);
    session.set_keep_alive_timeout(std::chrono::seconds(30));
    
    // Disable keep-alive for one-off requests
    auto response = requests::get("https://example.com",
                                   requests::headers({
                                       {"Connection", "close"}
                                   }));
}
```
```