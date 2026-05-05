### Send + Sync Guarantees

Tokio ensures that all spawned tasks are `Send` and `Sync` safe.

```rust
use std::sync::Arc;
use tokio::sync::Mutex;

#[tokio::main]
async fn main() {
    let data = Arc::new(Mutex::new(0));
    
    // This works because Arc<Mutex<i32>> is Send + Sync
    let handle = tokio::spawn(async move {
        let mut guard = data.lock().await;
        *guard += 1;
    });
    
    handle.await.unwrap();
}
```

### Async Runtime Considerations

```rust
use tokio::runtime::Runtime;

fn main() {
    // Multi-threaded runtime (default)
    let rt = Runtime::new().unwrap();
    
    rt.block_on(async {
        println!("Running on multi-threaded runtime");
    });
    
    // Single-threaded runtime
    let rt = tokio::runtime::Builder::new_current_thread()
        .build()
        .unwrap();
    
    rt.block_on(async {
        println!("Running on single-threaded runtime");
    });
}
```

### Concurrent Access with RwLock

```rust
use tokio::sync::RwLock;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let data = Arc::new(RwLock::new(String::from("Hello")));
    
    // Multiple readers can access concurrently
    let reader1 = {
        let data = data.clone();
        tokio::spawn(async move {
            let read = data.read().await;
            println!("Reader 1: {}", *read);
        })
    };
    
    let reader2 = {
        let data = data.clone();
        tokio::spawn(async move {
            let read = data.read().await;
            println!("Reader 2: {}", *read);
        })
    };
    
    // Writer waits for all readers to finish
    let writer = {
        let data = data.clone();
        tokio::spawn(async move {
            let mut write = data.write().await;
            *write = String::from("World");
        })
    };
    
    reader1.await.unwrap();
    reader2.await.unwrap();
    writer.await.unwrap();
}
```

### Thread Safety with tokio::sync::Notify

```rust
use tokio::sync::Notify;
use std::sync::Arc;

#[tokio::main]
async fn main() {
    let notify = Arc::new(Notify::new());
    
    let waiter = {
        let notify = notify.clone();
        tokio::spawn(async move {
            notify.notified().await;
            println!("Received notification");
        })
    };
    
    let notifier = tokio::spawn(async move {
        tokio::time::sleep(tokio::time::Duration::from_secs(1)).await;
        notify.notify_one();
    });
    
    waiter.await.unwrap();
    notifier.await.unwrap();
}
```
