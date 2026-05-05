**Use typed extractors for request data**
```rust
use serde::Deserialize;
use warp::Filter;

#[derive(Deserialize)]
struct CreateUserRequest {
    username: String,
    email: String,
    age: u8,
}

fn create_user_route() -> impl Filter<Extract = impl warp::Reply, Error = warp::Rejection> + Clone {
    warp::path("users")
        .and(warp::post())
        .and(warp::body::json::<CreateUserRequest>())
        .and_then(handle_create_user)
}

async fn handle_create_user(req: CreateUserRequest) -> Result<impl warp::Reply, warp::Rejection> {
    // Validate and process
    if req.username.is_empty() {
        return Err(warp::reject::bad_request());
    }
    Ok(warp::reply::json(&req))
}
```

**Organize routes into modules**
```rust
// routes/mod.rs
pub mod users;
pub mod posts;

// routes/users.rs
use warp::Filter;

pub fn routes() -> impl Filter<Extract = impl warp::Reply, Error = warp::Rejection> + Clone {
    warp::path("users")
        .and(list_users()
            .or(create_user())
            .or(get_user()))
}

fn list_users() -> impl Filter<Extract = impl warp::Reply, Error = warp::Rejection> + Clone {
    warp::path::end()
        .and(warp::get())
        .map(|| "list users")
}
```

**Use middleware for logging and metrics**
```rust
use warp::Filter;

fn with_logging() -> impl Filter<Extract = (), Error = warp::Rejection> + Clone {
    warp::log::custom(|info| {
        println!("{} {} {}ms", info.method(), info.path(), info.elapsed().as_millis());
    })
}

#[tokio::main]
async fn main() {
    let routes = warp::path("api")
        .and(with_logging())
        .map(|| "Hello");

    warp::serve(routes)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```
