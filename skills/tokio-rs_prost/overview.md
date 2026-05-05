`prost` is a Protocol Buffers implementation for Rust that generates idiomatic Rust code from `.proto` files. It's part of the tokio-rs ecosystem and focuses on simplicity, performance, and safety.

**When to use:**
- You need to serialize/deserialize structured data efficiently
- You're working with gRPC or other protobuf-based systems
- You want type-safe, compile-time checked serialization
- You need cross-language compatibility with protobuf

**When NOT to use:**
- For simple JSON/XML serialization (use serde instead)
- When you need runtime reflection or message descriptors
- For very small messages where protobuf overhead isn't justified
- When you need human-readable output (use JSON)

**Key design features:**
- Uses `bytes::{Buf, BufMut}` for zero-copy serialization
- Generates idiomatic Rust types with derive macros
- Preserves unknown enum values during deserialization
- Respects protobuf package specifiers for module organization
- Supports both proto2 and proto3 syntax

**Installation:**
```bash
cargo add prost
cargo add prost-types  # For well-known types
cargo add prost-build --build  # For build-time code generation
```
