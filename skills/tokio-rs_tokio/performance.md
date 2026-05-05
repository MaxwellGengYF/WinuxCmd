### Zero-Cost Abstractions

Tokio's async/await is a zero-cost abstraction - there's no runtime overhead for using async functions.

```rust
// This async function compiles to a state machine
async fn compute() -> i32 {
    let a = 1;
    let b = 2;
    a + b
}

#[tokio::main]
async fn main() {
    let result = compute().await;
    println!("Result: {}", result);
}
```

### Task Spawning Overhead

```rust
use tokio::time::{Instant, Duration};

#[tokio::main]
async fn main() {
    let start = Instant::now();
    
    // Spawning many tasks has overhead
    let handles: Vec<_> = (0..1000).map(|i| {
        tokio::spawn(async move {
            i * 2
        })
    }).collect();
    
    for handle in handles {
        handle.await.unwrap();
    }
    
    println!("Time: {:?}", start.elapsed());
}
```

### Using Bounded Channels

```rust
use tokio::sync::mpsc;

#[tokio::main]
async fn main() {
    // Bounded channel prevents unbounded memory growth
    let (tx, mut rx) = mpsc::channel(100);
    
    let producer = tokio::spawn(async move {
        for i in 0..1000 {
            if tx.send(i).await.is_err() {
                break;
            }
        }
    });
    
    let consumer = tokio::spawn(async move {
        while let Some(value) = rx.recv().await {
            // Process value
        }
    });
    
    producer.await.unwrap();
    consumer.await.unwrap();
}
```

### Optimization: Using tokio::pin!

```rust
use tokio::pin;

#[tokio::main]
async fn main() {
    let future = async {
        tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
        42
    };
    
    // Pin the future to avoid allocation
    pin!(future);
    let result = future.await;
    println!("Result: {}", result);
}
```

### Optimization: Using tokio::task::yield_now

```rust
use tokio::task::yield_now;

#[tokio::main]
async fn main() {
    for i in 0..100 {
        // Yield to allow other tasks to run
        yield_now().await;
        println!("Processing {}", i);
    }
}
```
