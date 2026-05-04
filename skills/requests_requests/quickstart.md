# Quickstart

```markdown
# Quickstart Guide

## Basic GET Request
```cpp
#include <iostream>
#include <string>
#include "requests/requests.h"

int main() {
    // Simple GET request
    auto response = requests::get("https://httpbin.org/get");
    std::cout << "Status: " << response.status_code << std::endl;
    std::cout << "Body: " << response.text << std::endl;
    return 0;
}
```

## POST with JSON Data
```cpp
#include "requests/requests.h"
#include <nlohmann/json.hpp>

int main() {
    nlohmann::json data = {{"key", "value"}, {"name", "test"}};
    
    auto response = requests::post("https://httpbin.org/post", 
                                    requests::json(data));
    std::cout << response.text << std::endl;
    return 0;
}
```

## Using Authentication
```cpp
#include "requests/requests.h"

int main() {
    auto response = requests::get("https://httpbin.org/basic-auth/user/pass",
                                   requests::auth("user", "pass"));
    std::cout << "Status: " << response.status_code << std::endl;
    return 0;
}
```

## Session with Cookie Persistence
```cpp
#include "requests/requests.h"

int main() {
    requests::Session session;
    
    // First request sets cookies
    session.get("https://httpbin.org/cookies/set?name=value");
    
    // Subsequent requests use stored cookies
    auto response = session.get("https://httpbin.org/cookies");
    std::cout << response.text << std::endl;
    return 0;
}
```

## File Upload
```cpp
#include "requests/requests.h"
#include <fstream>

int main() {
    std::ifstream file("example.txt");
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    
    auto response = requests::post("https://httpbin.org/post",
                                    requests::files({{"file", content}}));
    std::cout << response.text << std::endl;
    return 0;
}
```

## Streaming Download
```cpp
#include "requests/requests.h"
#include <fstream>

int main() {
    auto response = requests::get("https://example.com/large-file.zip",
                                   requests::stream(true));
    
    std::ofstream out("output.zip", std::ios::binary);
    for (auto chunk : response.iter_content(8192)) {
        out.write(chunk.data(), chunk.size());
    }
    return 0;
}
```

## Custom Headers
```cpp
#include "requests/requests.h"

int main() {
    requests::Headers headers = {
        {"User-Agent", "MyApp/1.0"},
        {"Accept", "application/json"}
    };
    
    auto response = requests::get("https://httpbin.org/headers",
                                   requests::headers(headers));
    std::cout << response.text << std::endl;
    return 0;
}
```

## Timeout Configuration
```cpp
#include "requests/requests.h"

int main() {
    try {
        auto response = requests::get("https://httpbin.org/delay/5",
                                       requests::timeout(std::chrono::seconds(3)));
        std::cout << "Response received" << std::endl;
    } catch (const requests::TimeoutError& e) {
        std::cerr << "Request timed out: " << e.what() << std::endl;
    }
    return 0;
}
```
```