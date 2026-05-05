### 1. Forgetting to handle Option fields in proto3
```rust
// BAD: Assuming message field is always present
#[derive(Clone, PartialEq, Message)]
pub struct BadExample {
    #[prost(message, tag = "1")]
    pub child: ChildMessage,  // This won't compile!
}

// GOOD: Use Option for message fields in proto3
#[derive(Clone, PartialEq, Message)]
pub struct GoodExample {
    #[prost(message, tag = "1")]
    pub child: Option<ChildMessage>,
}
```

### 2. Incorrect enum handling
```rust
// BAD: Using enum directly as field type
#[derive(Clone, PartialEq, Message)]
pub struct BadEnum {
    #[prost(enumeration = "Status", tag = "1")]
    pub status: Status,  // Wrong! Should be i32
}

// GOOD: Use i32 with enumeration attribute
#[derive(Clone, PartialEq, Message)]
pub struct GoodEnum {
    #[prost(enumeration = "Status", tag = "1")]
    pub status: i32,
}
```

### 3. Ignoring encoding errors
```rust
// BAD: Ignoring Result from encode
fn bad_encode(msg: &MyMessage) -> Vec<u8> {
    msg.encode_to_vec()  // Panics on error!
}

// GOOD: Handle encoding errors properly
fn good_encode(msg: &MyMessage) -> Result<Vec<u8>, prost::EncodeError> {
    let mut buf = Vec::new();
    msg.encode(&mut buf)?;
    Ok(buf)
}
```

### 4. Wrong tag numbers
```rust
// BAD: Duplicate or missing tags
#[derive(Clone, PartialEq, Message)]
pub struct BadTags {
    #[prost(string, tag = "1")]
    pub name: String,
    #[prost(string, tag = "1")]  // Duplicate tag!
    pub email: String,
}

// GOOD: Sequential unique tags
#[derive(Clone, PartialEq, Message)]
pub struct GoodTags {
    #[prost(string, tag = "1")]
    pub name: String,
    #[prost(string, tag = "2")]
    pub email: String,
}
```

### 5. Not handling unknown fields
```rust
// BAD: Assuming all fields are known
fn bad_decode(data: &[u8]) -> MyMessage {
    MyMessage::decode(data).unwrap()  // May lose data!
}

// GOOD: Use special_fields to preserve unknown data
fn good_decode(data: &[u8]) -> Result<MyMessage, prost::DecodeError> {
    let msg = MyMessage::decode(data)?;
    // Unknown fields are preserved in special_fields
    Ok(msg)
}
```

### 6. Incorrect map field usage
```rust
// BAD: Using wrong map syntax
#[derive(Clone, PartialEq, Message)]
pub struct BadMap {
    #[prost(map = "string, string", tag = "1")]
    pub properties: HashMap<String, String>,  // Wrong attribute syntax
}

// GOOD: Correct map field declaration
#[derive(Clone, PartialEq, Message)]
pub struct GoodMap {
    #[prost(map = "string, string", tag = "1")]
    pub properties: std::collections::HashMap<String, String>,
}
```
