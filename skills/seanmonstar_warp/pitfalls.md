**BAD: Using `.or()` without proper error handling**
```rust
let route1 = warp::path("a").map(|| "route a");
let route2 = warp::path("b").map(|| "route b");
// This will return 405 Method Not Allowed for unmatched paths
let routes = route1.or(route2);
```

**GOOD: Handle errors with `.recover()`**
```rust
let route1 = warp::path("a").map(|| "route a");
let route2 = warp::path("b").map(|| "route b");
let routes = route1.or(route2)
    .recover(|err: warp::Rejection| {
        Ok::<_, std::convert::Infallible>(warp::reply::with_status(
            "Not Found",
            warp::http::StatusCode::NOT_FOUND,
        ))
    });
```

**BAD: Blocking the async runtime with synchronous operations**
```rust
use std::thread;
use std::time::Duration;

let bad_route = warp::path("block")
    .map(|| {
        thread::sleep(Duration::from_secs(5)); // Blocks the async runtime!
        "done"
    });
```

**GOOD: Use async handlers or spawn_blocking**
```rust
let good_route = warp::path("block")
    .and_then(|| async {
        tokio::task::spawn_blocking(|| {
            std::thread::sleep(std::time::Duration::from_secs(5));
        }).await.unwrap();
        Ok::<_, warp::Rejection>("done")
    });
```

**BAD: Forgetting to handle CORS for browser requests**
```rust
let routes = warp::path("api")
    .map(|| "data");
// Browser will reject cross-origin requests
```

**GOOD: Add CORS middleware**
```rust
let routes = warp::path("api")
    .map(|| "data")
    .with(warp::cors()
        .allow_origin("https://example.com")
        .allow_methods(vec!["GET", "POST"]));
```
