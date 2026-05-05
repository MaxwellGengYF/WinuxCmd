**Warp filters are Send + Sync by default**
```rust
use std::sync::Arc;
use warp::Filter;

// Filters can be shared across threads safely
fn create_shared_filter() -> impl Filter<Extract = impl warp::Reply, Error = warp::Rejection> + Clone + Send + Sync {
    let shared_data = Arc::new(vec![1, 2, 3, 4, 5]);
    
    warp::path("data")
        .and(warp::any().map(move || shared_data.clone()))
        .map(|data: Arc<Vec<i32>>| {
            format!("Data: {:?}", data)
        })
}

#[tokio::main]
async fn main() {
    let filter = create_shared_filter();
    
    // Warp automatically handles threading with Tokio's work-stealing scheduler
    warp::serve(filter)
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```

**Use tokio::sync for shared state**
```rust
use tokio::sync::RwLock;
use std::sync::Arc;
use warp::Filter;

struct Database {
    users: RwLock<Vec<String>>,
}

async fn add_user(db: Arc<Database>, name: String) -> Result<impl warp::Reply, warp::Rejection> {
    let mut users = db.users.write().await;
    users.push(name);
    Ok("User added")
}

async fn get_users(db: Arc<Database>) -> Result<impl warp::Reply, warp::Rejection> {
    let users = db.users.read().await;
    Ok(warp::reply::json(&*users))
}

#[tokio::main]
async fn main() {
    let db = Arc::new(Database {
        users: RwLock::new(Vec::new()),
    });
    
    let add = warp::path("users")
        .and(warp::post())
        .and(warp::body::json::<String>())
        .and(warp::any().map(move || db.clone()))
        .and_then(|name: String, db: Arc<Database>| add_user(db, name));
    
    let get = warp::path("users")
        .and(warp::get())
        .and(warp::any().map(move || db.clone()))
        .and_then(|db: Arc<Database>| get_users(db));
    
    warp::serve(add.or(get))
        .run(([127, 0, 0, 1], 3030))
        .await;
}
```
