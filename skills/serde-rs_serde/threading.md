# Threading

### Serde types are `Send` and `Sync`
```cpp
use std::sync::Arc;
use std::thread;

#[derive(Serialize, Deserialize, Clone)]
struct Config {
    host: String,
    port: u16,
}

fn main() {
    let config = Arc::new(Config {
        host: "localhost".into(),
        port: 8080,
    });

    let mut handles = vec![];
    for _ in 0..4 {
        let config = Arc::clone(&config);
        handles.push(thread::spawn(move || {
            // Serialization is safe across threads
            let json = serde_json::to_string(&*config).unwrap();
            println!("{}", json);
        }));
    }
}
```

### Use `Mutex` for shared mutable state
```cpp
use std::sync::Mutex;

struct SharedState {
    config: Mutex<Config>,
}

impl SharedState {
    fn update_config(&self, json: &str) -> Result<(), Error> {
        let new_config: Config = serde_json::from_str(json)?;
        let mut config = self.config.lock().unwrap();
        *config = new_config;
        Ok(())
    }
}
```

### Thread-local deserialization for performance
```cpp
use std::cell::RefCell;

thread_local! {
    static BUFFER: RefCell<String> = RefCell::new(String::with_capacity(1024));
}

fn deserialize_thread_local(json: &str) -> Result<MyStruct, Error> {
    BUFFER.with(|buf| {
        let mut buffer = buf.borrow_mut();
        buffer.clear();
        buffer.push_str(json);
        serde_json::from_str(&buffer)
    })
}
```

### Avoid sharing mutable references across threads
```cpp
// BAD: Mutable reference shared across threads
fn bad_example(data: &mut MyStruct) {
    std::thread::spawn(move || {
        let json = serde_json::to_string(data).unwrap(); // Error: cannot move out of &mut
    });
}

// GOOD: Clone data for thread safety
fn good_example(data: &MyStruct) {
    let cloned = data.clone();
    std::thread::spawn(move || {
        let json = serde_json::to_string(&cloned).unwrap();
    });
}
```