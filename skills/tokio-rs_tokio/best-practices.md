### 1. Use Structured Concurrency with JoinSet

```rust
use tokio::task::JoinSet;

#[tokio::main]
async fn main() {
    let mut set = JoinSet::new();
    
    for i in 0..10 {
        set.spawn(async move {
            // Do work
            i * 2
        });
    }
    
    let mut results = Vec::new();
    while let Some(result) = set.join_next().await {
        match result {
            Ok(value) => results.push(value),
            Err(e) => eprintln!("Task failed: {:?}", e),
        }
    }
    
    println!("Results: {:?}", results);
}
```

### 2. Use tokio::select! for Timeouts

```rust
use tokio::time::{sleep, timeout, Duration};

#[tokio::main]
async fn main() {
    let result = timeout(Duration::from_secs(5), async {
        // Some long operation
        sleep(Duration::from_secs(10)).await;
        "Done"
    }).await;
    
    match result {
        Ok(value) => println!("Completed: {}", value),
        Err(_) => println!("Operation timed out"),
    }
}
```

### 3. Use Channels for Task Communication

```rust
use tokio::sync::mpsc;

#[tokio::main]
async fn main() {
    let (tx, mut rx) = mpsc::channel(32);
    
    let producer = tokio::spawn(async move {
        for i in 0..10 {
            tx.send(i).await.unwrap();
        }
    });
    
    let consumer = tokio::spawn(async move {
        while let Some(value) = rx.recv().await {
            println!("Received: {}", value);
        }
    });
    
    producer.await.unwrap();
    consumer.await.unwrap();
}
```

### 4. Use RwLock for Read-Heavy Workloads

```rust
use tokio::sync::RwLock;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(RwLock::new(vec![1, 2, 3]));
    
    let readers: Vec<_> = (0..5).map(|i| {
        let data = data.clone();
        tokio::spawn(async move {
            let read = data.read().await;
            println!("Reader {}: {:?}", i, *read);
        })
    }).collect();
    
    let writer = tokio::spawn(async {
        let mut write = data.write().await;
        write.push(4);
    });
    
    for reader in readers {
        reader.await.unwrap();
    }
    writer.await.unwrap();
}
```

### 5. Use Instrumentation for Debugging

```rust
use tracing::{info, instrument};

#[instrument]
async fn process_data(data: &str) -> String {
    info!("Processing data");
    tokio::time::sleep(tokio::time::Duration::from_millis(100)).await;
    format!("Processed: {}", data)
}

#[tokio::main]
async fn main() {
    tracing_subscriber::fmt::init();
    
    let result = process_data("test").await;
    println!("{}", result);
}
```
