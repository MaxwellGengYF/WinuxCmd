### 1. NEVER decode untrusted data without size limits
```rust
// BAD: No size limit on untrusted data
fn bad_decode_untrusted(data: &[u8]) -> Result<MyMessage, prost::DecodeError> {
    MyMessage::decode(data)  // Potential DoS from large messages
}

// GOOD: Implement size limits
fn good_decode_untrusted(data: &[u8]) -> Result<MyMessage, prost::DecodeError> {
    const MAX_SIZE: usize = 10 * 1024 * 1024; // 10MB limit
    if data.len() > MAX_SIZE {
        return Err(prost::DecodeError::new("Message too large"));
    }
    MyMessage::decode(data)
}
```

### 2. NEVER use `unwrap()` on decode operations
```rust
// BAD: Panics on malformed data
fn bad_decode_panic(data: &[u8]) -> MyMessage {
    MyMessage::decode(data).unwrap()  // Security vulnerability!
}

// GOOD: Handle errors properly
fn good_decode_safe(data: &[u8]) -> Result<MyMessage, prost::DecodeError> {
    MyMessage::decode(data)
}
```

### 3. NEVER ignore encoding buffer capacity
```rust
// BAD: No capacity pre-allocation
fn bad_encode_no_capacity(msg: &MyMessage) -> Vec<u8> {
    let mut buf = Vec::new();
    msg.encode(&mut buf).unwrap();  // May cause reallocations
    buf
}

// GOOD: Pre-allocate buffer
fn good_encode_with_capacity(msg: &MyMessage) -> Result<Vec<u8>, prost::EncodeError> {
    let mut buf = Vec::with_capacity(msg.encoded_len());
    msg.encode(&mut buf)?;
    Ok(buf)
}
```

### 4. NEVER share mutable references across threads without synchronization
```rust
// BAD: Unsafe shared mutable state
fn bad_shared_mutable(msg: &mut MyMessage) {
    // This could cause data races if called from multiple threads
    msg.name = "modified".to_string();
}

// GOOD: Use proper synchronization
use std::sync::Mutex;
fn good_shared_mutable(msg: &Mutex<MyMessage>) {
    let mut guard = msg.lock().unwrap();
    guard.name = "modified".to_string();
}
```

### 5. NEVER assume field order in serialized data
```rust
// BAD: Assuming field order
fn bad_field_order(data: &[u8]) {
    // Fields may be in any order in protobuf
    let first_field = &data[0..4];  // Wrong assumption!
}

// GOOD: Use proper decoding
fn good_field_order(data: &[u8]) -> Result<MyMessage, prost::DecodeError> {
    MyMessage::decode(data)  // Handles any field order
}
```
