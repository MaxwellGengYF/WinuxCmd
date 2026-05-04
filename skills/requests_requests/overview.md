# Overview

```markdown
# Overview


## What is Requests?

Requests is a high-level HTTP client library that simplifies making HTTP/1.1 requests. It provides an elegant, Python-inspired API for C++ developers, handling connection pooling, cookie persistence, SSL/TLS verification, and automatic content decoding.

## When to Use Requests

- **Simple HTTP interactions**: When you need to make GET, POST, PUT, DELETE requests without dealing with low-level socket programming
- **REST API clients**: Building clients that interact with web services
- **Web scraping**: Fetching web pages and handling cookies/sessions
- **File downloads**: Downloading files with streaming support
- **Authentication**: When you need Basic, Digest, or custom authentication schemes

## When NOT to Use Requests

- **Real-time applications**: For WebSocket or long-polling connections, use a dedicated library
- **High-performance servers**: For server-side HTTP handling, use a proper HTTP server library
- **Embedded systems**: The library has dependencies that may not be suitable for constrained environments
- **HTTP/2 or HTTP/3**: Requests only supports HTTP/1.1

## Key Design Principles

1. **Simplicity first**: The API is designed to be intuitive and readable
2. **Sensible defaults**: SSL verification is enabled by default, timeouts are configurable
3. **Resource management**: Sessions handle connection pooling automatically
4. **Error handling**: Exceptions for network errors, timeouts, and HTTP errors
5. **Extensibility**: Custom authentication, transport adapters, and hooks

## Core Components

```cpp
// Response object
struct Response {
    int status_code;
    std::string text;
    std::vector<uint8_t> content;
    Headers headers;
    std::string encoding;
    nlohmann::json json() const;
};

// Session for connection pooling
class Session {
public:
    Response get(const std::string& url, Options opts = {});
    Response post(const std::string& url, Options opts = {});
    // ... other HTTP methods
};

// Options for request customization
struct Options {
    std::chrono::milliseconds timeout;
    Headers headers;
    std::string auth_username;
    std::string auth_password;
    bool stream;
    nlohmann::json json_data;
    // ... other options
};
```
```