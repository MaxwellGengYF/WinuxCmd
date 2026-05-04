# Pitfalls

### 1. Recursive types without boxing
```cpp
// BAD: Infinite recursion in type resolution
#[derive(Serialize, Deserialize)]
struct Node {
    value: i32,
    children: Vec<Node>, // Recursive without indirection
}

// GOOD: Use Box for recursive types
#[derive(Serialize, Deserialize)]
struct Node {
    value: i32,
    children: Vec<Box<Node>>,
}
```

### 2. Missing default for optional fields
```cpp
// BAD: Deserialization fails if field missing
#[derive(Deserialize)]
struct Config {
    timeout: u64,
}

// GOOD: Provide default value
#[derive(Deserialize)]
struct Config {
    #[serde(default = "default_timeout")]
    timeout: u64,
}

fn default_timeout() -> u64 { 30 }
```

### 3. Flatten with field aliases
```cpp
// BAD: Aliases don't work with flatten
#[derive(Deserialize)]
struct Inner {
    #[serde(alias = "old_name")]
    name: String,
}

#[derive(Deserialize)]
struct Outer {
    #[serde(flatten)]
    inner: Inner,
}

// GOOD: Use rename instead of alias with flatten
#[derive(Deserialize)]
struct Inner {
    #[serde(rename = "name")]
    name: String,
}
```

### 4. Generic bounds with skip_serializing
```cpp
// BAD: T: Serialize bound still generated
#[derive(Serialize)]
struct Wrapper<T> {
    #[serde(skip_serializing)]
    value: T,
}

// GOOD: Use bound attribute to remove constraint
#[derive(Serialize)]
struct Wrapper<T> {
    #[serde(skip_serializing)]
    #[serde(bound = "")]
    value: T,
}
```

### 5. Stack overflow with large enums
```cpp
// BAD: Deeply nested enums cause stack overflow
#[derive(Serialize, Deserialize)]
enum DeepEnum {
    A(Box<DeepEnum>),
    B(Box<DeepEnum>),
    // ... many more variants
}

// GOOD: Limit nesting depth or use manual implementation
#[derive(Serialize, Deserialize)]
enum FlatEnum {
    A(i32),
    B(String),
    C { x: f64, y: f64 },
}
```

### 6. Trailing commas causing code generation issues
```cpp
// BAD: Trailing comma in attribute can cause compile errors
#[derive(Serialize)]
struct Point {
    x: i32,
    y: i32, // Trailing comma
}

// GOOD: Consistent comma style
#[derive(Serialize)]
struct Point {
    x: i32,
    y: i32
}
```