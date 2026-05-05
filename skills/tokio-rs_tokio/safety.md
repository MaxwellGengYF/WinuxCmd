### 1. NEVER Block the Async Runtime with std::thread::sleep

**BAD:**
```rust
use std::thread::sleep;
use std::time::Duration;

#[tokio::main]
async fn main() {
    // This blocks ALL tasks on the current thread
    sleep(Duration::from_secs(10));
    println!("Done");
}
```

**GOOD:**
```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    sleep(Duration::from_secs(10)).await;
    println!("Done");
}
```

### 2. NEVER Use std::sync::Mutex Across .await Points

**BAD:**
```rust
use std::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    let guard = data.lock().unwrap();
    // This can cause deadlocks
    tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
    drop(guard);
}
```

**GOOD:**
```rust
use tokio::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    let mut guard = data.lock().await;
    *guard += 1;
    drop(guard);
}
```

### 3. NEVER Ignore Task Join Errors

**BAD:**
```rust
#[tokio::main]
async fn main() {
    let handle = tokio::spawn(async {
        panic!("Task panicked!");
    });
    // Ignoring the error - task panic is swallowed
    let _ = handle.await;
}
```

**GOOD:**
```rust
#[tokio::main]
async fn main() {
    let handle = tokio::spawn(async {
        panic!("Task panicked!");
    });
    
    match handle.await {
        Ok(_) => println!("Task completed"),
        Err(e) => eprintln!("Task failed: {:?}", e),
    }
}
```

### 4. NEVER Use Recursive Async Functions Without Bounding

**BAD:**
```rust
async fn recursive(n: u32) {
    if n > 0 {
        recursive(n - 1).await; // Stack overflow!
    }
}

#[tokio::main]
async fn main() {
    recursive(10000).await;
}
```

**GOOD:**
```rust
use tokio::task::yield_now;

async fn recursive(n: u32) {
    if n > 0 {
        yield_now().await; // Allow other tasks to run
        Box::pin(recursive(n - 1)).await;
    }
}

#[tokio::main]
async fn main() {
    recursive(100).await;
}
```

### 5. NEVER Share Mutable State Without Proper Synchronization

**BAD:**
```rust
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let mut counter = 0;
    let counter = Arc::new(counter);
    
    let handles: Vec<_> = (0..10).map(|_| {
        let counter = counter.clone();
        tokio::spawn(async move {
            // Data race!
            *counter += 1;
        })
    }).collect();
    
    for handle in handles {
        handle.await.unwrap();
    }
}
```

**GOOD:**
```rust
use tokio::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let counter = Arc::new(Mutex::new(0));
    
    let handles: Vec<_> = (0..10).map(|_| {
        let counter = counter.clone();
        tokio::spawn(async move {
            let mut guard = counter.lock().await;
            *guard += 1;
        })
    }).collect();
    
    for handle in handles {
        handle.await.unwrap();
    }
}
```
