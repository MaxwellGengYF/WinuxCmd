# Best Practices

```markdown
# Best Practices

## Use Sessions for Connection Pooling

```cpp
#include "requests/requests.h"

class ApiClient {
public:
    ApiClient(const std::string& base_url) 
        : base_url_(base_url) {
        // Configure session once
        session_.set_timeout(std::chrono::seconds(30));
        session_.set_headers({
            {"User-Agent", "MyApp/1.0"},
            {"Accept", "application/json"}
        });
    }
    
    nlohmann::json get_data(const std::string& endpoint) {
        auto response = session_.get(base_url_ + endpoint);
        response.raise_for_status();
        return response.json();
    }

private:
    requests::Session session_;
    std::string base_url_;
};
```

## Implement Retry Logic

```cpp
#include "requests/requests.h"
#include <chrono>
#include <thread>

class ResilientClient {
public:
    nlohmann::json get_with_retry(const std::string& url, int max_retries = 3) {
        for (int attempt = 1; attempt <= max_retries; ++attempt) {
            try {
                auto response = session_.get(url);
                response.raise_for_status();
                return response.json();
            } catch (const requests::TimeoutError& e) {
                if (attempt == max_retries) throw;
                std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
            } catch (const requests::HTTPError& e) {
                if (e.status_code() >= 500 && attempt < max_retries) {
                    std::this_thread::sleep_for(std::chrono::seconds(1 << attempt));
                    continue;
                }
                throw;
            }
        }
        throw std::runtime_error("Max retries exceeded");
    }

private:
    requests::Session session_;
};
```

## Proper Error Handling

```cpp
#include "requests/requests.h"
#include <spdlog/spdlog.h>

class SafeHttpClient {
public:
    std::optional<nlohmann::json> safe_get(const std::string& url) {
        try {
            auto response = session_.get(url);
            response.raise_for_status();
            return response.json();
        } catch (const requests::ConnectionError& e) {
            spdlog::error("Connection failed: {}", e.what());
            return std::nullopt;
        } catch (const requests::TimeoutError& e) {
            spdlog::warn("Request timed out: {}", e.what());
            return std::nullopt;
        } catch (const requests::HTTPError& e) {
            spdlog::error("HTTP error {}: {}", e.status_code(), e.what());
            return std::nullopt;
        }
    }

private:
    requests::Session session_;
};
```

## Streaming Large Files

```cpp
#include "requests/requests.h"
#include <fstream>
#include <filesystem>

class FileDownloader {
public:
    void download_file(const std::string& url, 
                       const std::filesystem::path& output_path) {
        auto response = session_.get(url, requests::stream(true));
        response.raise_for_status();
        
        std::ofstream file(output_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open output file");
        }
        
        size_t total_downloaded = 0;
        for (auto chunk : response.iter_content(8192)) {
            file.write(chunk.data(), chunk.size());
            total_downloaded += chunk.size();
            spdlog::info("Downloaded {} bytes", total_downloaded);
        }
    }

private:
    requests::Session session_;
};
```

## Custom Authentication

```cpp
#include "requests/requests.h"
#include <chrono>
#include <jwt-cpp/jwt.h>

class JwtAuthClient {
public:
    JwtAuthClient(const std::string& base_url, 
                  const std::string& secret)
        : base_url_(base_url), secret_(secret) {
        refresh_token();
    }
    
    nlohmann::json authenticated_get(const std::string& endpoint) {
        if (token_expired()) {
            refresh_token();
        }
        
        auto response = session_.get(
            base_url_ + endpoint,
            requests::headers({{"Authorization", "Bearer " + token_}})
        );
        response.raise_for_status();
        return response.json();
    }

private:
    void refresh_token() {
        auto token = jwt::create()
            .set_issuer("auth0")
            .set_type("JWS")
            .set_payload_claim("sample", jwt::claim(std::string("test")))
            .sign(jwt::algorithm::hs256{secret_});
        token_ = token;
        token_expiry_ = std::chrono::system_clock::now() + std::chrono::hours(1);
    }
    
    bool token_expired() const {
        return std::chrono::system_clock::now() >= token_expiry_;
    }
    
    requests::Session session_;
    std::string base_url_;
    std::string secret_;
    std::string token_;
    std::chrono::system_clock::time_point token_expiry_;
};
```
```