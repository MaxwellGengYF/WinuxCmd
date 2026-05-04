# Overview

Serde is a framework for serializing and deserializing Rust data structures. It provides a generic way to convert between Rust types and various data formats (JSON, YAML, TOML, BSON, etc.) through the `Serialize` and `Deserialize` traits.

**When to use:**
- You need to convert Rust structs/enums to/from JSON, YAML, or other formats
- Building REST APIs, configuration files, or data exchange layers
- Working with complex nested data structures that need automatic serialization

**When NOT to use:**
- Simple string manipulation tasks where `format!()` suffices
- Performance-critical hot paths where manual serialization is faster
- Binary protocols requiring custom encoding schemes

**Key design:**
- Trait-based: `Serialize` for output, `Deserialize` for input
- Derive macros: `#[derive(Serialize, Deserialize)]` for automatic implementation
- Attribute-driven customization: `#[serde(rename, default, skip, flatten)]`
- Format-agnostic: same traits work with any supported format crate