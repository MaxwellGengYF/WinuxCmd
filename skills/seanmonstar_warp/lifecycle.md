**Filter construction and composition**
```rust
use warp::Filter;

// Filters are constructed and composed at startup
fn build_routes() -> impl Filter<Extract = impl warp::Reply, Error = warp::Rejection> + Clone {
    let base = warp::path("api").and(warp::path::param::<String>());
    
    // Filters are cloned cheaply (they're just function pointers)
    let get_handler = base.clone()
        .and(warp::get())
        .map(|id: String| format!("Get {}", id));
    
    let post_handler = base
        .and(warp::post())
        .and(warp::body::json::<serde_json::Value>())
        .map(|id: String, body: serde_json::Value| {
            format!("Post {} with {:?}", id, body)
        });
    
    get_handler.or(post_handler)
}

#[tokio::main]
async fn main() {
    let routes = build_routes();
    // Routes are consumed by serve(), which takes ownership
    warp::serve(routes)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```

**Shared state with Arc**
```rust
use std::sync::Arc;
use warp::Filter;

struct AppState {
    counter: std::sync::atomic::AtomicU64,
}

fn with_state(state: Arc<AppState>) -> impl Filter<Extract = (Arc<AppState>,), Error = std::convert::Infallible> + Clone {
    warp::any().map(move || state.clone())
}

#[tokio::main]
async fn main() {
    let state = Arc::new(AppState {
        counter: std::sync::atomic::AtomicU64::new(0),
    });
    
    let routes = warp::path("count")
        .and(with_state(state))
        .map(|state: Arc<AppState>| {
            let count = state.counter.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
            format!("Count: {}", count)
        });
    
    warp::serve(routes)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```
