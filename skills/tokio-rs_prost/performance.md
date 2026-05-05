### Zero-copy encoding with Bytes
```rust
use bytes::{Bytes, BytesMut};

// Efficient encoding with pre-allocated buffer
fn efficient_encode(msg: &MyMessage) -> Result<Bytes, prost::EncodeError> {
    let mut buf = BytesMut::with_capacity(msg.encoded_len());
    msg.encode(&mut buf)?;
    Ok(buf.freeze())  // Zero-copy conversion to Bytes
}

// Avoid unnecessary allocations
fn decode_from_bytes(data: Bytes) -> Result<MyMessage, prost::DecodeError> {
    MyMessage::decode(data)  // Uses Bytes directly
}
```

### Buffer reuse for multiple messages
```rust
fn batch_encode(messages: &[MyMessage]) -> Result<Vec<Vec<u8>>, prost::EncodeError> {
    let mut results = Vec::with_capacity(messages.len());
    let mut buf = Vec::new();
    
    for msg in messages {
        buf.clear();
        buf.reserve(msg.encoded_len());
        msg.encode(&mut buf)?;
        results.push(buf.clone());
    }
    
    Ok(results)
}
```

### Size estimation for allocation
```rust
fn optimized_serialization(msg: &MyMessage) -> Vec<u8> {
    let size = msg.encoded_len();
    let mut buf = Vec::with_capacity(size);
    msg.encode(&mut buf).unwrap();
    buf
}
```

### Avoid cloning large messages
```rust
// BAD: Cloning large messages
fn bad_clone(msg: &MyMessage) -> Vec<u8> {
    let cloned = msg.clone();  // Expensive for large messages
    cloned.encode_to_vec()
}

// GOOD: Encode directly from reference
fn good_encode(msg: &MyMessage) -> Result<Vec<u8>, prost::EncodeError> {
    let mut buf = Vec::with_capacity(msg.encoded_len());
    msg.encode(&mut buf)?;
    Ok(buf)
}
```
