### 1. Blocking the Async Runtime

**BAD:**
```rust
use std::thread::sleep;
use std::time::Duration;

#[tokio::main]
async fn main() {
    // This blocks the entire runtime!
    sleep(Duration::from_secs(5));
    println!("Done");
}
```

**GOOD:**
```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    // This yields control back to the runtime
    sleep(Duration::from_secs(5)).await;
    println!("Done");
}
```

### 2. Forgetting to Await Futures

**BAD:**
```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    // This does nothing - the future is never polled
    sleep(Duration::from_secs(1));
    println!("This prints immediately");
}
```

**GOOD:**
```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    sleep(Duration::from_secs(1)).await;
    println!("This prints after 1 second");
}
```

### 3. Holding Mutex Guards Across .await Points

**BAD:**
```rust
use tokio::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    let mut guard = data.lock().await;
    *guard += 1;
    // Holding guard across await - BAD!
    tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
    *guard += 1;
}
```

**GOOD:**
```rust
use tokio::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    {
        let mut guard = data.lock().await;
        *guard += 1;
    } // Guard dropped here
    tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
    let mut guard = data.lock().await;
    *guard += 1;
}
```

### 4. Using std::sync::Mutex in Async Context

**BAD:**
```rust
use std::sync::Mutex;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    // This can cause deadlocks in async context
    let guard = data.lock().unwrap();
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

### 5. Not Handling Task Cancellation

**BAD:**
```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    let handle = tokio::spawn(async {
        loop {
            // This task can't be cancelled gracefully
            sleep(Duration::from_secs(1)).await;
            println!("Working...");
        }
    });
    
    sleep(Duration::from_secs(3)).await;
    handle.abort(); // Task is cancelled, but no cleanup
}
```

**GOOD:**
```rust
use tokio::time::{sleep, Duration};
use tokio_util::sync::CancellationToken;

#[tokio::main]
async fn main() {
    let token = CancellationToken::new();
    let handle = tokio::spawn(async {
        loop {
            tokio::select! {
                _ = token.cancelled() => {
                    println!("Cleanup done");
                    return;
                }
                _ = sleep(Duration::from_secs(1)) => {
                    println!("Working...");
                }
            }
        }
    });
    
    sleep(Duration::from_secs(3)).await;
    token.cancel();
    handle.await.unwrap();
}
```

### 6. Creating Too Many Tasks

**BAD:**
```rust
#[tokio::main]
async fn main() {
    // Spawning 100,000 tasks can cause memory issues
    for i in 0..100_000 {
        tokio::spawn(async move {
            // Do nothing
        });
    }
}
```

**GOOD:**
```rust
use tokio::task::JoinSet;

#[tokio::main]
async fn main() {
    let mut set = JoinSet::new();
    for i in 0..100 {
        set.spawn(async move {
            // Do work
        });
    }
    
    while let Some(result) = set.join_next().await {
        // Handle results
    }
}
```
