### Send + Sync guarantees
```rust
use std::thread;

// Messages are Send + Sync by default
fn send_message_across_threads() {
    let msg = MyMessage {
        data: "thread-safe".to_string(),
    };
    
    // Send to another thread
    let handle = thread::spawn(move || {
        let encoded = msg.encode_to_vec();
        encoded
    });
    
    let result = handle.join().unwrap();
}
```

### Arc for shared ownership
```rust
use std::sync::Arc;

fn shared_message() {
    let msg = Arc::new(MyMessage {
        data: "shared".to_string(),
    });
    
    let msg_clone = Arc::clone(&msg);
    let handle = thread::spawn(move || {
        // Read-only access is safe
        println!("{}", msg_clone.data);
    });
    
    println!("{}", msg.data);
    handle.join().unwrap();
}
```

### Mutex for mutable access
```rust
use std::sync::Mutex;

fn mutable_shared_message() {
    let msg = Arc::new(Mutex::new(MyMessage {
        data: "mutable".to_string(),
    }));
    
    let msg_clone = Arc::clone(&msg);
    let handle = thread::spawn(move || {
        let mut guard = msg_clone.lock().unwrap();
        guard.data = "modified".to_string();
    });
    
    handle.join().unwrap();
    let guard = msg.lock().unwrap();
    assert_eq!(guard.data, "modified");
}
```

### Async runtime considerations
```rust
use tokio::sync::Mutex;

async fn async_message_handling() {
    let msg = Arc::new(tokio::sync::Mutex::new(MyMessage {
        data: "async".to_string(),
    }));
    
    let msg_clone = Arc::clone(&msg);
    let handle = tokio::spawn(async move {
        let mut guard = msg_clone.lock().await;
        guard.data = "async-modified".to_string();
    });
    
    handle.await.unwrap();
    let guard = msg.lock().await;
    assert_eq!(guard.data, "async-modified");
}
```
