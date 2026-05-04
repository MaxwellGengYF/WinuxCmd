# Threading

```markdown
# Thread Safety

## Thread Safety Guarantees

Requests provides the following thread safety guarantees:

- **Session objects are NOT thread-safe**: Each thread should have its own Session
- **Response objects are NOT thread-safe**: Access responses from a single thread
- **Free functions (get, post, etc.) ARE thread-safe**: They create temporary sessions
- **Connection pooling is thread-safe internally**: The underlying connection pool handles concurrent access

## Thread-Safe Usage Pattern

```cpp
#include "requests/requests.h"
#include <thread>
#include <vector>
#include <mutex>

// Thread-safe wrapper for session management
class ThreadSafeHttpClient {
public:
    nlohmann::json get(const std::string& url) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto response = session_.get(url);
        return response.json();
    }

private:
    requests::Session session_;
    std::mutex mutex_;
};

// Usage
void worker(ThreadSafeHttpClient& client, const std::string& url) {
    auto result = client.get(url);
    // Process result
}

int main() {
    ThreadSafeHttpClient client;
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back(worker, std::ref(client), "https://api.example.com");
    }
    
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
```

## Per-Thread Sessions (Recommended)

```cpp
#include "requests/requests.h"
#include <thread>
#include <vector>

// Each thread gets its own session - no synchronization needed
void thread_worker(int id) {
    requests::Session session;
    session.set_timeout(std::chrono::seconds(10));
    
    for (int i = 0; i < 10; ++i) {
        try {
            auto response = session.get("https://api.example.com/data");
            std::cout << "Thread " << id << " got response: " 
                      << response.status_code << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Thread " << id << " error: " << e.what() << std::endl;
        }
    }
}

int main() {
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(thread_worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    return 0;
}
```

## Thread Pool Pattern

```cpp
#include "requests/requests.h"
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>

class ThreadPool {
public:
    ThreadPool(size_t num_threads) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] {
                requests::Session session; // Each worker has its own session
                
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutex_);
                        condition_.wait(lock, [this] {
                            return stop_ || !tasks_.empty();
                        });
                        
                        if (stop_ && tasks_.empty()) return;
                        
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    
                    task(); // Execute with thread-local session
                }
            });
        }
    }
    
    template<typename F>
    void enqueue(F&& task) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            tasks_.emplace(std::forward<F>(task));
        }
        condition_.notify_one();
    }
    
    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            stop_ = true;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_ = false;
};

// Usage
void parallel_api_calls() {
    ThreadPool pool(4);
    
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i] {
            requests::Session session;
            auto response = session.get("https://api.example.com/endpoint/" + 
                                        std::to_string(i));
            std::cout << "Request " << i << " completed with status " 
                      << response.status_code << std::endl;
        });
    }
}
```

## Thread-Safe Response Processing

```cpp
#include "requests/requests.h"
#include <thread>
#include <mutex>
#include <vector>

class ThreadSafeResponseCollector {
public:
    void add_response(requests::Response response) {
        std::lock_guard<std::mutex> lock(mutex_);
        responses_.push_back(std::move(response));
    }
    
    void process_all() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& response : responses_) {
            // Process each response
            if (response.status_code == 200) {
                auto data = response.json();
                // Handle data
            }
        }
    }

private:
    std::vector<requests::Response> responses_;
    std::mutex mutex_;
};

void worker(ThreadSafeResponseCollector& collector, const std::string& url) {
    requests::Session session;
    auto response = session.get(url);
    collector.add_response(std::move(response));
}

int main() {
    ThreadSafeResponseCollector collector;
    std::vector<std::thread> threads;
    
    std::vector<std::string> urls = {
        "https://api1.example.com",
        "https://api2.example.com",
        "https://api3.example.com"
    };
    
    for (const auto& url : urls) {
        threads.emplace_back(worker, std::ref(collector), url);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    collector.process_all();
    return 0;
}
```
```