# Safety

### Condition 1: Never deserialize untrusted data without validation
```cpp
// BAD: Direct deserialization of user input
fn process_user_input(input: &str) {
    let data: MyStruct = serde_json::from_str(input).unwrap();
}

// GOOD: Validate after deserialization
fn process_user_input(input: &str) -> Result<(), Error> {
    let data: MyStruct = serde_json::from_str(input)?;
    if data.value < 0 || data.value > 100 {
        return Err(Error::InvalidRange);
    }
    Ok(())
}
```

### Condition 2: Never ignore deserialization errors
```cpp
// BAD: Silently ignoring errors
fn load_config(path: &str) -> Config {
    let content = std::fs::read_to_string(path).unwrap();
    serde_json::from_str(&content).unwrap_or(Config::default())
}

// GOOD: Handle errors explicitly
fn load_config(path: &str) -> Result<Config, Box<dyn std::error::Error>> {
    let content = std::fs::read_to_string(path)?;
    let config: Config = serde_json::from_str(&content)?;
    Ok(config)
}
```

### Condition 3: Never use `#[serde(flatten)]` with overlapping field names
```cpp
// BAD: Overlapping field names cause silent data loss
#[derive(Deserialize)]
struct Inner {
    name: String,
}

#[derive(Deserialize)]
struct Outer {
    name: String,
    #[serde(flatten)]
    inner: Inner,
}

// GOOD: Ensure unique field names
#[derive(Deserialize)]
struct Inner {
    inner_name: String,
}

#[derive(Deserialize)]
struct Outer {
    name: String,
    #[serde(flatten)]
    inner: Inner,
}
```

### Condition 4: Never serialize sensitive data without skipping
```cpp
// BAD: Password exposed in serialization
#[derive(Serialize)]
struct User {
    username: String,
    password: String,
}

// GOOD: Skip sensitive fields
#[derive(Serialize)]
struct User {
    username: String,
    #[serde(skip_serializing)]
    password: String,
}
```

### Condition 5: Never create recursive types without Box
```cpp
// BAD: Compile error or infinite recursion
#[derive(Serialize)]
struct LinkedList {
    value: i32,
    next: LinkedList,
}

// GOOD: Use Box for indirection
#[derive(Serialize)]
struct LinkedList {
    value: i32,
    next: Option<Box<LinkedList>>,
}
```