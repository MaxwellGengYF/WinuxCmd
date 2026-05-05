### Construction
```rust
use prost::Message;

#[derive(Clone, PartialEq, Message)]
pub struct MyMessage {
    #[prost(string, tag = "1")]
    pub data: String,
}

// Direct construction
let msg = MyMessage {
    data: "hello".to_string(),
};

// Default construction (all fields get default values)
let default_msg = MyMessage::default();
```

### Destruction and Drop
```rust
// Messages implement Drop automatically
fn process_message(msg: MyMessage) {
    // msg is consumed here
    let encoded = msg.encode_to_vec();
    // msg is dropped when function returns
}

// Clone for reuse
fn clone_example(msg: &MyMessage) {
    let cloned = msg.clone();
    // Both original and clone are valid
}
```

### Ownership patterns
```rust
// Taking ownership
fn take_ownership(msg: MyMessage) -> Vec<u8> {
    msg.encode_to_vec()  // msg is moved
}

// Borrowing
fn borrow_message(msg: &MyMessage) -> usize {
    msg.encoded_len()  // msg is borrowed
}

// Mutable borrowing
fn modify_message(msg: &mut MyMessage) {
    msg.data = "modified".to_string();
}
```

### RAII with encoding/decoding
```rust
use bytes::BytesMut;

struct EncodedMessage {
    buffer: BytesMut,
}

impl EncodedMessage {
    fn new(msg: &impl Message) -> Result<Self, prost::EncodeError> {
        let mut buffer = BytesMut::with_capacity(msg.encoded_len());
        msg.encode(&mut buffer)?;
        Ok(Self { buffer })
    }
}

impl Drop for EncodedMessage {
    fn drop(&mut self) {
        // Buffer is automatically cleaned up
        println!("Encoded message dropped");
    }
}
```
