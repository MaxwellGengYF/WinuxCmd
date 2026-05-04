# Lifecycle

```markdown
# Lifecycle Management

## Construction

```cpp
#include "requests/requests.h"

// Default construction
requests::Session session;

// Construction with configuration
requests::Session configured_session(
    requests::timeout(std::chrono::seconds(30)),
    requests::headers({{"User-Agent", "MyApp/1.0"}})
);

// Response objects are returned by value
auto response = requests::get("https://example.com");
```

## Destruction and Resource Cleanup

```cpp
#include "requests/requests.h"

void proper_cleanup() {
    {
        requests::Session session;
        auto response = session.get("https://example.com");
        // Use response
    } // Session destructor closes all connections
    
    {
        auto response = requests::get("https://example.com",
                                       requests::stream(true));
        // Must consume stream before destruction
        for (auto chunk : response.iter_content(1024)) {
            // Process chunk
        }
    } // Response destructor releases connection back to pool
}
```

## Move Semantics

```cpp
#include "requests/requests.h"
#include <vector>

// Sessions are moveable
requests::Session create_session() {
    requests::Session session;
    session.set_timeout(std::chrono::seconds(10));
    return session; // Move construction
}

// Store sessions in containers
std::vector<requests::Session> sessions;
sessions.push_back(requests::Session()); // Move insertion

// Move assignment
requests::Session session1, session2;
session1 = std::move(session2); // session2 is now empty
```

## Resource Management Patterns

```cpp
#include "requests/requests.h"
#include <memory>

// RAII wrapper for session management
class ConnectionPool {
public:
    ConnectionPool(size_t pool_size) {
        for (size_t i = 0; i < pool_size; ++i) {
            pool_.push_back(std::make_unique<requests::Session>());
        }
    }
    
    requests::Session& acquire() {
        if (pool_.empty()) {
            throw std::runtime_error("No available connections");
        }
        auto session = std::move(pool_.back());
        pool_.pop_back();
        return *session;
    }
    
    void release(std::unique_ptr<requests::Session> session) {
        pool_.push_back(std::move(session));
    }

private:
    std::vector<std::unique_ptr<requests::Session>> pool_;
};

// Usage
void process_requests() {
    ConnectionPool pool(5);
    
    auto& session = pool.acquire();
    auto response = session.get("https://example.com");
    // Session will be returned to pool when done
}
```

## Copy Semantics (Deleted)

```cpp
#include "requests/requests.h"

// Sessions cannot be copied
requests::Session session1;
// requests::Session session2 = session1; // Compile error: copy deleted

// Responses cannot be copied
auto response1 = requests::get("https://example.com");
// auto response2 = response1; // Compile error: copy deleted

// Must use move semantics
requests::Session session3 = std::move(session1); // OK
auto response3 = std::move(response1); // OK
```

## Connection Lifecycle

```cpp
#include "requests/requests.h"

void connection_lifecycle_example() {
    requests::Session session;
    
    // First request establishes connection
    auto response1 = session.get("https://example.com");
    // Connection is kept alive in the pool
    
    // Second request reuses the connection
    auto response2 = session.get("https://example.com/other");
    
    // Connection stays alive until:
    // 1. Session is destroyed
    // 2. Idle timeout expires
    // 3. Connection error occurs
    
    // Force connection close
    session.close(); // Closes all idle connections
}
```
```