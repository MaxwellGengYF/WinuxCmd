Warp is a composable web server framework built on top of hyper, using a unique Filter system that allows you to combine and chain request handlers. Each Filter represents a requirement on incoming requests, and they can be combined using `.and()`, `.or()`, and `.map()` to build complex routing logic.

**When to use:** You need a lightweight, type-safe web server with excellent performance for REST APIs, WebSocket support, and static file serving. Warp's filter system makes it easy to compose middleware and extract request data.

**When not to use:** You need a full-featured web framework with built-in ORM, templating, or admin panels. Consider Actix-web or Rocket for more batteries-included solutions.

**Key design:** Filters are zero-cost abstractions that compile to efficient code. The library uses traits extensively for composability, and all handlers are async by default.

```bash
cargo add warp --features server
cargo add tokio --features full
cargo add serde --features derive
```

```rust
use warp::Filter;
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize)]
struct User {
    name: String,
    age: u32,
}

#[tokio::main]
async fn main() {
    let json_route = warp::path("user")
        .and(warp::post())
        .and(warp::body::json::<User>())
        .map(|user: User| {
            warp::reply::json(&user)
        });

    warp::serve(json_route)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```
