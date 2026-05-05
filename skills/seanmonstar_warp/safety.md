**NEVER: Expose internal server errors to clients**
```rust
// BAD: Leaking error details
let bad = warp::any().map(|| {
    let result = std::fs::read_to_string("/etc/passwd");
    result.unwrap() // Panics and exposes error
});

// GOOD: Handle errors gracefully
let good = warp::any().and_then(|| async {
    match std::fs::read_to_string("/etc/passwd") {
        Ok(content) => Ok::<_, warp::Rejection>(content),
        Err(_) => Err(warp::reject::not_found()),
    }
});
```

**NEVER: Accept unsanitized user input in file paths**
```rust
// BAD: Path traversal vulnerability
let bad = warp::path!("files" / String)
    .map(|filename| {
        std::fs::read_to_string(format!("./uploads/{}", filename))
    });

// GOOD: Validate and sanitize paths
fn sanitize_path(input: &str) -> Option<String> {
    let path = std::path::Path::new(input);
    if path.components().any(|c| matches!(c, std::path::Component::ParentDir)) {
        return None;
    }
    Some(input.to_string())
}

let good = warp::path!("files" / String)
    .and_then(|filename: String| async move {
        match sanitize_path(&filename) {
            Some(safe) => Ok::<_, warp::Rejection>(safe),
            None => Err(warp::reject::not_found()),
        }
    });
```

**NEVER: Use unwrap() in request handlers**
```rust
// BAD: Will panic on bad input
let bad = warp::body::json::<serde_json::Value>()
    .map(|body| {
        body.as_object().unwrap()["key"].as_str().unwrap()
    });

// GOOD: Handle parsing errors
let good = warp::body::json::<serde_json::Value>()
    .and_then(|body| async {
        body.get("key")
            .and_then(|v| v.as_str())
            .map(|s| s.to_string())
            .ok_or_else(|| warp::reject::bad_request())
    });
```
