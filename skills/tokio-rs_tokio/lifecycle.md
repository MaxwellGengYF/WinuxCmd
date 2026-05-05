### Construction: Creating Tokio Runtime

```rust
use tokio::runtime::Runtime;

// Manual construction
let rt = Runtime::new().unwrap();
rt.block_on(async {
    println!("Running in Tokio runtime");
});

// Using macro
#[tokio::main]
async fn main() {
    println!("Running with #[tokio::main]");
}
```

### Destruction: Dropping Runtime

```rust
use tokio::runtime::Runtime;

fn main() {
    let rt = Runtime::new().unwrap();
    
    rt.block_on(async {
        // Work here
    });
    
    // Runtime is dropped here, all tasks are cancelled
    drop(rt);
}
```

### Ownership: Moving Data into Tasks

```rust
use std::sync::Arc;
use tokio::sync::Mutex;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    
    let handle = tokio::spawn({
        let data = data.clone();
        async move {
            let mut guard = data.lock().await;
            *guard += 1;
        }
    });
    
    handle.await.unwrap();
    println!("Value: {}", *data.lock().await);
}
```

### Borrowing: Using References in Async Context

```rust
#[tokio::main]
async fn main() {
    let value = String::from("Hello");
    
    // Borrowing in async context
    let result = process(&value).await;
    println!("{}", result);
}

async fn process(s: &str) -> String {
    format!("Processed: {}", s)
}
```

### RAII Pattern: Using Guards

```rust
use tokio::sync::Mutex;
use std::sync::Arc;

struct Resource {
    data: Arc<Mutex<Vec<i32>>>,
}

impl Resource {
    fn new() -> Self {
        Self {
            data: Arc::new(Mutex::new(Vec::new())),
        }
    }
    
    async fn add(&self, value: i32) {
        let mut guard = self.data.lock().await;
        guard.push(value);
        // Guard is dropped here, releasing the lock
    }
}

#[tokio::main]
async fn main() {
    let resource = Resource::new();
    resource.add(42).await;
    println!("Added value");
}
```
