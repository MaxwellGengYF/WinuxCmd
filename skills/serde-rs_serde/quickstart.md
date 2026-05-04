# Quickstart

```toml
[dependencies]
serde = { version = "1.0", features = ["derive"] }
serde_json = "1.0"
```

```rust
use serde::{Deserialize, Serialize};
use serde_json;

// 1. Basic struct serialization/deserialization
#[derive(Serialize, Deserialize, Debug)]
struct Point {
    x: i32,
    y: i32,
}

fn basic_example() {
    let point = Point { x: 1, y: 2 };
    let json = serde_json::to_string(&point).unwrap();
    let deserialized: Point = serde_json::from_str(&json).unwrap();
}

// 2. Enum serialization
#[derive(Serialize, Deserialize, Debug)]
enum Status {
    Active,
    Inactive,
    Pending { reason: String },
}

fn enum_example() {
    let status = Status::Pending { reason: "review".into() };
    let json = serde_json::to_string(&status).unwrap();
}

// 3. Custom field names with rename
#[derive(Serialize, Deserialize, Debug)]
struct User {
    #[serde(rename = "user_id")]
    id: u64,
    #[serde(rename = "full_name")]
    name: String,
}

// 4. Optional fields with default
#[derive(Serialize, Deserialize, Debug)]
struct Config {
    #[serde(default = "default_timeout")]
    timeout: u64,
    #[serde(default)]
    enabled: bool,
}

fn default_timeout() -> u64 { 30 }

// 5. Flatten nested structures
#[derive(Serialize, Deserialize, Debug)]
struct Response {
    status: String,
    #[serde(flatten)]
    data: Data,
}

#[derive(Serialize, Deserialize, Debug)]
struct Data {
    value: i32,
    message: String,
}

// 6. Skip fields during serialization
#[derive(Serialize, Deserialize, Debug)]
struct SensitiveData {
    name: String,
    #[serde(skip_serializing)]
    password: String,
    #[serde(skip)]
    internal_id: u64,
}

fn main() {
    basic_example();
    enum_example();
}
```