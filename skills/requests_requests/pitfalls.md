# Pitfalls

```markdown
# Common Pitfalls

## 1. Not Handling Timeouts

**BAD**: No timeout set - request may hang indefinitely
```cpp
// This could hang forever if the server is slow
auto response = requests::get("https://example.com");
```

**GOOD**: Always set reasonable timeouts
```cpp
auto response = requests::get("https://example.com",
                               requests::timeout(std::chrono::seconds(10)));
```

## 2. Ignoring SSL Certificate Errors

**BAD**: Disabling SSL verification without understanding risks
```cpp
// Vulnerable to MITM attacks
auto response = requests::get("https://self-signed.badssl.com",
                               requests::verify(false));
```

**GOOD**: Handle certificate errors properly
```cpp
try {
    auto response = requests::get("https://self-signed.badssl.com");
} catch (const requests::SSLError& e) {
    std::cerr << "SSL verification failed: " << e.what() << std::endl;
    // Log and investigate, don't just disable verification
}
```

## 3. Memory Leaks with Streaming Responses

**BAD**: Not consuming streaming responses
```cpp
// Response body is never read, connection stays open
auto response = requests::get("https://example.com/large-file",
                               requests::stream(true));
// response goes out of scope without reading
```

**GOOD**: Always consume streaming responses
```cpp
{
    auto response = requests::get("https://example.com/large-file",
                                   requests::stream(true));
    std::string data;
    for (auto chunk : response.iter_content(8192)) {
        data.append(chunk.begin(), chunk.end());
    }
} // Response properly cleaned up
```

## 4. Not Checking Response Status

**BAD**: Assuming request always succeeds
```cpp
auto response = requests::get("https://example.com/nonexistent");
std::cout << response.text; // Might be error page
```

**GOOD**: Check status code or use raise_for_status
```cpp
auto response = requests::get("https://example.com/nonexistent");
if (response.status_code != 200) {
    std::cerr << "Request failed with status: " << response.status_code << std::endl;
    return;
}
// Or use:
response.raise_for_status(); // Throws on 4xx/5xx
```

## 5. Creating New Sessions for Every Request

**BAD**: No connection reuse
```cpp
for (int i = 0; i < 100; ++i) {
    // Creates new connection each time
    auto response = requests::get("https://api.example.com/data");
}
```

**GOOD**: Reuse session for connection pooling
```cpp
requests::Session session;
for (int i = 0; i < 100; ++i) {
    auto response = session.get("https://api.example.com/data");
}
```

## 6. Incorrect JSON Handling

**BAD**: Manual JSON string construction
```cpp
std::string json_str = "{\"name\": \"test\", \"value\": 42}";
auto response = requests::post("https://httpbin.org/post",
                                requests::body(json_str));
```

**GOOD**: Use proper JSON objects
```cpp
nlohmann::json data = {{"name", "test"}, {"value", 42}};
auto response = requests::post("https://httpbin.org/post",
                                requests::json(data));
```

## 7. Not Handling Redirects

**BAD**: Assuming no redirects occur
```cpp
auto response = requests::get("https://httpbin.org/redirect/3");
// May follow redirects unexpectedly
```

**GOOD**: Control redirect behavior
```cpp
auto response = requests::get("https://httpbin.org/redirect/3",
                               requests::allow_redirects(true),
                               requests::max_redirects(5));
```

## 8. File Descriptor Leaks

**BAD**: Not closing response objects
```cpp
for (int i = 0; i < 10000; ++i) {
    auto response = requests::get("https://example.com");
    // Response not properly cleaned up
}
```

**GOOD**: Ensure proper cleanup
```cpp
for (int i = 0; i < 10000; ++i) {
    {
        auto response = requests::get("https://example.com");
        // Use response
    } // Response destructor closes connection
}
```
```