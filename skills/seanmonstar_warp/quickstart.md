```bash
cargo add warp --features server
cargo add tokio --features full
```

```rust
use warp::Filter;

#[tokio::main]
async fn main() {
    // 1. Basic GET route with path parameter
    let hello = warp::path!("hello" / String)
        .map(|name| format!("Hello, {}!", name));

    // 2. POST with JSON body
    let create_user = warp::path("users")
        .and(warp::post())
        .and(warp::body::json())
        .map(|user: serde_json::Value| {
            warp::reply::json(&user)
        });

    // 3. Query string parameters
    let search = warp::path("search")
        .and(warp::query::<std::collections::HashMap<String, String>>())
        .map(|params: std::collections::HashMap<String, String>| {
            format!("Searching for: {:?}", params)
        });

    // 4. Static file serving
    let static_files = warp::path("static")
        .and(warp::fs::dir("./static"));

    // 5. Combined routes with error handling
    let routes = hello
        .or(create_user)
        .or(search)
        .or(static_files)
        .with(warp::cors().allow_any_origin());

    warp::serve(routes)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```
