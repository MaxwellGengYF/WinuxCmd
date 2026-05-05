```bash
cargo add tokio --features full
```

Here are 5 common usage patterns with Tokio:

### 1. Async TCP Echo Server

```rust
use tokio::net::TcpListener;
use tokio::io::{AsyncReadExt, AsyncWriteExt};

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let listener = TcpListener::bind("127.0.0.1:8080").await?;
    
    loop {
        let (mut socket, _) = listener.accept().await?;
        tokio::spawn(async move {
            let mut buf = [0; 1024];
            loop {
                let n = match socket.read(&mut buf).await {
                    Ok(0) => return,
                    Ok(n) => n,
                    Err(e) => {
                        eprintln!("failed to read; err = {:?}", e);
                        return;
                    }
                };
                if let Err(e) = socket.write_all(&buf[0..n]).await {
                    eprintln!("failed to write; err = {:?}", e);
                    return;
                }
            }
        });
    }
}
```

### 2. Async HTTP Client with reqwest

```rust
use reqwest;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let resp = reqwest::get("https://httpbin.org/ip")
        .await?
        .text()
        .await?;
    println!("Response: {}", resp);
    Ok(())
}
```

### 3. Concurrent Tasks with tokio::spawn

```rust
use tokio::time::{sleep, Duration};

#[tokio::main]
async fn main() {
    let handle1 = tokio::spawn(async {
        sleep(Duration::from_secs(1)).await;
        "Task 1 done"
    });
    
    let handle2 = tokio::spawn(async {
        sleep(Duration::from_secs(2)).await;
        "Task 2 done"
    });
    
    let result1 = handle1.await.unwrap();
    let result2 = handle2.await.unwrap();
    println!("{}, {}", result1, result2);
}
```

### 4. Async File I/O

```rust
use tokio::fs::File;
use tokio::io::AsyncWriteExt;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut file = File::create("hello.txt").await?;
    file.write_all(b"Hello, Tokio!").await?;
    println!("File written successfully");
    Ok(())
}
```

### 5. Timer and Interval

```rust
use tokio::time::{interval, Duration};

#[tokio::main]
async fn main() {
    let mut interval = interval(Duration::from_secs(1));
    
    for i in 0..5 {
        interval.tick().await;
        println!("Tick {}", i + 1);
    }
}
```
