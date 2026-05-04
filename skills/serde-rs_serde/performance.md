# Performance

### Use `serde_json::from_reader` for large files
```cpp
// BAD: Load entire file into memory
fn load_json_bad(path: &str) -> Result<MyStruct, Error> {
    let content = std::fs::read_to_string(path)?;
    serde_json::from_str(&content)
}

// GOOD: Stream from reader
fn load_json_good(path: &str) -> Result<MyStruct, Error> {
    let file = std::fs::File::open(path)?;
    let reader = std::io::BufReader::new(file);
    serde_json::from_reader(reader)
}
```

### Preallocate vectors when size is known
```cpp
#[derive(Serialize, Deserialize)]
struct Data {
    #[serde(default)]
    items: Vec<Item>,
}

// GOOD: Use with_capacity if you know approximate size
fn process_items(count: usize) -> Vec<Item> {
    let mut items = Vec::with_capacity(count);
    for i in 0..count {
        items.push(Item { id: i as u64 });
    }
    items
}
```

### Use `#[serde(flatten)]` sparingly
```cpp
// BAD: Flatten causes extra allocation and lookup
#[derive(Serialize, Deserialize)]
struct Response {
    status: String,
    #[serde(flatten)]
    data: HashMap<String, serde_json::Value>,
}

// GOOD: Explicit fields are faster
#[derive(Serialize, Deserialize)]
struct Response {
    status: String,
    value: i32,
    message: String,
}
```

### Avoid unnecessary allocations in hot paths
```cpp
// BAD: String allocation in hot loop
fn serialize_many(items: &[Item]) -> Vec<String> {
    items.iter()
        .map(|item| serde_json::to_string(item).unwrap())
        .collect()
}

// GOOD: Write to buffer directly
fn serialize_many(items: &[Item], writer: &mut impl std::io::Write) -> Result<(), Error> {
    for item in items {
        serde_json::to_writer(&mut *writer, item)?;
    }
    Ok(())
}
```