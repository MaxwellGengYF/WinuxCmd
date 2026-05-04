# Lifecycle

### Construction with default values
```cpp
#[derive(Serialize, Deserialize)]
struct Server {
    #[serde(default = "default_host")]
    host: String,
    #[serde(default = "default_port")]
    port: u16,
}

fn default_host() -> String { "0.0.0.0".into() }
fn default_port() -> u16 { 8080 }

fn main() {
    // Construction from JSON
    let json = r#"{"host": "localhost"}"#;
    let server: Server = serde_json::from_str(json).unwrap();
    // port will be 8080
}
```

### Move semantics with serialization
```cpp
#[derive(Serialize, Deserialize)]
struct Data {
    value: String,
}

fn main() {
    let data = Data { value: "hello".into() };
    
    // Serialization borrows the data
    let json = serde_json::to_string(&data).unwrap();
    
    // Deserialization creates new ownership
    let new_data: Data = serde_json::from_str(&json).unwrap();
    
    // Original data still valid
    println!("{}", data.value);
}
```

### Resource management with large structures
```cpp
use std::fs::File;
use std::io::BufReader;

#[derive(Serialize, Deserialize)]
struct LargeDataset {
    records: Vec<Record>,
}

#[derive(Serialize, Deserialize)]
struct Record {
    id: u64,
    data: String,
}

fn load_large_file(path: &str) -> Result<LargeDataset, Box<dyn std::error::Error>> {
    let file = File::open(path)?;
    let reader = BufReader::new(file);
    let dataset: LargeDataset = serde_json::from_reader(reader)?;
    Ok(dataset)
}
```