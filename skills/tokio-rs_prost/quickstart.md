```rust
// Add to Cargo.toml:
// cargo add prost
// cargo add prost-types
// cargo add prost-build --build
// cargo add bytes
// cargo add anyhow

// 1. Basic message encoding/decoding
use prost::Message;

#[derive(Clone, PartialEq, Message)]
pub struct Person {
    #[prost(string, tag = "1")]
    pub name: String,
    #[prost(int32, tag = "2")]
    pub age: i32,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let person = Person {
        name: "Alice".to_string(),
        age: 30,
    };
    
    let encoded = person.encode_to_vec();
    let decoded = Person::decode(&encoded[..])?;
    assert_eq!(person, decoded);
    Ok(())
}

// 2. Using bytes for zero-copy encoding
use bytes::BytesMut;

fn encode_with_bytes() -> Result<(), Box<dyn std::error::Error>> {
    let person = Person {
        name: "Bob".to_string(),
        age: 25,
    };
    
    let mut buf = BytesMut::with_capacity(64);
    person.encode(&mut buf)?;
    let decoded = Person::decode(buf.freeze())?;
    assert_eq!(person, decoded);
    Ok(())
}

// 3. Working with enums
#[derive(Clone, Copy, Debug, PartialEq, prost::Enumeration)]
pub enum Color {
    Red = 0,
    Green = 1,
    Blue = 2,
}

#[derive(Clone, PartialEq, Message)]
pub struct ColoredItem {
    #[prost(enumeration = "Color", tag = "1")]
    pub color: i32,
}

fn enum_example() -> Result<(), Box<dyn std::error::Error>> {
    let item = ColoredItem { color: Color::Green as i32 };
    let encoded = item.encode_to_vec();
    let decoded = ColoredItem::decode(&encoded[..])?;
    assert_eq!(Color::try_from(decoded.color).unwrap(), Color::Green);
    Ok(())
}

// 4. Nested messages
#[derive(Clone, PartialEq, Message)]
pub struct Address {
    #[prost(string, tag = "1")]
    pub street: String,
    #[prost(string, tag = "2")]
    pub city: String,
}

#[derive(Clone, PartialEq, Message)]
pub struct Employee {
    #[prost(message, tag = "1")]
    pub person: Option<Person>,
    #[prost(message, tag = "2")]
    pub address: Option<Address>,
}

fn nested_example() -> Result<(), Box<dyn std::error::Error>> {
    let employee = Employee {
        person: Some(Person {
            name: "Charlie".to_string(),
            age: 35,
        }),
        address: Some(Address {
            street: "123 Main St".to_string(),
            city: "Springfield".to_string(),
        }),
    };
    
    let encoded = employee.encode_to_vec();
    let decoded = Employee::decode(&encoded[..])?;
    assert_eq!(employee, decoded);
    Ok(())
}

// 5. Repeated fields (vectors)
#[derive(Clone, PartialEq, Message)]
pub struct ShoppingList {
    #[prost(string, repeated, tag = "1")]
    pub items: Vec<String>,
}

fn repeated_example() -> Result<(), Box<dyn std::error::Error>> {
    let list = ShoppingList {
        items: vec!["apples".to_string(), "bread".to_string(), "milk".to_string()],
    };
    
    let encoded = list.encode_to_vec();
    let decoded = ShoppingList::decode(&encoded[..])?;
    assert_eq!(list.items.len(), decoded.items.len());
    Ok(())
}
```
