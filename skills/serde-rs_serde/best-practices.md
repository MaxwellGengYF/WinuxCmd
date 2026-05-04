# Best Practices

### Use explicit error handling
```cpp
use serde::{Deserialize, Serialize};
use serde_json::Error;

#[derive(Serialize, Deserialize)]
struct ApiResponse {
    success: bool,
    data: Option<serde_json::Value>,
}

fn parse_response(json: &str) -> Result<ApiResponse, Error> {
    serde_json::from_str(json)
}
```

### Prefer `#[serde(default)]` for optional fields
```cpp
#[derive(Deserialize)]
struct Config {
    #[serde(default = "default_host")]
    host: String,
    #[serde(default = "default_port")]
    port: u16,
    #[serde(default)]
    debug: bool,
}

fn default_host() -> String { "localhost".into() }
fn default_port() -> u16 { 8080 }
```

### Use `#[serde(rename_all = "camelCase")]` for API compatibility
```cpp
#[derive(Serialize, Deserialize)]
#[serde(rename_all = "camelCase")]
struct UserProfile {
    first_name: String,
    last_name: String,
    date_of_birth: String,
}
```

### Validate data after deserialization
```cpp
#[derive(Deserialize)]
struct EmailInput {
    email: String,
}

impl EmailInput {
    fn validate(&self) -> Result<(), String> {
        if !self.email.contains('@') {
            return Err("Invalid email".into());
        }
        Ok(())
    }
}
```

### Use `#[serde(deny_unknown_fields)]` for strict parsing
```cpp
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct StrictConfig {
    host: String,
    port: u16,
}
```