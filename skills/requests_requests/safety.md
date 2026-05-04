# Safety

```markdown
# Safety Guidelines

## RED LINE 1: Never Disable SSL Verification in Production

**NEVER** do this in production code:
```cpp
// DANGEROUS: Vulnerable to man-in-the-middle attacks
auto response = requests::get("https://bank.example.com",
                               requests::verify(false));
```

**ALWAYS** use proper certificate validation:
```cpp
// SAFE: Default SSL verification
auto response = requests::get("https://bank.example.com");

// Or with custom CA bundle
auto response = requests::get("https://internal.example.com",
                               requests::verify("/etc/ssl/certs/ca-bundle.crt"));
```

## RED LINE 2: Never Ignore Timeout Exceptions

**NEVER** catch and ignore timeout errors:
```cpp
// DANGEROUS: Silently ignoring timeouts
try {
    auto response = requests::get("https://slow.example.com",
                                   requests::timeout(std::chrono::seconds(1)));
} catch (const requests::TimeoutError&) {
    // Silently ignored - application appears to hang
}
```

**ALWAYS** handle timeouts properly:
```cpp
// SAFE: Proper timeout handling
try {
    auto response = requests::get("https://slow.example.com",
                                   requests::timeout(std::chrono::seconds(1)));
} catch (const requests::TimeoutError& e) {
    std::cerr << "Request timed out: " << e.what() << std::endl;
    // Implement retry logic or fallback
    return handle_timeout();
}
```

## RED LINE 3: Never Store Credentials in Code

**NEVER** hardcode authentication credentials:
```cpp
// DANGEROUS: Credentials exposed in source code
auto response = requests::get("https://api.example.com/secret",
                               requests::auth("admin", "P@ssw0rd!"));
```

**ALWAYS** use environment variables or secure storage:
```cpp
// SAFE: Credentials from environment
const char* username = std::getenv("API_USERNAME");
const char* password = std::getenv("API_PASSWORD");
if (!username || !password) {
    throw std::runtime_error("Missing API credentials");
}
auto response = requests::get("https://api.example.com/secret",
                               requests::auth(username, password));
```

## RED LINE 4: Never Ignore Response Status Codes

**NEVER** assume requests succeed:
```cpp
// DANGEROUS: No status check
auto response = requests::post("https://api.example.com/transfer",
                                requests::json({{"amount", 1000}}));
// Proceeds even if transfer failed
process_transaction(response.text);
```

**ALWAYS** validate responses:
```cpp
// SAFE: Validate before processing
auto response = requests::post("https://api.example.com/transfer",
                                requests::json({{"amount", 1000}}));
if (response.status_code != 200) {
    throw std::runtime_error("Transfer failed: " + 
                             std::to_string(response.status_code));
}
process_transaction(response.text);
```

## RED LINE 5: Never Use GET for State-Changing Operations

**NEVER** use GET for operations that modify state:
```cpp
// DANGEROUS: GET request deletes resources
auto response = requests::get("https://api.example.com/delete/user/123");
```

**ALWAYS** use appropriate HTTP methods:
```cpp
// SAFE: DELETE for deletion
auto response = requests::del("https://api.example.com/users/123");

// SAFE: POST for creation
auto response = requests::post("https://api.example.com/users",
                                requests::json({{"name", "new_user"}}));

// SAFE: PUT for updates
auto response = requests::put("https://api.example.com/users/123",
                               requests::json({{"name", "updated_name"}}));
```
```